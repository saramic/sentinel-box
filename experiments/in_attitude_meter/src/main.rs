#![no_std]
#![no_main]

use cortex_m_rt::entry;
use panic_halt as _;

mod bmi160;
mod gpio;
mod max7219;
mod pmic;
mod sys;

use gpio::Pin;
use max7219::Max7219;
use sys::CPU_HZ;

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
    sys::delay_cycles(CPU_HZ / 10); // 100 ms

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

    // sample bmi160 at 200 Hz and align this with the delay in the loop
    let odr = bmi160::AccOdr::Hz200;
    bmi160::acc_init(odr);

    let chip_id_post = bmi160::read_chip_id();
    let intfl_post   = bmi160::read_intfl() as u8;

    display.write_reg(1, chip_id_pre);
    display.write_reg(2, intfl_pre);
    display.write_reg(3, chip_id_post);
    display.write_reg(4, intfl_post);
    sys::delay_cycles(CPU_HZ * 3); // 3 s

    display.clear();

    // row 4 (0-indexed) = row 5 from top: level position
    let mut bar_pos: f32 = 4.0;

    loop {
        let (ax, ay, _az) = bmi160::read_accel_xyz();

        // ay drives velocity: tilt makes bar scroll, urging user to counter-tilt
        let velocity = -(ay as f32) / 32768.0;
        let next = bar_pos + velocity;
        let r = next % 8.0;
        bar_pos = if r < 0.0 { r + 8.0 } else { r };

        // ax tilts the bar diagonally: ±3.5 rows across 8 columns at max tilt
        let slope = (ax as f32) / 32768.0 * 3.5;

        let mut rows = [0u8; 8];
        for col in 0..8u32 {
            let col_pos = bar_pos + slope * (col as f32 - 3.5);
            let r = col_pos % 8.0;
            let wrapped = if r < 0.0 { r + 8.0 } else { r };
            let row = ((wrapped + 0.5) as usize) % 8;
            rows[row] |= 1 << col;
        }

        display.clear();
        for (i, &mask) in rows.iter().enumerate() {
            if mask != 0 {
                display.write_reg((i + 1) as u8, mask);
            }
        }

        // aligned with the bmi160 sample rate
        sys::delay_cycles(odr.delay_cycles());
    }
}
