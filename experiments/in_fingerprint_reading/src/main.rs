#![no_std]
#![no_main]

use cortex_m::asm;
use cortex_m_rt::entry;
use panic_halt as _;

mod fingerprint;
mod gpio;
mod max7219;
mod pmic;
mod sys;
mod uart;

use gpio::Pin;
use max7219::Max7219;

// MAX7219 bit-bang SPI pins.
// P3.0/P3.1 are reserved for UART2 (fingerprint sensor), so MAX7219 moves to P3.3/P3.4/P3.5.
const DIN_PORT: u32 = 3;
const DIN_PIN: u32 = 3;
const CLK_PORT: u32 = 3;
const CLK_PIN: u32 = 4;
const CS_PORT: u32 = 3;
const CS_PIN: u32 = 5;

#[entry]
fn main() -> ! {
    sys::init();
    pmic::init();
    // LDO3 (3V3 rail) just came up — let MAX7219 and fingerprint sensor complete POR.
    asm::delay(9_600_000); // ~100 ms at 96 MHz

    let mut display = Max7219::new(
        Pin::push_pull(DIN_PORT, DIN_PIN),
        Pin::push_pull(CS_PORT, CS_PIN),
        Pin::push_pull(CLK_PORT, CLK_PIN),
    );
    display.init();

    uart::init();

    // Display test — confirms wiring before fingerprint work begins.
    display.write_reg(0x0F, 0x01); // display test on
    asm::delay(96_000_000);        // ~1 s
    display.write_reg(0x0F, 0x00); // display test off
    display.clear();

    // Verify the fingerprint sensor is responding.
    // Row 1 all on = sensor alive; row 8 all on = no response.
    let alive = fingerprint::verify_password() == fingerprint::OK;
    display.write_reg(if alive { 1 } else { 8 }, 0xFF);
    asm::delay(192_000_000); // ~2 s
    display.clear();

    if !alive {
        // Sensor not responding — blink row 8 forever as error indicator.
        loop {
            display.write_reg(8, 0xFF);
            asm::delay(48_000_000);
            display.clear();
            asm::delay(48_000_000);
        }
    }

    // TODO: implement the main fingerprint loop:
    //   1. Wait for a finger (get_image loop)
    //   2. Convert image to template (image_to_tz slot 1)
    //   3. Search the database (finger_search)
    //   4. On match: show match ID on display rows, green LED or similar
    //   5. On no match: show error row, offer enrolment
    loop {
        asm::delay(96_000_000);
    }
}
