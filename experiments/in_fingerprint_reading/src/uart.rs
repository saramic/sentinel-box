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

// IOMAN — UART2_REQ (offset 0x0040) / UART2_ACK (offset 0x0044).
// IO_MAP[0]=0 (Map A: P3.0/P3.1), IO_REQ[4]=1 → write 0x10.
// ACK must read back 0x10 before the pins are usable as UART2.
const IOMAN_UART2_REQ: *mut u32    = 0x4000_0C40 as *mut u32;
const IOMAN_UART2_ACK: *const u32  = 0x4000_0C44 as *const u32;

// GPIO — base 0x4000_A000
// out_mode[port] = base + 0x0080 + port*4, 4 bits per pin (pin N = bits [N*4+3:N*4])
//   0x0 = HIGH_Z input (default after reset)
//   0x5 = normal push-pull output  ← required for UART TX to drive the wire
// out_val[port] = base + 0x00C0 + port*4, 1 bit per pin
const GPIO_OUT_MODE_P3: *mut u32 = (0x4000_A000 + 0x0080 + 3 * 4) as *mut u32;
const GPIO_OUT_VAL_P3:  *mut u32 = (0x4000_A000 + 0x00C0 + 3 * 4) as *mut u32;

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
// Baud rate divisors for BAUD_MODE=0 (128× oversampling).
// CPU runs at 96 MHz (calibrated HIRC, confirmed via asm::delay probe ~1550 µs).
//   divisor = 96_000_000 / (baud * 128)
const BAUD_57600: u32 = 13; // 96_000_000 / (57600 * 128) = 13.02 → 13, 0.16% error
const BAUD_9600:  u32 = 78; // 96_000_000 / (9600 * 128)  = 78.13 → 78, 0.16% error

pub fn init() {
    unsafe {
        CLKMAN_SYS_CLK_CTRL_8_UART.write_volatile(1); // DIV_1 — all UARTs

        // P3.1 (TX): pre-set HIGH (UART idle) then switch to normal push-pull output.
        // P3.0 (RX): leave as HIGH_Z input (default 0x0).
        // Without this the GPIO drive is HIGH_Z after reset and UART TX has no output.
        GPIO_OUT_VAL_P3.write_volatile(GPIO_OUT_VAL_P3.read_volatile() | (1 << 1));
        let mode = GPIO_OUT_MODE_P3.read_volatile();
        GPIO_OUT_MODE_P3.write_volatile((mode & !0xF0) | 0x50); // P3.1 = 0x5 normal drive

        IOMAN_UART2_REQ.write_volatile(0x10);                          // Map A, IO_REQ=1
        // Wait up to ~10 ms for IOMAN to grant pins — bounded so a bad ACK value
        // doesn't hang forever, but long enough for the arbiter to respond.
        let mut ioman_timeout = 960_000_u32;
        while IOMAN_UART2_ACK.read_volatile() != 0x10 && ioman_timeout > 0 {
            ioman_timeout -= 1;
        }
        // UART2_BAUD.write_volatile(BAUD_57600); // match sensor factory default; change to BAUD_9600 if reconfigured
        UART2_BAUD.write_volatile(BAUD_9600); // match sensor factory default; change to BAUD_9600 if reconfigured
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
/// 500_000 iterations covers a 14-byte 57600 bps response (~2.4 ms) with large margin.
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
