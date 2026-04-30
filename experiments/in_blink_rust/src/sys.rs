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
const CLKMAN_CLK_CTRL:      *mut u32 = 0x4000_0400 as *mut u32; // offset 0x0000
const CLKMAN_SYS_SRC_96MHZ: u32      = 0x0000_0001;

// TRIM (factory calibration info block) — base 0x4000_1000
const TRIM_PWR_REG5: *const u32 = 0x4000_1034 as *const u32; // offset 0x0034
const TRIM_PWR_REG6: *const u32 = 0x4000_1038 as *const u32; // offset 0x0038

// PWRSEQ — base 0x4000_0800
const PWRSEQ_REG0: *mut u32 = 0x4000_0800 as *mut u32; // offset 0x0000
const PWRSEQ_REG5: *mut u32 = 0x4000_0814 as *mut u32; // offset 0x0014
const PWRSEQ_REG6: *mut u32 = 0x4000_0818 as *mut u32; // offset 0x0018
const PWRSEQ_REG0_HIRCEN: u32 = 1 << 12; // enable 96 MHz ring oscillator

// FLC (flash controller) — base 0x4000_2000
const FLC_CTRL:    *const u32 = 0x4000_2008 as *const u32; // offset 0x0008
const FLC_PERFORM: *mut u32   = 0x4000_2050 as *mut u32;   // offset 0x0050

// DWT (Data Watchpoint and Trace) — ARM CoreSight, fixed addresses on all Cortex-M4
const DEMCR:      *mut u32 = 0xE000_EDFC as *mut u32; // bit 24 = TRCENA, enables DWT
const DWT_CTRL:   *mut u32 = 0xE000_1000 as *mut u32; // bit 0 = CYCCNTENA
const DWT_CYCCNT: *mut u32 = 0xE000_1004 as *mut u32; // 32-bit cycle counter @ CPU freq
const DWT_LAR:    *mut u32 = 0xE000_1FB0 as *mut u32; // CoreSight Lock Access Register

// CPU clock frequency — Internal Relaxation Oscillator, typ 96 MHz (range 94–98 MHz).
pub const CPU_HZ: u32 = 96_000_000;

pub fn init() {
    unsafe { sys_init() }
}

/// Spin for exactly `cycles` CPU clock cycles using the DWT cycle counter.
/// Wraps correctly at 2^32 (~44.7 s at 96 MHz) via wrapping subtraction.
/// Falls back to a counted loop if DWT is not counting (CYCCNT stuck at 0).
pub fn delay_cycles(cycles: u32) {
    let start = unsafe { DWT_CYCCNT.read_volatile() };
    // If DWT_CYCCNT is stuck (returns start on every read), the wrapping
    // subtraction never advances and this loops forever.  Detect by checking
    // whether the counter has moved at all after a brief spin; if not, fall
    // back to a simple counted loop (~3 cycles/iter at 96 MHz).
    let probe = unsafe { DWT_CYCCNT.read_volatile() };
    if probe == start {
        // DWT not counting — use asm::delay as fallback (calibrated via CPU_HZ).
        // 3 cycles per iteration is typical for Cortex-M4 SUBS+BNE.
        cortex_m::asm::delay(cycles / 3);
        return;
    }
    while unsafe { DWT_CYCCNT.read_volatile() }.wrapping_sub(start) < cycles {}
}

unsafe fn sys_init() {
    // 1. Flash performance: AUTO_CLKDIV + back-to-back reads + merge-grab-GNT + AUTO_TACC.
    //    Must be first: without AUTO_CLKDIV, flash reads at 96 MHz are unreliable, which
    //    corrupts the TRIM values and leaves the HIRC oscillator running at ~48 MHz.
    //    Bits: [16] | [24] | [28] | [29] = 0x3701_0000
    let perform = FLC_PERFORM.read_volatile();
    FLC_PERFORM.write_volatile(perform | 0x3701_0000);

    // 2. Ensure the 96 MHz HIRC is fully powered, then switch to it.
    //    On cold boot CLKMAN defaults to HIRC/2 (48 MHz). HIRC is already
    //    running (it feeds HIRC/2), but setting HIRCEN in PWRSEQ_REG0 ensures
    //    it is in full-power mode before we remove the divider.
    //    A short delay lets the oscillator settle; no ready-bit poll is used
    //    because the ready bit position is undocumented in available headers.
    //    Source: LPSDK PreInit() in system_max3263x.c.
    PWRSEQ_REG0.write_volatile(PWRSEQ_REG0.read_volatile() | PWRSEQ_REG0_HIRCEN);
    cortex_m::asm::delay(200); // ~4 µs at 48 MHz — ring oscillator settles in < 1 µs

    // 3. Load oscillator trim from INFO block → PWRSEQ while still on HIRC/2.
    //    Must happen BEFORE the CLKMAN switch so HIRC is calibrated to 96 MHz
    //    before we select it. Without TRIM, HIRC runs at ~54 MHz uncalibrated.
    //    FLC_CTRL bit 25 = INFO_BLOCK_VALID. Flash reads are safe: AUTO_CLKDIV active.
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

    // 4. Select now-calibrated 96 MHz ring oscillator (bits [1:0] = 0x1).
    //    DSB+ISB flush the write and pipeline so subsequent code runs at 96 MHz.
    CLKMAN_CLK_CTRL.write_volatile(CLKMAN_SYS_SRC_96MHZ);
    cortex_m::asm::dsb();
    cortex_m::asm::isb();

    // 5. Enable DWT cycle counter.
    //    TRCENA must be set before any DWT register is accessed.
    //    DSB+ISB ensure the write propagates through the pipeline before we proceed.
    //    DWT_LAR unlock (key 0xC5ACCE55) is required on some implementations where
    //    the CoreSight lock is asserted after power-on reset.
    DEMCR.write_volatile(DEMCR.read_volatile() | (1 << 24)); // set TRCENA
    cortex_m::asm::dsb();
    cortex_m::asm::isb();
    DWT_LAR.write_volatile(0xC5AC_CE55); // unlock DWT registers
    DWT_CYCCNT.write_volatile(0);        // reset counter
    DWT_CTRL.write_volatile(DWT_CTRL.read_volatile() | 1); // set CYCCNTENA
    cortex_m::asm::dsb();
    cortex_m::asm::isb();
}
