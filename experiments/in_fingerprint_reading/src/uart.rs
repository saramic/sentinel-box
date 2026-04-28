// UART2 driver — TX=P3.1, RX=P3.0, Map A.
//
// Adafruit fingerprint sensor (AS608 / R305 / R307 family) uses 8N1 at
// 57600 bps by default (configurable via the SysPara command to 9600–115200).
//
// Hardware resources confirmed from PeripheralPins.c:
//   UART2 Map A: TX=P3.1, RX=P3.0   ← pins on board header
//   UART1 Map A: TX=P2.1, RX=P2.0   ← NOT on board header
//   UART0 Map A: TX=P0.1, RX=P0.0   ← available if P3 is needed for something else
//
// P3.0 and P3.1 conflict with the MAX7219 bit-bang SPI pins used in
// in_attitude_meter, so in this experiment MAX7219 is wired to P3.3/P3.4/P3.5.
//
// Register reference: max3263x.h + uart_regs.h from mbed TARGET_MAX32630.

// CLKMAN — SYS_CLK_CTRL_8_UART (offset 0x0060) — shared clock for all UARTs.
const CLKMAN_SYS_CLK_CTRL_8_UART: *mut u32 = 0x4000_0460 as *mut u32;

// IOMAN — UART2_REQ (offset 0x0040): IO_MAP[0]=0 (Map A), IO_REQ[4]=1 → write 0x10.
const IOMAN_UART2_REQ: *mut u32 = 0x4000_0C40 as *mut u32;

// UART2 control registers — base 0x4001_4000
const UART2_CTRL:         *mut u32 = 0x4001_4000 as *mut u32; // offset 0x0000
const UART2_BAUD:         *mut u32 = 0x4001_4004 as *mut u32; // offset 0x0004
const UART2_TX_FIFO_CTRL: *mut u32 = 0x4001_4008 as *mut u32; // offset 0x0008
const UART2_RX_FIFO_CTRL: *mut u32 = 0x4001_400C as *mut u32; // offset 0x000C

// UART2 interrupt flags (INTFL) — offset 0x0014 (write-1-to-clear)
//   bit 3: RX_FIFO_NOT_EMPTY  — safe to read from FIFO when set
//   bit 0: TX_DONE            — TX FIFO empty and last byte shifted out
const UART2_INTFL: *mut u32 = 0x4001_4014 as *mut u32;

// UART2 data FIFOs — TX base 0x4010_5000, RX at +0x0800
const UART2_FIFO_TX: *mut u8   = 0x4010_5000 as *mut u8;
const UART2_FIFO_RX: *const u8 = 0x4010_5800 as *const u8;

// UART CTRL fields (uart_regs.h)
//   bit  0: UART_EN
//   bit  1: RX_FIFO_EN
//   bit  2: TX_FIFO_EN
//   bits [5:4]: DATA_SIZE  0b11 = 8 bits
//   bit  8: EXTRA_STOP     0 = 1 stop bit
//   bits [13:12]: PARITY   0b00 = no parity
// 8N1 with FIFOs enabled: 0b0000_0000_0011_0111 = 0x37
const CTRL_8N1_FIFO_EN: u32 = 0x37;

// UART BAUD register fields:
//   bits [7:0]  = BAUD_DIVISOR
//   bits [9:8]  = BAUD_MODE  (0 = 128× oversampling, 2 = fractional)
//
// For 57600 bps at 96 MHz with BAUD_MODE=0 (128× oversampling):
//   divisor = 96_000_000 / (57600 * 128) ≈ 13.02  → use 13 (≈0.16% error)
//
// TODO: verify with an oscilloscope or logic analyser on first bring-up.
// If the sensor does not respond, try BAUD_MODE=2 (fractional divider) or
// drop to 9600 bps (divisor = 96_000_000 / (9600 * 128) ≈ 78.1 → 78).
// const BAUD_57600: u32 = 13; // BAUD_DIVISOR, BAUD_MODE=0
const BAUD_57600: u32 = 78; // BAUD_DIVISOR, BAUD_MODE=0

pub fn init() {
    unsafe {
        CLKMAN_SYS_CLK_CTRL_8_UART.write_volatile(1); // DIV_1 — all UARTs
        IOMAN_UART2_REQ.write_volatile(0x10);          // Map A, IO_REQ=1
        UART2_BAUD.write_volatile(BAUD_57600);
        UART2_TX_FIFO_CTRL.write_volatile(0);
        UART2_RX_FIFO_CTRL.write_volatile(0);
        UART2_CTRL.write_volatile(CTRL_8N1_FIFO_EN);
    }
}

/// Write one byte to the TX FIFO.
pub fn write_byte(b: u8) {
    unsafe {
        // TX FIFO is 32 bytes deep; our packets are ≤12 bytes so overflow is not a concern.
        UART2_FIFO_TX.write_volatile(b);
    }
}

/// Block until the TX FIFO is empty and the last byte has shifted out.
/// Must be called before polling RX so the sensor has received the complete command.
pub fn flush_tx() {
    unsafe {
        // Clear TX_DONE first (W1C) in case it was already set from a prior idle state.
        UART2_INTFL.write_volatile(0x01);
        // Wait for TX_DONE to reassert: FIFO drained + last bit shifted out.
        while UART2_INTFL.read_volatile() & 0x01 == 0 {}
    }
}

/// Read one byte with a timeout. Returns None if no byte arrives within ~50 ms.
/// 500_000 iterations covers a 14-byte 9600 bps response (~15 ms) with margin.
pub fn read_byte() -> Option<u8> {
    let mut timeout = 500_000u32;
    unsafe {
        loop {
            // INTFL bit 3 = RX_FIFO_NOT_EMPTY (uart_regs.h MXC_F_UART_INTFL_RX_FIFO_NOT_EMPTY).
            // Only read from the FIFO when data is present — reading an empty FIFO
            // can hold the AHB bus indefinitely, blocking SWD access.
            if UART2_INTFL.read_volatile() & (1 << 3) != 0 {
                return Some(UART2_FIFO_RX.read_volatile());
            }
            timeout -= 1;
            if timeout == 0 {
                return None;
            }
        }
    }
}

/// Send a slice of bytes.
pub fn write_all(buf: &[u8]) {
    for &b in buf {
        write_byte(b);
    }
}
