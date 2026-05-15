/**
 * CRC-16/CCITT-FALSE — polynomial 0x1021, init 0xFFFF, no reflection.
 *
 * Matches the firmware's CRC check over the INFO flash config block.
 * See README § Persistent Config Layout.
 */
export function crc16(data: Uint8Array): number {
  let crc = 0xffff
  for (const byte of data) {
    crc ^= byte << 8
    for (let i = 0; i < 8; i++) {
      crc = crc & 0x8000 ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff
    }
  }
  return crc
}
