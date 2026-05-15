/**
 * Encode and decode the SentinelBox INFO flash config block.
 *
 * Layout (100 bytes total):
 *
 * Offset  Size  Field
 * ──────  ────  ─────────────────────────────────────────────────────
 * 0x00    2     magic         0x5B5A ("SB" little-endian)
 * 0x02    2     vault_steps   u16 LE — steps from home to locked
 * 0x04    1     fp_count      number of enrolled slots (0–10)
 * 0x05    1     unlock_policy UnlockPolicy enum
 * 0x06    1     relock_minutes 0 = never
 * 0x07    1     reserved      (zero pad)
 * 0x08    80    fp_name[10]   8 bytes each, null-padded ASCII
 * 0x58    10    fp_role[10]   1 byte each: FpRole enum
 * 0x62    2     crc16         CRC-16/CCITT over bytes 0x00–0x61
 */

import { crc16 } from "@/lib/crc16"
import { FpRole, UnlockPolicy } from "./gatt"

export const CONFIG_MAGIC = 0x5b5a
export const CONFIG_SIZE = 100 // bytes
const FP_SLOTS = 10
const FP_NAME_LEN = 8
const CRC_OFFSET = 0x62 // 98

export interface FpSlot {
  name: string // max 8 ASCII chars; empty string = slot unused
  role: FpRole
  enrolled: boolean
}

export interface SentinelConfig {
  vaultSteps: number // u16
  unlockPolicy: UnlockPolicy
  relockMinutes: number // 0 = never
  fingerprints: FpSlot[] // exactly 10 entries
}

export const DEFAULT_CONFIG: SentinelConfig = {
  vaultSteps: 300,
  unlockPolicy: UnlockPolicy.AnyFinger,
  relockMinutes: 10,
  fingerprints: Array.from({ length: FP_SLOTS }, () => ({
    name: "",
    role: FpRole.Adult,
    enrolled: false,
  })),
}

const encoder = new TextEncoder()
const decoder = new TextDecoder("ascii")

/** Encode a SentinelConfig into the 100-byte wire/flash format. */
export function encodeConfig(cfg: SentinelConfig): Uint8Array {
  const buf = new Uint8Array(CONFIG_SIZE)
  const view = new DataView(buf.buffer)

  // magic
  view.setUint16(0x00, CONFIG_MAGIC, true)
  // vault_steps
  view.setUint16(0x02, cfg.vaultSteps, true)
  // fp_count: number of slots where enrolled === true
  const enrolledCount = cfg.fingerprints.filter((fp) => fp.enrolled).length
  buf[0x04] = enrolledCount
  // unlock_policy
  buf[0x05] = cfg.unlockPolicy
  // relock_minutes
  buf[0x06] = cfg.relockMinutes
  // reserved — already 0

  // fp_name[10]: 8 bytes each starting at 0x08
  for (let i = 0; i < FP_SLOTS; i++) {
    const nameBytes = encoder.encode(
      cfg.fingerprints[i].name.slice(0, FP_NAME_LEN),
    )
    buf.set(nameBytes, 0x08 + i * FP_NAME_LEN)
    // remaining bytes in the slot stay 0 (null pad)
  }

  // fp_role[10]: 1 byte each starting at 0x58
  for (let i = 0; i < FP_SLOTS; i++) {
    buf[0x58 + i] = cfg.fingerprints[i].role
  }

  // CRC over bytes 0x00–0x61 (98 bytes)
  const crc = crc16(buf.slice(0, CRC_OFFSET))
  view.setUint16(CRC_OFFSET, crc, true)

  return buf
}

/** Decode a 100-byte buffer into a SentinelConfig, or null if invalid. */
export function decodeConfig(buf: Uint8Array): SentinelConfig | null {
  if (buf.length < CONFIG_SIZE) return null

  const view = new DataView(buf.buffer, buf.byteOffset)

  // Verify magic
  if (view.getUint16(0x00, true) !== CONFIG_MAGIC) return null

  // Verify CRC
  const storedCrc = view.getUint16(CRC_OFFSET, true)
  const computedCrc = crc16(buf.slice(0, CRC_OFFSET))
  if (storedCrc !== computedCrc) return null

  const fingerprints: FpSlot[] = []
  for (let i = 0; i < FP_SLOTS; i++) {
    const nameSlice = buf.slice(
      0x08 + i * FP_NAME_LEN,
      0x08 + (i + 1) * FP_NAME_LEN,
    )
    // Trim null bytes
    const nullIdx = nameSlice.indexOf(0)
    const nameBytes = nullIdx === -1 ? nameSlice : nameSlice.slice(0, nullIdx)
    const name = decoder.decode(nameBytes)
    const role: FpRole =
      buf[0x58 + i] === FpRole.Child ? FpRole.Child : FpRole.Adult
    fingerprints.push({ name, role, enrolled: name.length > 0 })
  }

  return {
    vaultSteps: view.getUint16(0x02, true),
    unlockPolicy: buf[0x05] as UnlockPolicy,
    relockMinutes: buf[0x06],
    fingerprints,
  }
}
