#![no_std]
#![no_main]

use cortex_m::asm;
use cortex_m_rt::entry;
use panic_halt as _;

mod bmi160;
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

    // --- Diagnostic phase (4 rows, 3 seconds) ---
    // Row 1: chip ID before ACC_NORMAL write  → expect 0xD1 (●●·●···●)
    // Row 2: INTFL after that read            → expect TX_DONE, no NACK/RX_UND
    // Row 3: chip ID after  ACC_NORMAL write  → expect 0xD1 still
    // Row 4: INTFL after CMD write            → expect TX_DONE

    bmi160::hw_init();

    let chip_id_pre  = bmi160::read_chip_id();
    let intfl_pre    = bmi160::read_intfl() as u8;

    bmi160::acc_init();

    let chip_id_post = bmi160::read_chip_id();
    let intfl_post   = bmi160::read_intfl() as u8;

    display.write_reg(1, chip_id_pre);
    display.write_reg(2, intfl_pre);
    display.write_reg(3, chip_id_post);
    display.write_reg(4, intfl_post);
    asm::delay(288_000_000); // ~3 s

    display.clear();

    loop {
        let ax = bmi160::read_accel_x();
        let ay = bmi160::read_accel_y();

        let row_x = (2i32 - (ax as i32 / 8192)).clamp(1, 4) as u8;
        let row_y = (6i32 - (ay as i32 / 8192)).clamp(5, 8) as u8;
        display.clear();
        display.write_reg(row_x, 0xFF);
        display.write_reg(row_y, 0xFF);

        asm::delay(9_600_000); // ~100 ms
    }
}
