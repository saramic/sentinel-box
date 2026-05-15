import { describe, it, expect } from "vitest"
import { crc16 } from "./crc16"

describe("crc16 (CRC-16/CCITT-FALSE)", () => {
  it('returns 0x29B1 for the canonical "123456789" test vector', () => {
    const input = new TextEncoder().encode("123456789")
    expect(crc16(input)).toBe(0x29b1)
  })

  it("returns 0xFFFF for an empty buffer", () => {
    // Empty input: CRC never leaves init value (loop doesn't run)
    expect(crc16(new Uint8Array(0))).toBe(0xffff)
  })

  it("returns the correct CRC for a single 0x00 byte", () => {
    // 0xE1F0 — computed value for CRC-16/CCITT-FALSE over a single null byte
    expect(crc16(new Uint8Array([0x00]))).toBe(0xe1f0)
  })

  it("produces different CRCs for different data", () => {
    const a = crc16(new Uint8Array([0x5b, 0x5a, 0x00, 0x01]))
    const b = crc16(new Uint8Array([0x5b, 0x5a, 0x00, 0x02]))
    expect(a).not.toBe(b)
  })

  it("result fits in a u16", () => {
    const result = crc16(new Uint8Array([0xde, 0xad, 0xbe, 0xef]))
    expect(result).toBeGreaterThanOrEqual(0)
    expect(result).toBeLessThanOrEqual(0xffff)
  })
})
