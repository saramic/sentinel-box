use crate::gpio::Pin;
use cortex_m::asm;

// MAX7219 register addresses
const REG_DECODE_MODE: u8 = 0x09;
const REG_INTENSITY: u8 = 0x0A;
const REG_SCAN_LIMIT: u8 = 0x0B;
const REG_SHUTDOWN: u8 = 0x0C;
const REG_DISPLAY_TEST: u8 = 0x0F;

pub struct Max7219 {
    din: Pin,
    cs: Pin,
    clk: Pin,
}

impl Max7219 {
    pub fn new(din: Pin, cs: Pin, clk: Pin) -> Self {
        Max7219 { din, cs, clk }
    }

    pub fn init(&mut self) {
        self.cs.set_high();
        self.clk.set_low();
        self.write_reg(REG_DISPLAY_TEST, 0x00); // normal operation
        self.write_reg(REG_SCAN_LIMIT, 0x07); // scan all 8 rows
        self.write_reg(REG_DECODE_MODE, 0x00); // no decode — raw pixels
        self.write_reg(REG_INTENSITY, 0x03); // low-medium intensity
        self.write_reg(REG_SHUTDOWN, 0x01); // exit shutdown → display on
        self.clear();
    }

    pub fn write_reg(&mut self, addr: u8, data: u8) {
        self.cs.set_low();
        self.send_byte(addr);
        self.send_byte(data);
        self.cs.set_high();
    }

    fn send_byte(&mut self, byte: u8) {
        for i in (0..8).rev() {
            if (byte >> i) & 1 != 0 {
                self.din.set_high();
            } else {
                self.din.set_low();
            }
            asm::delay(10);
            self.clk.set_high();
            asm::delay(10);
            self.clk.set_low();
        }
    }

    pub fn clear(&mut self) {
        for row in 1..=8u8 {
            self.write_reg(row, 0x00);
        }
    }

    // Light every LED in one row (rows 1–8).
    pub fn horizontal_bar(&mut self, row: u8) {
        self.clear();
        self.write_reg(row.clamp(1, 8), 0xFF);
    }

    // Light one column across all rows (col 0 = leftmost).
    pub fn vertical_bar(&mut self, col: u8) {
        self.clear();
        let mask = 0x80u8 >> col.min(7);
        for row in 1..=8u8 {
            self.write_reg(row, mask);
        }
    }
}
