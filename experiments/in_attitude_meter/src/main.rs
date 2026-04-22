#![no_std]
#![no_main]

use cortex_m::asm;
use cortex_m_rt::entry;
use panic_halt as _;

mod gpio;
mod max7219;
mod pmic;
mod sys;

use gpio::Pin;
use max7219::Max7219;

// MAX7219 bit-bang SPI pins — P3 header on MAX32630FTHR
const DIN_PORT: u32 = 3;
const DIN_PIN: u32 = 0;
const CLK_PORT: u32 = 3;
const CLK_PIN: u32 = 1;
const CS_PORT: u32 = 3;
const CS_PIN: u32 = 2;

const DELAY_CYCLES: u32 = 12_000_000; // ~125 ms at 96 MHz

#[entry]
fn main() -> ! {
    sys::init();
    pmic::init();
    // LDO3 (3V3 rail) just came up — give MAX7219 time to complete power-on reset.
    // On warm restart LDO3 was already on so this is a no-op cost; cold boot needs it.
    cortex_m::asm::delay(9_600_000); // ~100 ms at 96 MHz

    let mut display = Max7219::new(
        Pin::push_pull(DIN_PORT, DIN_PIN),
        Pin::push_pull(CS_PORT, CS_PIN),
        Pin::push_pull(CLK_PORT, CLK_PIN),
    );
    display.init();

    // Display test: all LEDs on for 1 second, then normal operation.
    // If the matrix lights up here, SPI wiring is correct.
    display.write_reg(0x0F, 0x01); // display test on
    asm::delay(96_000_000);        // ~1 s
    display.write_reg(0x0F, 0x00); // display test off

    loop {
        // Sweep horizontal bars row 1..8
        for row in 1..=8u8 {
            display.horizontal_bar(row);
            asm::delay(DELAY_CYCLES);
        }

        // Sweep vertical bars col 0..7
        for col in 0..8u8 {
            display.vertical_bar(col);
            asm::delay(DELAY_CYCLES);
        }
    }
}
