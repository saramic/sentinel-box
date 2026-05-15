import { describe, it, expect, vi, beforeEach } from "vitest"
import { render, act } from "@testing-library/react"
import { useRef } from "react"
import { BleProvider, useSentinelBle } from "./useSentinelBle"
import type { SentinelBleManager } from "./SentinelBleManager"
import { DeviceState } from "./gatt"

// ── Helpers ───────────────────────────────────────────────────────────────────

/** Minimal mock of <sentinel-ble-manager> that supports addEventListener. */
function makeMockManager() {
  const listeners = new Map<string, EventListenerOrEventListenerObject[]>()
  const manager = {
    connect: vi.fn(),
    disconnect: vi.fn(),
    readChar: vi.fn(),
    writeChar: vi.fn(),
    subscribe: vi.fn(),
    addEventListener: vi.fn(
      (event: string, handler: EventListenerOrEventListenerObject) => {
        listeners.set(event, [...(listeners.get(event) ?? []), handler])
      },
    ),
    removeEventListener: vi.fn(),
    dispatchEvent: vi.fn(),
    // Fire a custom event to all registered listeners
    _fire(name: string, detail: unknown) {
      const e = new CustomEvent(name, { detail })
      for (const h of listeners.get(name) ?? []) {
        if (typeof h === "function") h(e)
        else h.handleEvent(e)
      }
    },
  }
  return manager
}

// A simple consumer component that exposes context values via data attributes
function Consumer() {
  const { connected, deviceName, deviceState, lastError } = useSentinelBle()
  return (
    <div
      data-connected={String(connected)}
      data-device-name={deviceName}
      data-device-state={String(deviceState)}
      data-last-error={lastError ?? ""}
    />
  )
}

function Wrapper({ manager }: { manager: ReturnType<typeof makeMockManager> }) {
  const ref = useRef<SentinelBleManager | null>(
    manager as unknown as SentinelBleManager,
  )
  return (
    <BleProvider managerRef={ref}>
      <Consumer />
    </BleProvider>
  )
}

// ── Tests ─────────────────────────────────────────────────────────────────────

describe("BleProvider / useSentinelBle", () => {
  let manager: ReturnType<typeof makeMockManager>

  beforeEach(() => {
    manager = makeMockManager()
  })

  it("starts disconnected with Unconfigured state", () => {
    const { container } = render(<Wrapper manager={manager} />)
    const div = container.firstChild as HTMLElement
    expect(div.dataset.connected).toBe("false")
    expect(div.dataset.deviceState).toBe(String(DeviceState.Unconfigured))
  })

  it("sets connected=true and deviceName on sentinel-connected event", () => {
    const { container } = render(<Wrapper manager={manager} />)
    act(() => {
      manager._fire("sentinel-connected", { deviceName: "SentinelBox" })
    })
    const div = container.firstChild as HTMLElement
    expect(div.dataset.connected).toBe("true")
    expect(div.dataset.deviceName).toBe("SentinelBox")
  })

  it("resets state on sentinel-disconnected event", () => {
    const { container } = render(<Wrapper manager={manager} />)
    act(() => {
      manager._fire("sentinel-connected", { deviceName: "SentinelBox" })
    })
    act(() => {
      manager._fire("sentinel-disconnected", {})
    })
    const div = container.firstChild as HTMLElement
    expect(div.dataset.connected).toBe("false")
    expect(div.dataset.deviceName).toBe("")
    expect(div.dataset.deviceState).toBe(String(DeviceState.Unconfigured))
  })

  it("updates deviceState on sentinel-state-change event", () => {
    const { container } = render(<Wrapper manager={manager} />)
    act(() => {
      manager._fire("sentinel-state-change", { state: DeviceState.ArmedLocked })
    })
    const div = container.firstChild as HTMLElement
    expect(div.dataset.deviceState).toBe(String(DeviceState.ArmedLocked))
  })

  it("records lastError on sentinel-error event", () => {
    const { container } = render(<Wrapper manager={manager} />)
    act(() => {
      manager._fire("sentinel-error", { message: "GATT error" })
    })
    const div = container.firstChild as HTMLElement
    expect(div.dataset.lastError).toBe("GATT error")
  })

  it("throws when used outside BleProvider", () => {
    // Suppress React error boundary noise in test output
    const spy = vi.spyOn(console, "error").mockImplementation(() => {})
    expect(() => render(<Consumer />)).toThrow(
      "useSentinelBle must be used inside <BleProvider>",
    )
    spy.mockRestore()
  })
})
