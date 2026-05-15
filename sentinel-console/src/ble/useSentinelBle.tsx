/**
 * useSentinelBle — React hook + context for the <sentinel-ble-manager> element.
 *
 * Usage:
 *   // Mount once at the root (App.tsx):
 *   <sentinel-ble-manager ref={bleRef} />
 *   <BleProvider managerRef={bleRef}>{children}</BleProvider>
 *
 *   // Anywhere in the tree:
 *   const { connected, deviceName, deviceState, connect, disconnect,
 *           readChar, writeChar } = useSentinelBle()
 */

import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useRef,
  useState,
  type RefObject,
} from "react"
import type { SentinelBleManager } from "./SentinelBleManager"
import type {
  SentinelConnectedEvent,
  SentinelStateChangeEvent,
  SentinelErrorEvent,
} from "./SentinelBleManager"
import { DeviceState } from "./gatt"

// ── Context value type ────────────────────────────────────────────────────────

export interface BleContextValue {
  connected: boolean
  deviceName: string
  deviceState: DeviceState
  lastError: string | null
  connect: (acceptAll?: boolean) => Promise<void>
  disconnect: () => Promise<void>
  readChar: (uuid: number) => Promise<DataView | null>
  writeChar: (uuid: number, value: Uint8Array | number) => Promise<void>
  subscribe: (uuid: number) => Promise<void>
}

// ── Context ───────────────────────────────────────────────────────────────────

const BleContext = createContext<BleContextValue | null>(null)

// ── Provider ──────────────────────────────────────────────────────────────────

interface BleProviderProps {
  managerRef: RefObject<SentinelBleManager | null>
  children: React.ReactNode
}

export function BleProvider({ managerRef, children }: BleProviderProps) {
  const [connected, setConnected] = useState(false)
  const [deviceName, setDeviceName] = useState("")
  const [deviceState, setDeviceState] = useState<DeviceState>(
    DeviceState.Unconfigured,
  )
  const [lastError, setLastError] = useState<string | null>(null)

  useEffect(() => {
    const el = managerRef.current
    if (!el) return

    const onConnected = (e: Event) => {
      const detail = (e as CustomEvent<SentinelConnectedEvent>).detail
      setConnected(true)
      setDeviceName(detail.deviceName)
      setLastError(null)
    }

    const onDisconnected = () => {
      setConnected(false)
      setDeviceName("")
      setDeviceState(DeviceState.Unconfigured)
    }

    const onStateChange = (e: Event) => {
      const detail = (e as CustomEvent<SentinelStateChangeEvent>).detail
      setDeviceState(detail.state)
    }

    const onError = (e: Event) => {
      const detail = (e as CustomEvent<SentinelErrorEvent>).detail
      setLastError(detail.message)
    }

    el.addEventListener("sentinel-connected", onConnected)
    el.addEventListener("sentinel-disconnected", onDisconnected)
    el.addEventListener("sentinel-state-change", onStateChange)
    el.addEventListener("sentinel-error", onError)

    return () => {
      el.removeEventListener("sentinel-connected", onConnected)
      el.removeEventListener("sentinel-disconnected", onDisconnected)
      el.removeEventListener("sentinel-state-change", onStateChange)
      el.removeEventListener("sentinel-error", onError)
    }
  }, [managerRef])

  const connect = useCallback(
    (acceptAll?: boolean) =>
      managerRef.current?.connect(acceptAll) ?? Promise.resolve(),
    [managerRef],
  )

  const disconnect = useCallback(
    () => managerRef.current?.disconnect() ?? Promise.resolve(),
    [managerRef],
  )

  const readChar = useCallback(
    (uuid: number) =>
      managerRef.current?.readChar(uuid) ?? Promise.resolve(null),
    [managerRef],
  )

  const writeChar = useCallback(
    (uuid: number, value: Uint8Array | number) =>
      managerRef.current?.writeChar(uuid, value) ?? Promise.resolve(),
    [managerRef],
  )

  const subscribe = useCallback(
    (uuid: number) => managerRef.current?.subscribe(uuid) ?? Promise.resolve(),
    [managerRef],
  )

  return (
    <BleContext.Provider
      value={{
        connected,
        deviceName,
        deviceState,
        lastError,
        connect,
        disconnect,
        readChar,
        writeChar,
        subscribe,
      }}
    >
      {children}
    </BleContext.Provider>
  )
}

// ── Hook ──────────────────────────────────────────────────────────────────────

export function useSentinelBle(): BleContextValue {
  const ctx = useContext(BleContext)
  if (!ctx) {
    throw new Error("useSentinelBle must be used inside <BleProvider>")
  }
  return ctx
}

// ── Convenience: create the manager ref (call once in App) ────────────────────

export function useBleManagerRef() {
  return useRef<SentinelBleManager | null>(null)
}
