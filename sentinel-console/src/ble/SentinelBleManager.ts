/**
 * <sentinel-ble-manager> — Lit web component.
 *
 * Owns all Web Bluetooth API interactions. Zero visible UI.
 * React consumers communicate via:
 *   - methods:  connect(), disconnect(), readChar(), writeChar(), subscribe()
 *   - events:   sentinel-connected, sentinel-disconnected,
 *               sentinel-state-change, sentinel-gatt-notify, sentinel-error
 *
 * Mount once in <App> and hold a ref to call methods from the React tree.
 *
 * Example (React):
 *   const ref = useRef<SentinelBleManager>(null)
 *   ref.current?.connect()
 */

import { LitElement } from "lit"
import { SERVICE_UUID, DeviceState, CHAR_DEVICE_STATUS } from "./gatt"

// ── Custom event types ────────────────────────────────────────────────────────

export interface SentinelConnectedEvent {
  deviceName: string
}

export interface SentinelStateChangeEvent {
  state: DeviceState
}

export interface SentinelGattNotifyEvent {
  uuid: number
  value: DataView
}

export interface SentinelErrorEvent {
  message: string
}

// Helper: fire a typed CustomEvent from a LitElement
function emit<T>(el: LitElement, name: string, detail: T): void {
  el.dispatchEvent(
    new CustomEvent(name, { detail, bubbles: true, composed: true }),
  )
}

// ── Component ─────────────────────────────────────────────────────────────────

export class SentinelBleManager extends LitElement {
  // Lit reactive properties — decorator-free API compatible with esbuild.
  static override properties = {
    connected: { type: Boolean, reflect: true },
    deviceName: { type: String },
  }

  // No shadow DOM or template — this component is invisible.
  protected override createRenderRoot() {
    return this
  }

  /** True while a BLE connection is active. */
  declare connected: boolean

  /** Name reported by the BLE device (set after successful connect). */
  declare deviceName: string

  private _device: BluetoothDevice | null = null
  private _service: BluetoothRemoteGATTService | null = null
  private _notifyHandlers = new Map<number, (event: Event) => void>()

  constructor() {
    super()
    // Initialise reactive properties via assignment so Lit's accessor runs
    this.connected = false
    this.deviceName = ""
  }

  // ── Public API ──────────────────────────────────────────────────────────────

  /**
   * Open the browser BLE scan dialog and connect to a SentinelBox device.
   * Fires `sentinel-connected` on success, `sentinel-error` on failure.
   */
  async connect(): Promise<void> {
    try {
      const device = await navigator.bluetooth.requestDevice({
        filters: [{ name: "SentinelBox" }],
        optionalServices: [SERVICE_UUID],
      })

      device.addEventListener("gattserverdisconnected", () =>
        this._handleDisconnect(),
      )

      const server = await device.gatt!.connect()
      const service = await server.getPrimaryService(SERVICE_UUID)

      this._device = device
      this._service = service
      this.connected = true
      this.deviceName = device.name ?? "SentinelBox"

      emit<SentinelConnectedEvent>(this, "sentinel-connected", {
        deviceName: this.deviceName,
      })

      // Auto-subscribe to device status notifications
      await this.subscribe(CHAR_DEVICE_STATUS)
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err)
      // User cancelled the picker — don't surface as an error
      if (message.includes("cancelled") || message.includes("chosen")) return
      emit<SentinelErrorEvent>(this, "sentinel-error", { message })
    }
  }

  /** Disconnect and clean up. */
  async disconnect(): Promise<void> {
    this._teardown()
    this._device?.gatt?.disconnect()
  }

  /**
   * Read a characteristic by its 16-bit UUID.
   * Returns a DataView of the characteristic value, or null on failure.
   */
  async readChar(uuid: number): Promise<DataView | null> {
    try {
      const char = await this._getChar(uuid)
      return await char.readValue()
    } catch (err) {
      this._emitError(err)
      return null
    }
  }

  /**
   * Write a characteristic by its 16-bit UUID.
   * Pass a Uint8Array or a single byte as a number.
   */
  async writeChar(uuid: number, value: Uint8Array | number): Promise<void> {
    try {
      const char = await this._getChar(uuid)
      // Ensure the buffer is always backed by a plain ArrayBuffer (TS6 type strictness)
      const data =
        typeof value === "number"
          ? new Uint8Array([value])
          : new Uint8Array(value)
      await char.writeValueWithResponse(data)
    } catch (err) {
      this._emitError(err)
    }
  }

  /**
   * Subscribe to notifications from a characteristic.
   * Fires `sentinel-gatt-notify` events whenever the device sends a value.
   */
  async subscribe(uuid: number): Promise<void> {
    try {
      const char = await this._getChar(uuid)
      await char.startNotifications()

      const handler = (event: Event) => {
        const target = event.target as BluetoothRemoteGATTCharacteristic
        emit<SentinelGattNotifyEvent>(this, "sentinel-gatt-notify", {
          uuid,
          value: target.value!,
        })
        // Device Status notifications update the state-change event
        if (uuid === CHAR_DEVICE_STATUS && target.value) {
          emit<SentinelStateChangeEvent>(this, "sentinel-state-change", {
            state: target.value.getUint8(0) as DeviceState,
          })
        }
      }

      char.addEventListener("characteristicvaluechanged", handler)
      this._notifyHandlers.set(uuid, handler)
    } catch (err) {
      this._emitError(err)
    }
  }

  // ── Private helpers ─────────────────────────────────────────────────────────

  private async _getChar(
    uuid: number,
  ): Promise<BluetoothRemoteGATTCharacteristic> {
    if (!this._service) throw new Error("Not connected")
    return this._service.getCharacteristic(uuid)
  }

  private _handleDisconnect(): void {
    this._teardown()
    this.dispatchEvent(
      new CustomEvent("sentinel-disconnected", {
        bubbles: true,
        composed: true,
      }),
    )
  }

  private _teardown(): void {
    this._notifyHandlers.clear()
    this._service = null
    this._device = null
    this.connected = false
    this.deviceName = ""
  }

  private _emitError(err: unknown): void {
    const message = err instanceof Error ? err.message : String(err)
    emit<SentinelErrorEvent>(this, "sentinel-error", { message })
  }
}

// Register the custom element (replaces the @customElement decorator)
customElements.define("sentinel-ble-manager", SentinelBleManager)

// Teach TypeScript about the custom element in JSX / querySelector results
declare global {
  interface HTMLElementTagNameMap {
    "sentinel-ble-manager": SentinelBleManager
  }
}
