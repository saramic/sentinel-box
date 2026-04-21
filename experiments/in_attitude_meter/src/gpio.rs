// MAX32630 GPIO peripheral — MXC_BASE_GPIO = 0x4000_A000
// Source: mbed-os TARGET_MAX32630/device/gpio_regs.h + max3263x.h
//
//   out_mode[port]  base + 0x0080 + port*4   4 bits per pin, pin N = bits[(N*4)+3:(N*4)]
//   out_val[port]   base + 0x00C0 + port*4   1 bit per pin
//
// out_mode nibble values:
//   0x0 = high-z with pull-up
//   0x1 = open-drain
//   0x5 = push-pull output
//   0xF = input disabled

const GPIO_BASE: u32 = 0x4000_A000;

const PUSH_PULL: u32 = 0x5;
#[allow(dead_code)]
const OPEN_DRAIN: u32 = 0x1;

fn out_mode_reg(port: u32) -> *mut u32 {
    (GPIO_BASE + 0x0080 + port * 4) as *mut u32
}

fn out_val_reg(port: u32) -> *mut u32 {
    (GPIO_BASE + 0x00C0 + port * 4) as *mut u32
}

pub struct Pin {
    port: u32,
    pin: u32,
}

impl Pin {
    fn configure(port: u32, pin: u32, mode: u32) -> Self {
        unsafe {
            let reg = out_mode_reg(port);
            let val = reg.read_volatile();
            reg.write_volatile((val & !(0xF << (pin * 4))) | (mode << (pin * 4)));
        }
        Pin { port, pin }
    }

    pub fn push_pull(port: u32, pin: u32) -> Self {
        Self::configure(port, pin, PUSH_PULL)
    }

    #[allow(dead_code)]
    pub fn open_drain(port: u32, pin: u32) -> Self {
        Self::configure(port, pin, OPEN_DRAIN)
    }

    pub fn set_high(&mut self) {
        unsafe {
            let reg = out_val_reg(self.port);
            reg.write_volatile(reg.read_volatile() | (1 << self.pin));
        }
    }

    pub fn set_low(&mut self) {
        unsafe {
            let reg = out_val_reg(self.port);
            reg.write_volatile(reg.read_volatile() & !(1 << self.pin));
        }
    }
}
