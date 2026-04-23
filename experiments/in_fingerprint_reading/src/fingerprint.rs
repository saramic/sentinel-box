// Adafruit fingerprint sensor (AS608 / R305 / R307) driver.
//
// Protocol reference: https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library
// Packet format (all multi-byte fields are big-endian):
//
//   [0xEF][0x01]            — header (2 bytes)
//   [addr: 4 bytes]         — module address, factory default 0xFFFFFFFF
//   [pid: 1 byte]           — 0x01=command, 0x02=data, 0x07=ack, 0x08=end_data
//   [length: 2 bytes]       — byte count of (data + checksum), i.e. len(data) + 2
//   [data: N bytes]         — instruction code + parameters
//   [checksum: 2 bytes]     — sum of pid + length_hi + length_lo + data bytes
//
// TODO list (implement in order):
//   1. send_packet / recv_packet  — raw framing layer
//   2. verify_password            — handshake to confirm sensor is alive (pw=0x00000000)
//   3. get_image                  — capture finger image (0x01)
//   4. image_to_tz(slot)          — convert image to feature template in slot 1 or 2 (0x02)
//   5. finger_search              — search all stored templates (0x04)
//   6. store_model(id)            — store matched template to flash at id (0x06)
//   7. empty_database             — wipe all stored fingerprints (0x0D)

use crate::uart;

// Default module address (broadcast).
const ADDR: [u8; 4] = [0xFF, 0xFF, 0xFF, 0xFF];
const HEADER: [u8; 2] = [0xEF, 0x01];

const PID_CMD: u8 = 0x01;
const PID_ACK: u8 = 0x07;

// Instruction codes
pub const CMD_VERIFY_PASSWORD: u8 = 0x13;
pub const CMD_GET_IMAGE:       u8 = 0x01;
pub const CMD_IMAGE_TO_TZ:     u8 = 0x02;
pub const CMD_SEARCH:          u8 = 0x04;
pub const CMD_STORE:           u8 = 0x06;
pub const CMD_EMPTY_DB:        u8 = 0x0D;

// Confirmation codes returned by the sensor in the ACK packet
pub const OK:             u8 = 0x00;
pub const ERR_NO_FINGER:  u8 = 0x02;
pub const ERR_IMAGE_FAIL: u8 = 0x03;
pub const ERR_NO_MATCH:   u8 = 0x09;
pub const ERR_NOT_FOUND:  u8 = 0x0A;

/// Send a command packet with the given data bytes.
pub fn send_packet(data: &[u8]) {
    let pid = PID_CMD;
    let length = (data.len() + 2) as u16; // data + 2 checksum bytes

    let mut checksum: u16 = pid as u16
        + (length >> 8) as u16
        + (length & 0xFF) as u16;
    for &b in data {
        checksum += b as u16;
    }

    uart::write_all(&HEADER);
    uart::write_all(&ADDR);
    uart::write_byte(pid);
    uart::write_byte((length >> 8) as u8);
    uart::write_byte((length & 0xFF) as u8);
    uart::write_all(data);
    uart::write_byte((checksum >> 8) as u8);
    uart::write_byte((checksum & 0xFF) as u8);
}

/// Read an ACK packet. Returns the confirmation code byte, or 0xFF on timeout/framing error.
pub fn recv_ack() -> u8 {
    // Skip until we see 0xEF 0x01 header.
    loop {
        match uart::read_byte() {
            Some(0xEF) => {}
            None => return 0xFF,
            _ => continue,
        }
        match uart::read_byte() {
            Some(0x01) => break,
            None => return 0xFF,
            _ => continue,
        }
    }

    // Skip 4-byte address, 1-byte PID.
    for _ in 0..5 {
        if uart::read_byte().is_none() {
            return 0xFF;
        }
    }

    // Read 2-byte length.
    let len_hi = match uart::read_byte() { Some(b) => b, None => return 0xFF };
    let len_lo = match uart::read_byte() { Some(b) => b, None => return 0xFF };
    let payload_len = ((len_hi as u16) << 8 | len_lo as u16) as usize;

    if payload_len < 2 {
        return 0xFF;
    }

    // First data byte is the confirmation code.
    let confirm = match uart::read_byte() { Some(b) => b, None => return 0xFF };

    // Drain remaining payload bytes (including checksum) so the FIFO stays clean.
    for _ in 1..payload_len {
        uart::read_byte();
    }

    confirm
}

/// Verify the sensor password (factory default 0x00000000). Returns OK on success.
pub fn verify_password() -> u8 {
    send_packet(&[CMD_VERIFY_PASSWORD, 0x00, 0x00, 0x00, 0x00]);
    recv_ack()
}

/// Tell the sensor to capture a finger image. Returns OK when a finger is detected.
pub fn get_image() -> u8 {
    send_packet(&[CMD_GET_IMAGE]);
    recv_ack()
}

/// Convert the captured image to a feature template in buffer slot (1 or 2).
pub fn image_to_tz(slot: u8) -> u8 {
    send_packet(&[CMD_IMAGE_TO_TZ, slot]);
    recv_ack()
}

/// Search all stored templates. Returns OK and fills `*found_id` on a match.
pub fn finger_search(found_id: &mut u16) -> u8 {
    // Search slot 1, library pages 0–162.
    send_packet(&[CMD_SEARCH, 0x01, 0x00, 0x00, 0x00, 0xA3]);
    let code = recv_ack();
    // TODO: parse the 2-byte finger_id and 2-byte match_score from the ack payload.
    *found_id = 0;
    code
}
