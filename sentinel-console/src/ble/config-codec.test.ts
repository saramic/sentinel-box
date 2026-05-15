import { describe, it, expect } from "vitest"
import {
  encodeConfig,
  decodeConfig,
  DEFAULT_CONFIG,
  CONFIG_MAGIC,
  CONFIG_SIZE,
} from "./config-codec"
import { FpRole, UnlockPolicy } from "./gatt"

describe("config-codec", () => {
  describe("encodeConfig", () => {
    it("produces a buffer of exactly CONFIG_SIZE bytes", () => {
      expect(encodeConfig(DEFAULT_CONFIG).length).toBe(CONFIG_SIZE)
    })

    it("writes the magic bytes at offset 0", () => {
      const buf = encodeConfig(DEFAULT_CONFIG)
      const view = new DataView(buf.buffer)
      expect(view.getUint16(0, true)).toBe(CONFIG_MAGIC)
    })

    it("encodes vault_steps at offset 0x02 little-endian", () => {
      const buf = encodeConfig({ ...DEFAULT_CONFIG, vaultSteps: 0x0142 })
      expect(buf[0x02]).toBe(0x42)
      expect(buf[0x03]).toBe(0x01)
    })

    it("encodes fp_name with null padding", () => {
      const cfg = {
        ...DEFAULT_CONFIG,
        fingerprints: DEFAULT_CONFIG.fingerprints.map((fp, i) =>
          i === 0 ? { ...fp, name: "Alice", enrolled: true } : fp,
        ),
      }
      const buf = encodeConfig(cfg)
      // "Alice" = 0x41 0x6c 0x69 0x63 0x65 at offset 0x08–0x0c
      expect(buf[0x08]).toBe("A".charCodeAt(0)) // 0x41
      expect(buf[0x09]).toBe("l".charCodeAt(0)) // 0x6c
      expect(buf[0x0c]).toBe("e".charCodeAt(0)) // 0x65 — last char of Alice
      expect(buf[0x0d]).toBe(0) // null pad after "Alice"
    })

    it("sets fp_count to number of enrolled slots", () => {
      const cfg = {
        ...DEFAULT_CONFIG,
        fingerprints: DEFAULT_CONFIG.fingerprints.map((fp, i) =>
          i < 3 ? { ...fp, enrolled: true, name: `FP${i}` } : fp,
        ),
      }
      expect(encodeConfig(cfg)[0x04]).toBe(3)
    })
  })

  describe("decodeConfig", () => {
    it("round-trips DEFAULT_CONFIG", () => {
      const buf = encodeConfig(DEFAULT_CONFIG)
      const decoded = decodeConfig(buf)
      expect(decoded).not.toBeNull()
      expect(decoded!.vaultSteps).toBe(DEFAULT_CONFIG.vaultSteps)
      expect(decoded!.unlockPolicy).toBe(DEFAULT_CONFIG.unlockPolicy)
      expect(decoded!.relockMinutes).toBe(DEFAULT_CONFIG.relockMinutes)
    })

    it("round-trips fingerprint names and roles", () => {
      const cfg = {
        ...DEFAULT_CONFIG,
        fingerprints: DEFAULT_CONFIG.fingerprints.map((fp, i) =>
          i === 0
            ? { name: "Bob", role: FpRole.Adult, enrolled: true }
            : i === 1
              ? { name: "Zara", role: FpRole.Child, enrolled: true }
              : fp,
        ),
      }
      const decoded = decodeConfig(encodeConfig(cfg))!
      expect(decoded.fingerprints[0].name).toBe("Bob")
      expect(decoded.fingerprints[0].role).toBe(FpRole.Adult)
      expect(decoded.fingerprints[1].name).toBe("Zara")
      expect(decoded.fingerprints[1].role).toBe(FpRole.Child)
    })

    it("returns null when magic is wrong", () => {
      const buf = encodeConfig(DEFAULT_CONFIG)
      buf[0] = 0x00 // corrupt magic
      expect(decodeConfig(buf)).toBeNull()
    })

    it("returns null when CRC is wrong", () => {
      const buf = encodeConfig(DEFAULT_CONFIG)
      buf[0x62] ^= 0xff // corrupt CRC
      expect(decodeConfig(buf)).toBeNull()
    })

    it("returns null for a buffer shorter than CONFIG_SIZE", () => {
      expect(decodeConfig(new Uint8Array(10))).toBeNull()
    })

    it("round-trips all UnlockPolicy values", () => {
      for (const policy of [
        UnlockPolicy.AnyFinger,
        UnlockPolicy.OneAdult,
        UnlockPolicy.OneAdultOneChild,
        UnlockPolicy.TwoAdults,
      ]) {
        const cfg = { ...DEFAULT_CONFIG, unlockPolicy: policy }
        expect(decodeConfig(encodeConfig(cfg))!.unlockPolicy).toBe(policy)
      }
    })
  })
})
