// BMI160 6-axis IMU on I2CM2 (P5.7 = SDA, P6.0 = SCL), I2C address 0x68 (SDO low).
//
// P5.7/P6.0 map to I2C_2 = I2CM2 per PeripheralPins.c — the same bus and peripheral
// as the PMIC (MAX14690 at 0x28).  pmic::init() must run first; it initialises
// I2CM2 (clock, IOMAN, FS_CLK_DIV, CTRL).  This driver only adds BMI160 transactions
// on top of the already-running peripheral.
//
// Register addresses from reference/MAX32630FTHR_balance_bot_code/justin-jordan-BMI160/bmi160.h

// I2CM2 registers — base 0x4001_8000 (same as pmic.rs)
const I2CM2_CTRL:  *mut u32 = 0x4001_8010 as *mut u32;
const I2CM2_TRANS: *mut u32 = 0x4001_8014 as *mut u32;
const I2CM2_INTFL: *mut u32 = 0x4001_8018 as *mut u32;

// I2CM2 FIFOs — TX base 0x4010_9000, RX base 0x4010_9800
const I2CM2_FIFO_TX: *mut u16   = 0x4010_9000 as *mut u16;
const I2CM2_FIFO_RX: *const u16 = 0x4010_9800 as *const u16;

// I2CM FIFO transaction tags
const TAG_START:        u16 = 0x000;
const TAG_TXDATA_ACK:   u16 = 0x100;
const TAG_RXDATA_COUNT: u16 = 0x400;
const TAG_RXDATA_LAST:  u16 = 0x500;
const TAG_STOP:         u16 = 0x700;

// BMI160 — 7-bit address 0x68 (SDO tied low on FTHR)
const BMI160_WRITE: u16 = 0xD0; // 0x68 << 1
const BMI160_READ:  u16 = 0xD1; // 0x68 << 1 | 1

// BMI160 register addresses
const REG_CMD:       u8 = 0x7E;
const REG_ACC_X_LSB: u8 = 0x12; // 6 bytes: X_L X_H Y_L Y_H Z_L Z_H

// BMI160 power mode commands (CMD register)
const CMD_ACC_NORMAL: u8 = 0x11;

/// No I2CM hardware init needed — pmic::init() already set up I2CM2.
pub fn hw_init() {}

/// Send CMD_ACC_NORMAL to wake the accelerometer from suspend mode.
pub fn acc_init() {
    unsafe {
        i2cm2_write(REG_CMD, CMD_ACC_NORMAL);
        cortex_m::asm::delay(480_000); // ~5 ms startup per BMI160 datasheet
    }
}

pub fn init() {
    acc_init();
}

/// Return raw INTFL register — call immediately after a transaction, before the next one.
/// Bit 0 = TX_DONE, bit 1 = TX_NACK_ERR.  Expect 0x01 on success.
pub fn read_intfl() -> u32 {
    unsafe { I2CM2_INTFL.read_volatile() }
}

/// Read CHIP_ID register (0x00). Should return 0xD1 if I2C is working.
pub fn read_chip_id() -> u8 {
    unsafe {
        let mut buf = [0u8; 1];
        i2cm2_read_bytes(0x00, &mut buf);
        buf[0]
    }
}

/// Read all three acceleration axes in a single 6-byte burst.
/// Returns (ax, ay, az): signed 16-bit values, ±16384 LSB ≈ ±1 g at default ±2 g range.
pub fn read_accel_xyz() -> (i16, i16, i16) {
    unsafe {
        let mut buf = [0u8; 6];
        i2cm2_read_bytes(REG_ACC_X_LSB, &mut buf);
        // buf layout: [X_L, X_H, Y_L, Y_H, Z_L, Z_H]
        (
            i16::from_le_bytes([buf[0], buf[1]]),
            i16::from_le_bytes([buf[2], buf[3]]),
            i16::from_le_bytes([buf[4], buf[5]]),
        )
    }
}

/// Flush TX and RX FIFOs by toggling CTRL reset. Call before every transaction.
unsafe fn i2cm2_flush() {
    I2CM2_CTRL.write_volatile(0x80); // MSTR_RESET_EN — clears FIFOs
    I2CM2_CTRL.write_volatile(0x00);
    I2CM2_CTRL.write_volatile(0x0C); // TX_FIFO_EN | RX_FIFO_EN
    I2CM2_INTFL.write_volatile(I2CM2_INTFL.read_volatile());
}

unsafe fn i2cm2_write(reg: u8, val: u8) {
    i2cm2_flush();
    I2CM2_FIFO_TX.write_volatile(TAG_START | BMI160_WRITE);
    I2CM2_TRANS.write_volatile(I2CM2_TRANS.read_volatile() | 0x01); // TX_START
    I2CM2_FIFO_TX.write_volatile(TAG_TXDATA_ACK | reg as u16);
    I2CM2_FIFO_TX.write_volatile(TAG_TXDATA_ACK | val as u16);
    I2CM2_FIFO_TX.write_volatile(TAG_STOP);

    let mut timeout = 100_000_u32;
    while I2CM2_TRANS.read_volatile() & 0x02 != 0 && timeout > 0 {
        timeout -= 1;
    }
}

unsafe fn i2cm2_read_bytes(reg: u8, buf: &mut [u8]) {
    let n = buf.len() as u16;

    i2cm2_flush();

    // Kick TX_START right after TAG_START, then stream the rest — matches pmic_write
    // and the LPSDK i2cm.c pattern (kick early so hardware starts while we fill FIFO).
    I2CM2_FIFO_TX.write_volatile(TAG_START | BMI160_WRITE);
    I2CM2_TRANS.write_volatile(I2CM2_TRANS.read_volatile() | 0x01); // TX_START
    I2CM2_FIFO_TX.write_volatile(TAG_TXDATA_ACK | reg as u16);
    I2CM2_FIFO_TX.write_volatile(TAG_START | BMI160_READ);
    if n > 1 {
        I2CM2_FIFO_TX.write_volatile(TAG_RXDATA_COUNT | (n - 1)); // n-1 bytes with ACK
    }
    I2CM2_FIFO_TX.write_volatile(TAG_RXDATA_LAST); // final byte with NACK
    I2CM2_FIFO_TX.write_volatile(TAG_STOP);

    let mut timeout = 100_000_u32;
    while I2CM2_TRANS.read_volatile() & 0x02 != 0 && timeout > 0 {
        timeout -= 1;
    }

    for byte in buf.iter_mut() {
        *byte = (I2CM2_FIFO_RX.read_volatile() & 0xFF) as u8;
    }
}
