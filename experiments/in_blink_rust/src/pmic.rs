// MAX14690 PMIC init via I2CM2 on Map A (P3.4=SDA, P3.5=SCL).
//
// Derived from mbed-os TARGET_MAX32630FTHR/low_level_init.c — the authoritative
// reference for this board's PMIC boot sequence. That file also confirms:
//   - I2CM2 (not I2CM1) with Map A
//   - PMIC address 0x50 (= 0x28 << 1, 7-bit addr 0x28)
//   - Only LDO2_VSET (0x15) and LDO2_CFG (0x14) are strictly needed at boot
//
// Do NOT write the TIMEOUT register. With AUTO_STOP_EN=1 and TX_TIMEOUT=0xFF
// the hardware stalls ~750 ms per failed transaction (4 writes = ~3 s delay).

// CLKMAN — base 0x4000_0400
const CLKMAN_SYS_CLK_CTRL_9_I2CM: *mut u32 = 0x4000_0464 as *mut u32; // offset 0x0064
const CLKMAN_I2C_TIMER_CTRL: *mut u32 = 0x4000_0414 as *mut u32; // offset 0x0014

// IOMAN — base 0x4000_0C00
const IOMAN_I2CM2_REQ: *mut u32 = 0x4000_0C60 as *mut u32; // offset 0x0060

// I2CM2 registers — base 0x4001_8000
const I2CM2_FS_CLK_DIV: *mut u32 = 0x4001_8000 as *mut u32; // offset 0x0000
const I2CM2_CTRL: *mut u32 = 0x4001_8010 as *mut u32; // offset 0x0010
const I2CM2_TRANS: *mut u32 = 0x4001_8014 as *mut u32; // offset 0x0014
const I2CM2_INTFL: *mut u32 = 0x4001_8018 as *mut u32; // offset 0x0018
const I2CM2_INTEN: *mut u32 = 0x4001_801C as *mut u32; // offset 0x001C

// I2CM2 TX FIFO — base 0x4010_9000 (16-bit tagged entries)
const I2CM2_FIFO_TX: *mut u16 = 0x4010_9000 as *mut u16;

// fs_clk_div values at 96 MHz. Source: LPSDK i2cm.c clk_div_table.
// Fields: FILTER_CLK_DIV[7:0]=48, SCL_LO_CNT[19:8], SCL_HI_CNT[31:20]
// 100 kHz values from LPSDK; 50 kHz doubles the SCL counts.
// Use 50 kHz if the board's pull-up resistors (>4.7 kΩ) cause NACKs at 100 kHz.
const FS_CLK_DIV_100KHZ_96MHZ: u32 = (48_u32) | ( 576_u32 << 8) | (164_u32 << 20);
const FS_CLK_DIV_50KHZ_96MHZ:  u32 = (48_u32) | (1152_u32 << 8) | (328_u32 << 20);

// FIFO transaction tags (i2cm_regs.h MXC_S_I2CM_TRANS_TAG_*)
const TAG_START: u16 = 0x000;
const TAG_TXDATA_ACK: u16 = 0x100;
const TAG_STOP: u16 = 0x700;

// MAX14690 PMIC — 7-bit address 0x28, 8-bit write address 0x50
const PMIC_WRITE_ADDR: u16 = 0x50;
// LDO2_VSET: (3300 - 800) / 100 = 25 = 0x19
const LDO2_3300MV: u8 = 0x19;
const LDO_ENABLED: u8 = 0x02;

pub fn init() {
    unsafe { pmic_init() }
}

unsafe fn pmic_init() {
    // Enable I2CM clock at full system speed (DIV_1)
    CLKMAN_SYS_CLK_CTRL_9_I2CM.write_volatile(1);
    CLKMAN_I2C_TIMER_CTRL.write_volatile(1);

    // Request I2CM2 pin mapping — Map A: io_sel=0 at bits[1:0], mapping_req=1 at bit[4]
    IOMAN_I2CM2_REQ.write_volatile(0x10);

    // Configure I2CM2 at 100 kHz
    I2CM2_FS_CLK_DIV.write_volatile(FS_CLK_DIV_50KHZ_96MHZ);
    I2CM2_CTRL.write_volatile(0x80); // MSTR_RESET_EN: reset peripheral
    I2CM2_CTRL.write_volatile(0x00); // release reset
    I2CM2_CTRL.write_volatile(0x0C); // TX_FIFO_EN(bit2) | RX_FIFO_EN(bit3)

    // Disable and clear interrupts (matches mbed low_level_init.c)
    I2CM2_INTEN.write_volatile(0);
    I2CM2_INTFL.write_volatile(I2CM2_INTFL.read_volatile());

    // LDO2 to 3.3V, enabled — minimum needed for board to run standalone.
    // (mbed only writes LDO2; LDO3 omitted here to match the reference exactly.)
    pmic_write(0x15, LDO2_3300MV); // LDO2_VSET
    pmic_write(0x14, LDO_ENABLED); // LDO2_CFG
}

// Write one [reg, val] pair to the MAX14690.
// Sequence mirrors mbed-os low_level_init.c exactly:
//   load START → kick TX_START → load data + STOP → poll TX_IN_PROGRESS.
unsafe fn pmic_write(reg: u8, val: u8) {
    I2CM2_FIFO_TX.write_volatile(TAG_START | PMIC_WRITE_ADDR);
    I2CM2_TRANS.write_volatile(I2CM2_TRANS.read_volatile() | 0x01); // TX_START
    I2CM2_FIFO_TX.write_volatile(TAG_TXDATA_ACK | reg as u16);
    I2CM2_FIFO_TX.write_volatile(TAG_TXDATA_ACK | val as u16);
    I2CM2_FIFO_TX.write_volatile(TAG_STOP);

    // TX_IN_PROGRESS (bit 1) clears when the transaction finishes or errors.
    // Bound the wait so a dead bus doesn't hang forever.
    let mut timeout = 100_000_u32;
    while I2CM2_TRANS.read_volatile() & 0x02 != 0 && timeout > 0 {
        timeout -= 1;
    }
}
