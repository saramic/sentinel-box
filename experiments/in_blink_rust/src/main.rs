#![no_std]
#![no_main]

use cortex_m::asm;
use cortex_m_rt::entry;
use panic_halt as _;

// MAX32630 has a single GPIO peripheral with register arrays indexed by port.
// Source: mbed-os TARGET_MAX32630/device/gpio_regs.h + max3263x.h
//
// MXC_BASE_GPIO = 0x4000_A000
//   out_mode[port]  base + 0x0080 + port*4   4 bits per pin, pin N = bits[(N*4)+3:(N*4)]
//   out_val[port]   base + 0x00C0 + port*4   1 bit per pin
//
// out_mode values: 0x0=high-z pullup, 0x1=open-drain, 0x5=normal push-pull, 0xF=input disabled
const GPIO_BASE: u32 = 0x4000_A000;

const fn out_mode(port: u32) -> *mut u32 { (GPIO_BASE + 0x0080 + port * 4) as *mut u32 }
const fn out_val(port: u32)  -> *mut u32 { (GPIO_BASE + 0x00C0 + port * 4) as *mut u32 }

// MAX32630FTHR: Red LED = P2.4, active low, open-drain
const LED_PORT: u32 = 2;
const LED_PIN:  u32 = 4;
const OPEN_DRAIN: u32 = 0x1;

const DELAY_CYCLES: u32 = 12_000_000; // ~125ms at 96 MHz

#[entry]
fn main() -> ! {
    unsafe {
        // Set P2.4 to open-drain output (FTHR LEDs are active-low, open-drain)
        let mode = out_mode(LED_PORT).read_volatile();
        out_mode(LED_PORT).write_volatile(
            (mode & !(0xF << (LED_PIN * 4))) | (OPEN_DRAIN << (LED_PIN * 4)),
        );

        loop {
            // LED on: drive low (active low)
            out_val(LED_PORT).write_volatile(
                out_val(LED_PORT).read_volatile() & !(1 << LED_PIN),
            );
            asm::delay(DELAY_CYCLES);

            // LED off: drive high
            out_val(LED_PORT).write_volatile(
                out_val(LED_PORT).read_volatile() | (1 << LED_PIN),
            );
            asm::delay(DELAY_CYCLES);
        }
    }
}
