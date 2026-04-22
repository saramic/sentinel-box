// System initialisation — mirrors LPSDK PreInit() + SystemInit() from
// system_max3263x.c. Must be the very first call in main().
//
// Three things happen here that are required for reliable cold-boot operation:
//
//  1. Select the 96 MHz ring oscillator (CLKMAN). After POR the source may be
//     the divided-down version (48 MHz), making all timing calculations wrong.
//
//  2. Load factory oscillator trim values from the INFO block into the power
//     sequencer (PWRSEQ). Without this the 96 MHz oscillator is uncalibrated
//     after POR — it may be unstable or off-frequency. These values survive an
//     SWD reset (warm) but are lost on a true power cycle (cold), which is why
//     the firmware worked after OpenOCD upload but not after unplugging power.
//
//  3. Enable FLC AUTO_CLKDIV so the flash controller derives its own clock
//     divider from the running frequency. Without it flash reads can be
//     corrupted at 96 MHz on cold boot.
//
// Source: LPSDK system_max3263x.c (mbed TARGET_MAX32630 device driver).

// CLKMAN — base 0x4000_0400
const CLKMAN_CLK_CTRL: *mut u32 = 0x4000_0404 as *mut u32; // offset 0x0004

// TRIM (factory calibration info block) — base 0x4000_1000
const TRIM_PWR_REG5: *const u32 = 0x4000_1034 as *const u32; // offset 0x0034
const TRIM_PWR_REG6: *const u32 = 0x4000_1038 as *const u32; // offset 0x0038

// PWRSEQ — base 0x4000_0800
const PWRSEQ_REG5: *mut u32 = 0x4000_0814 as *mut u32; // offset 0x0014
const PWRSEQ_REG6: *mut u32 = 0x4000_0818 as *mut u32; // offset 0x0018

// FLC (flash controller) — base 0x4000_2000
const FLC_CTRL:    *const u32 = 0x4000_2008 as *const u32; // offset 0x0008
const FLC_PERFORM: *mut u32   = 0x4000_2050 as *mut u32;   // offset 0x0050

pub fn init() {
    unsafe { sys_init() }
}

unsafe fn sys_init() {
    // 1. Select 96 MHz ring oscillator (bits [1:0] = 0x1).
    CLKMAN_CLK_CTRL.write_volatile(0x0000_0001);

    // 2. Load oscillator trim from INFO block → PWRSEQ.
    //    FLC_CTRL bit 25 = INFO_BLOCK_VALID.
    let flc_ctrl = FLC_CTRL.read_volatile();
    let trim5    = TRIM_PWR_REG5.read_volatile();
    let trim6    = TRIM_PWR_REG6.read_volatile();

    if (flc_ctrl & (1 << 25)) != 0 && trim5 != 0xFFFF_FFFF && trim6 != 0xFFFF_FFFF {
        PWRSEQ_REG5.write_volatile(trim5);
        PWRSEQ_REG6.write_volatile(trim6);
    } else {
        // No valid INFO block — apply a safe default VREF trim for reg6.
        // Bits [24:16] = OSC_VREF field; default safe value = 0x1E0.
        let r6 = PWRSEQ_REG6.read_volatile();
        PWRSEQ_REG6.write_volatile((r6 & !0x01FF_0000) | (0x1E0 << 16));
    }

    // 3. Flash performance: AUTO_CLKDIV + back-to-back reads + merge-grab-GNT + AUTO_TACC.
    //    Bits: [16] | [24] | [28] | [29] = 0x3701_0000
    let perform = FLC_PERFORM.read_volatile();
    FLC_PERFORM.write_volatile(perform | 0x3701_0000);
}
