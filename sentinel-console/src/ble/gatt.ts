/**
 * GATT service and characteristic UUIDs for the SentinelBox BLE server.
 *
 * SERVICE_UUID is currently 0xF001 to match the in_btstack experiment firmware.
 * The 16-bit short UUIDs are promoted to full 128-bit Bluetooth base UUIDs
 * by the Web Bluetooth API automatically when passed as numbers.
 */

// ── Service ──────────────────────────────────────────────────────────────────

/** Custom SentinelBox GATT service (0xF001 in current firmware) */
export const SERVICE_UUID = 0xf001

// ── Characteristic UUIDs ─────────────────────────────────────────────────────

/** Device status — Read + Notify. 1 byte: DeviceState enum value. */
export const CHAR_DEVICE_STATUS = 0xf001

/** LED override — Read + Write. 1 byte: LedColor enum value (debug only). */
export const CHAR_LED_OVERRIDE = 0xf002

/** Setup command — Write. 1 byte: SetupCmd enum value. */
export const CHAR_SETUP_CMD = 0xf010

/** Vault steps — Read + Write. u16 little-endian: steps from home to locked. */
export const CHAR_VAULT_STEPS = 0xf011

/** Fingerprint slot — Read + Write. 1 byte: slot index 0–9. */
export const CHAR_FP_SLOT = 0xf012

/**
 * Fingerprint metadata — Read + Write.
 * 9 bytes: 8-byte name (null-padded ASCII) + 1-byte role (FpRole enum).
 */
export const CHAR_FP_METADATA = 0xf013

/**
 * Config blob — Write.
 * Full serialised config (100 bytes including CRC-16) matching INFO flash layout.
 */
export const CHAR_CONFIG_BLOB = 0xf014

// ── Value objects (replaces enum — compatible with erasableSyntaxOnly) ────────
//
// Pattern: `as const` object + type alias extracted from it.
// Usage is identical to enum: DeviceState.Unconfigured, etc.

/** Firmware device state — value reported on CHAR_DEVICE_STATUS. */
export const DeviceState = {
  Unconfigured: 0x00,
  BleAdvertising: 0x01,
  BleConnectedSetup: 0x02,
  SetupVaultTune: 0x03,
  SetupFingerprintEnrol: 0x04,
  ArmedLocked: 0x10,
  AwaitingAuth: 0x11,
  AuthGrantedOpening: 0x12,
  VaultOpen: 0x13,
  AuthFailed: 0x14,
  GuardianOverride: 0x15,
  Error: 0xff,
} as const
export type DeviceState = (typeof DeviceState)[keyof typeof DeviceState]

/** Human-readable label and LED colour hint for each device state. */
export const DEVICE_STATE_INFO: Record<
  DeviceState,
  { label: string; color: string; description: string }
> = {
  [DeviceState.Unconfigured]: {
    label: "Unconfigured",
    color: "#3b82f6",
    description: "Fresh firmware — no config stored",
  },
  [DeviceState.BleAdvertising]: {
    label: "Advertising",
    color: "#3b82f6",
    description: "Waiting for setup connection",
  },
  [DeviceState.BleConnectedSetup]: {
    label: "Setup",
    color: "#06b6d4",
    description: "Browser connected — wizard active",
  },
  [DeviceState.SetupVaultTune]: {
    label: "Vault Tune",
    color: "#eab308",
    description: "Calibrating stepper motor travel",
  },
  [DeviceState.SetupFingerprintEnrol]: {
    label: "FP Enrol",
    color: "#a855f7",
    description: "Enrolling fingerprints",
  },
  [DeviceState.ArmedLocked]: {
    label: "Armed & Locked",
    color: "#22c55e",
    description: "Secured — ready for use",
  },
  [DeviceState.AwaitingAuth]: {
    label: "Awaiting Auth",
    color: "#f59e0b",
    description: "Fingerprint scan requested",
  },
  [DeviceState.AuthGrantedOpening]: {
    label: "Opening",
    color: "#22c55e",
    description: "Valid scan — motor running",
  },
  [DeviceState.VaultOpen]: {
    label: "Vault Open",
    color: "#22c55e",
    description: "Relock timer counting down",
  },
  [DeviceState.AuthFailed]: {
    label: "Auth Failed",
    color: "#ef4444",
    description: "No match — returning to Armed",
  },
  [DeviceState.GuardianOverride]: {
    label: "Admin Override",
    color: "#f8fafc",
    description: "Manual unlock — no auth",
  },
  [DeviceState.Error]: {
    label: "Error",
    color: "#ef4444",
    description: "System fault — check hardware",
  },
}

/** LED colour byte written to CHAR_LED_OVERRIDE. */
export const LedColor = {
  Off: 0,
  Red: 1,
  Green: 2,
  Blue: 3,
  Cyan: 4,
  White: 5,
} as const
export type LedColor = (typeof LedColor)[keyof typeof LedColor]

/** Setup command byte written to CHAR_SETUP_CMD. */
export const SetupCmd = {
  StartVaultTune: 0x01,
  StartFpEnrol: 0x02,
  StartPolicy: 0x03,
  WriteConfig: 0x04,
} as const
export type SetupCmd = (typeof SetupCmd)[keyof typeof SetupCmd]

/** Fingerprint role byte stored in CHAR_FP_METADATA and config blob. */
export const FpRole = {
  Adult: 0,
  Child: 1,
} as const
export type FpRole = (typeof FpRole)[keyof typeof FpRole]

/** Unlock policy byte in config blob. */
export const UnlockPolicy = {
  AnyFinger: 0,
  OneAdult: 1,
  OneAdultOneChild: 2,
  TwoAdults: 3,
} as const
export type UnlockPolicy = (typeof UnlockPolicy)[keyof typeof UnlockPolicy]
