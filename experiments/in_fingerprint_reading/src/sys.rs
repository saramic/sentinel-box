// System initialisation for MAX32630 — mirrors LPSDK PreInit() + SystemInit()
// + CLKMAN_TrimRO() from system_max3263x.c. Must be the first call in main().
//
// Correct cold-boot sequence to reach 96 MHz:
//  1. FLC_PERFORM — enable AUTO_CLKDIV and fast-read flags for flash at 96 MHz.
//  2. Load factory oscillator trim from INFO block → PWRSEQ_REG5/REG6. This gives
//     a good initial VREF so HIRC is close to 96 MHz even before ADC calibration.
//     Mirrors LPSDK SystemInit() lines 206-215.
//  3. Enable ADC clock via CLKMAN (required for ring oscillator calibration hardware).
//  4. trim_ro() — ADC measures HIRC against 32kHz RTC, refines PWRSEQ_REG6
//     TRIM_OSC_VREF until HIRC locks exactly to 96 MHz. (LPSDK: CLKMAN_TrimRO())
//     PWRSEQ_REG6 is only updated if calibration actually completes; if it times
//     out, the INFO-block value loaded in step 2 remains in effect (~96 MHz).
//  5. Switch system clock from HIRC/2 → HIRC (now calibrated = 96 MHz).
//  6. Enable DWT cycle counter for delay_cycles().
//
// Key corrections from prior versions:
//  - CLKMAN_CLK_CTRL is at CLKMAN base + 0x0004 (NOT base + 0x0000 = CLK_CONFIG).
//  - PWRSEQ base is 0x4000_0A30 (NOT 0x4000_0800 = PWRMAN).
//  - INFO-block trim loaded to PWRSEQ (step 2) before ADC calibration (step 4).
//  - trim_ro() guards PWRSEQ_REG6 write — skipped if calibration times out.

// CLKMAN — base 0x4000_0400 (MXC_BASE_CLKMAN)
// clk_ctrl is the SECOND register at offset 0x0004, not the base.
const CLKMAN_CLK_CTRL: *mut u32 = 0x4000_0404 as *mut u32; // offset 0x0004
const CLKMAN_CLK_CTRL_SYS_SRC_HIRC: u32 = 0x0000_0001;    // bits [1:0] = 1 → 96 MHz RO
const CLKMAN_CLK_CTRL_ADC_CLK_EN:   u32 = 0x0100_0000;    // bit 24 → enable ADC clock

// FLC (flash controller) — base 0x4000_2000 (MXC_BASE_FLC)
const FLC_CTRL:    *const u32 = 0x4000_2008 as *const u32; // offset 0x0008
const FLC_PERFORM: *mut u32   = 0x4000_2050 as *mut u32;   // offset 0x0050

// TRIM (factory calibration info block) — base 0x4000_1000 (MXC_BASE_TRIM)
const TRIM_PWR_REG5: *const u32 = 0x4000_1034 as *const u32; // offset 0x0034
const TRIM_PWR_REG6: *const u32 = 0x4000_1038 as *const u32; // offset 0x0038

// ADC (ring oscillator calibration) — base 0x4001_F000 (MXC_BASE_ADC)
const ADC_INTR:    *mut u32 = 0x4001_F00C as *mut u32; // offset 0x00C
const ADC_RO_CAL0: *mut u32 = 0x4001_F024 as *mut u32; // offset 0x024
const ADC_RO_CAL1: *mut u32 = 0x4001_F028 as *mut u32; // offset 0x028

const ADC_INTR_RO_CAL_DONE_IE:   u32 = 1 << 5;        // bit  5: calibration done IE
const ADC_INTR_RO_CAL_DONE_IF:   u32 = 1 << 21;       // bit 21: calibration done IF (W1C)
const ADC_RO_CAL0_RO_CAL_EN:     u32 = 1 << 0;        // bit  0: enable frequency loop
const ADC_RO_CAL0_RO_CAL_RUN:    u32 = 1 << 1;        // bit  1: run (clear to stop)
const ADC_RO_CAL0_RO_CAL_LOAD:   u32 = 1 << 2;        // bit  2: load initial trim to active
const ADC_RO_CAL0_RO_CAL_ATOMIC: u32 = 1 << 4;        // bit  4: one-shot atomic mode
const ADC_RO_CAL0_RO_TRM_MASK:   u32 = 0xFF80_0000;   // bits [31:23]: 9-bit trim result
const ADC_RO_CAL0_RO_TRM_POS:    u32 = 23;
const ADC_RO_CAL1_TRM_INIT_MASK: u32 = 0x0000_01FF;   // bits  [8:0]: 9-bit initial seed

// PWRSEQ — base 0x4000_0A30 (MXC_BASE_PWRSEQ — corrected from wrong 0x4000_0800 = PWRMAN)
const PWRSEQ_REG0: *mut u32 = 0x4000_0A30 as *mut u32; // offset 0x000
const PWRSEQ_REG5: *mut u32 = 0x4000_0A44 as *mut u32; // offset 0x014
const PWRSEQ_REG6: *mut u32 = 0x4000_0A48 as *mut u32; // offset 0x018

const PWRSEQ_REG0_PWR_RTCEN_RUN:      u32 = 1 << 11;       // bit 11: enable 32kHz RTC OSC
const PWRSEQ_REG6_TRIM_OSC_VREF_MASK: u32 = 0x000F_F800;   // bits [19:11]: 9-bit VREF trim
const PWRSEQ_REG6_TRIM_OSC_VREF_POS:  u32 = 11;

// RTCCFG — base 0x4000_0A70 (MXC_BASE_RTCCFG)
const RTCCFG_OSC_CTRL: *mut u32 = 0x4000_0A7C as *mut u32; // offset 0x00C
const RTCCFG_OSC_WARMUP_ENABLE: u32 = 1 << 14; // bit 14: clears when 32kHz OSC is ready

// DWT (Data Watchpoint and Trace) — ARM CoreSight, fixed on all Cortex-M4
const DEMCR:      *mut u32 = 0xE000_EDFC as *mut u32; // bit 24 = TRCENA
const DWT_CTRL:   *mut u32 = 0xE000_1000 as *mut u32; // bit 0 = CYCCNTENA
const DWT_CYCCNT: *mut u32 = 0xE000_1004 as *mut u32; // 32-bit cycle counter
const DWT_LAR:    *mut u32 = 0xE000_1FB0 as *mut u32; // CoreSight Lock Access Register

// CPU clock frequency after successful calibration.
pub const CPU_HZ: u32 = 96_000_000;

pub fn init() {
    unsafe { sys_init() }
}

/// Spin for exactly `cycles` CPU clock cycles using the DWT cycle counter.
/// Falls back to a counted loop if DWT is not counting (CYCCNT stuck at 0).
pub fn delay_cycles(cycles: u32) {
    let start = unsafe { DWT_CYCCNT.read_volatile() };
    let probe = unsafe { DWT_CYCCNT.read_volatile() };
    if probe == start {
        // DWT not counting — fallback (~3 cycles/iter on Cortex-M4).
        cortex_m::asm::delay(cycles / 3);
        return;
    }
    while unsafe { DWT_CYCCNT.read_volatile() }.wrapping_sub(start) < cycles {}
}

unsafe fn sys_init() {
    // 1. Flash performance: AUTO_CLKDIV + back-to-back reads + merge-grab-GNT + AUTO_TACC.
    //    Must be first so flash reads are reliable before the 96 MHz switch.
    //    Bits: [16] | [24] | [28] | [29] = 0x3701_0000
    let perform = FLC_PERFORM.read_volatile();
    FLC_PERFORM.write_volatile(perform | 0x3701_0000);

    // 2. Load factory oscillator trim from INFO block into PWRSEQ_REG5/REG6.
    //    Mirrors LPSDK SystemInit() — must happen BEFORE trim_ro() so the initial
    //    TRIM_OSC_VREF seed is a valid factory value (~96 MHz), not 0.
    //    If INFO block is absent/erased, apply the documented safe default (0x1E0).
    let flc_ctrl = FLC_CTRL.read_volatile();
    let trim5    = TRIM_PWR_REG5.read_volatile();
    let trim6    = TRIM_PWR_REG6.read_volatile();
    if (flc_ctrl & (1 << 25)) != 0 && trim5 != 0xFFFF_FFFF && trim6 != 0xFFFF_FFFF {
        PWRSEQ_REG5.write_volatile(trim5);
        PWRSEQ_REG6.write_volatile(trim6);
    } else {
        let r6 = PWRSEQ_REG6.read_volatile();
        PWRSEQ_REG6.write_volatile(
            (r6 & !PWRSEQ_REG6_TRIM_OSC_VREF_MASK)
                | (0x1E0 << PWRSEQ_REG6_TRIM_OSC_VREF_POS),
        );
    }

    // 3. Enable ADC clock — required for the ring oscillator calibration hardware.
    CLKMAN_CLK_CTRL.write_volatile(
        CLKMAN_CLK_CTRL.read_volatile() | CLKMAN_CLK_CTRL_ADC_CLK_EN,
    );

    // 4. Calibrate HIRC to 96 MHz using ADC + 32kHz RTC (LPSDK: CLKMAN_TrimRO).
    //    CPU is still on HIRC/2 (~56 MHz uncalibrated) during calibration.
    //    If calibration times out, PWRSEQ_REG6 retains the INFO-block value from step 2.
    trim_ro();

    // 5. Switch system clock from HIRC/2 → HIRC (96 MHz, now calibrated).
    //    Preserve other bits; only change SYSTEM_SOURCE_SELECT [1:0].
    CLKMAN_CLK_CTRL.write_volatile(
        (CLKMAN_CLK_CTRL.read_volatile() & !0x0000_0003) | CLKMAN_CLK_CTRL_SYS_SRC_HIRC,
    );
    cortex_m::asm::dsb();
    cortex_m::asm::isb();

    // 6. Enable DWT cycle counter.
    DEMCR.write_volatile(DEMCR.read_volatile() | (1 << 24)); // set TRCENA
    cortex_m::asm::dsb();
    cortex_m::asm::isb();
    DWT_LAR.write_volatile(0xC5AC_CE55); // unlock DWT registers
    DWT_CYCCNT.write_volatile(0);
    DWT_CTRL.write_volatile(DWT_CTRL.read_volatile() | 1); // set CYCCNTENA
    cortex_m::asm::dsb();
    cortex_m::asm::isb();
}

/// Calibrate the ring oscillator (HIRC) to 96 MHz.
/// Mirrors LPSDK CLKMAN_TrimRO() from system_max3263x.c.
/// Only writes PWRSEQ_REG6 if calibration completes — if it times out the
/// INFO-block value loaded by sys_init() step 2 remains intact.
unsafe fn trim_ro() {
    // 1. Enable 32kHz RTC oscillator (calibration reference).
    let was_running = PWRSEQ_REG0.read_volatile() & PWRSEQ_REG0_PWR_RTCEN_RUN;
    PWRSEQ_REG0.write_volatile(PWRSEQ_REG0.read_volatile() | PWRSEQ_REG0_PWR_RTCEN_RUN);

    // 2. Wait for 32kHz oscillator warmup — bit 14 clears when ready.
    //    Bounded: return without calibrating if no crystal (leaves INFO-block trim intact).
    let mut wd = 5_000_000_u32;
    while RTCCFG_OSC_CTRL.read_volatile() & RTCCFG_OSC_WARMUP_ENABLE != 0 {
        wd = wd.wrapping_sub(1);
        if wd == 0 {
            if was_running == 0 {
                PWRSEQ_REG0.write_volatile(
                    PWRSEQ_REG0.read_volatile() & !PWRSEQ_REG0_PWR_RTCEN_RUN,
                );
            }
            return;
        }
    }

    // 3. Enable and clear RO calibration done flag (W1C).
    ADC_INTR.write_volatile(ADC_INTR.read_volatile() | ADC_INTR_RO_CAL_DONE_IE);
    ADC_INTR.write_volatile(ADC_INTR.read_volatile() | ADC_INTR_RO_CAL_DONE_IF);

    // 4. Seed with current PWRSEQ_REG6 TRIM_OSC_VREF as initial trim value.
    let init_trim = (PWRSEQ_REG6.read_volatile() & PWRSEQ_REG6_TRIM_OSC_VREF_MASK)
        >> PWRSEQ_REG6_TRIM_OSC_VREF_POS;
    ADC_RO_CAL1.write_volatile(
        (ADC_RO_CAL1.read_volatile() & !ADC_RO_CAL1_TRM_INIT_MASK) | init_trim,
    );

    // 5. Load initial trim → enable frequency loop → run atomic (one-shot) calibration.
    ADC_RO_CAL0.write_volatile(ADC_RO_CAL0.read_volatile() | ADC_RO_CAL0_RO_CAL_LOAD);
    ADC_RO_CAL0.write_volatile(ADC_RO_CAL0.read_volatile() | ADC_RO_CAL0_RO_CAL_EN);
    ADC_RO_CAL0.write_volatile(ADC_RO_CAL0.read_volatile() | ADC_RO_CAL0_RO_CAL_ATOMIC);

    // 6. Wait for calibration done — bounded to avoid hanging if ADC clock not running.
    let mut wd = 10_000_000_u32;
    while ADC_INTR.read_volatile() & ADC_INTR_RO_CAL_DONE_IF == 0 {
        wd = wd.wrapping_sub(1);
        if wd == 0 {
            break;
        }
    }
    let cal_done = ADC_INTR.read_volatile() & ADC_INTR_RO_CAL_DONE_IF != 0;

    // 7. Stop run, disable interrupt enable.
    ADC_RO_CAL0.write_volatile(ADC_RO_CAL0.read_volatile() & !ADC_RO_CAL0_RO_CAL_RUN);
    ADC_INTR.write_volatile(ADC_INTR.read_volatile() & !ADC_INTR_RO_CAL_DONE_IE);

    // 8. Write calibrated trim to PWRSEQ_REG6 TRIM_OSC_VREF — only if cal completed.
    //    If timed out, INFO-block trim from sys_init() step 2 stays in PWRSEQ_REG6.
    if cal_done {
        let final_trim =
            (ADC_RO_CAL0.read_volatile() & ADC_RO_CAL0_RO_TRM_MASK) >> ADC_RO_CAL0_RO_TRM_POS;
        PWRSEQ_REG6.write_volatile(
            (PWRSEQ_REG6.read_volatile() & !PWRSEQ_REG6_TRIM_OSC_VREF_MASK)
                | ((final_trim << PWRSEQ_REG6_TRIM_OSC_VREF_POS) & PWRSEQ_REG6_TRIM_OSC_VREF_MASK),
        );
    }

    // 9. Restore RTC state.
    if was_running == 0 {
        PWRSEQ_REG0.write_volatile(PWRSEQ_REG0.read_volatile() & !PWRSEQ_REG0_PWR_RTCEN_RUN);
    }

    // 10. Disable frequency control loop.
    ADC_RO_CAL0.write_volatile(ADC_RO_CAL0.read_volatile() & !ADC_RO_CAL0_RO_CAL_EN);
}
