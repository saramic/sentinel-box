// UART1 driver — TX=P2.1, RX=P2.0, Map A.
//
// Adafruit fingerprint sensor (AS608 / R305 / R307 family) uses 8N1 at
// 57600 bps by default (configurable via the SysPara command to 9600–115200).
//
// Hardware resources confirmed from PeripheralPins.c:
//   UART1 Map A: TX=P2.1, RX=P2.0
//   UART0 Map A: TX=P0.1, RX=P0.0  ← available if P2 is needed for something else
//
// Register reference: max3263x.h + uart_regs.h from mbed TARGET_MAX32630.

// CLKMAN — SYS_CLK_CTRL_8_UART (offset 0x0060) — shared clock for all UARTs.
const CLKMAN_SYS_CLK_CTRL_8_UART: *mut u32 = 0x4000_0460 as *mut u32;

// IOMAN — UART1_REQ (offset 0x0038): IO_MAP[0]=0 (Map A), IO_REQ[4]=1 → write 0x10.
const IOMAN_UART1_REQ: *mut u32 = 0x4000_0C38 as *mut u32;

// UART1 control registers — base 0x4001_3000
const UART1_CTRL:         *mut u32 = 0x4001_3000 as *mut u32; // offset 0x0000
const UART1_BAUD:         *mut u32 = 0x4001_3004 as *mut u32; // offset 0x0004
const UART1_TX_FIFO_CTRL: *mut u32 = 0x4001_3008 as *mut u32; // offset 0x0008
const UART1_RX_FIFO_CTRL: *mut u32 = 0x4001_300C as *mut u32; // offset 0x000C

// UART1 data FIFOs — TX base 0x4010_4000, RX at +0x0800
const UART1_FIFO_TX: *mut u8   = 0x4010_4000 as *mut u8;
const UART1_FIFO_RX: *const u8 = 0x4010_4800 as *const u8;

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
const BAUD_57600: u32 = 13; // BAUD_DIVISOR, BAUD_MODE=0

pub fn init() {
    unsafe {
        CLKMAN_SYS_CLK_CTRL_8_UART.write_volatile(1); // DIV_1 — all UARTs
        IOMAN_UART1_REQ.write_volatile(0x10);          // Map A, IO_REQ=1
        UART1_BAUD.write_volatile(BAUD_57600);
        UART1_TX_FIFO_CTRL.write_volatile(0);
        UART1_RX_FIFO_CTRL.write_volatile(0);
        UART1_CTRL.write_volatile(CTRL_8N1_FIFO_EN);
    }
}

/// Write one byte, blocking until the TX FIFO accepts it.
/// STATUS bit 4 = TX_BUSY / TX full — TODO: confirm exact bit from uart_regs.h.
pub fn write_byte(b: u8) {
    unsafe {
        // TODO: poll TX-not-full status before writing.
        UART1_FIFO_TX.write_volatile(b);
    }
}

/// Read one byte with a timeout. Returns None if no byte arrives within ~10 ms.
pub fn read_byte() -> Option<u8> {
    let mut timeout = 100_000u32;
    unsafe {
        // TODO: poll RX-not-empty status (STATUS bit, to be confirmed).
        // For now busy-spin on timeout and read from FIFO.
        loop {
            // placeholder: real check is RX FIFO not-empty status bit
            let _status = UART1_FIFO_RX.read_volatile();
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
