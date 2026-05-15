import { useEffect, useRef, useState } from "react"
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { Button } from "@/components/ui/button"
import { ScrollArea } from "@/components/ui/scroll-area"
import type {
  SentinelConnectedEvent,
  SentinelStateChangeEvent,
  SentinelGattNotifyEvent,
  SentinelErrorEvent,
} from "@/ble/SentinelBleManager"
import { DEVICE_STATE_INFO, DeviceState } from "@/ble/gatt"

type LogLevel = "info" | "ok" | "warn" | "error" | "rx"

interface LogEntry {
  id: number
  time: string
  level: LogLevel
  msg: string
}

let _seq = 0
function makeEntry(level: LogLevel, msg: string): LogEntry {
  const now = new Date()
  const time =
    now.toTimeString().slice(0, 8) +
    "." +
    String(now.getMilliseconds()).padStart(3, "0")
  return { id: _seq++, time, level, msg }
}

const LEVEL_CLASS: Record<LogLevel, string> = {
  info: "text-foreground",
  ok: "text-green-400",
  warn: "text-amber-400",
  error: "text-red-400",
  rx: "text-purple-400",
}

function dataViewToHex(dv: DataView): string {
  return Array.from({ length: dv.byteLength }, (_, i) =>
    dv.getUint8(i).toString(16).padStart(2, "0").toUpperCase(),
  ).join(" ")
}

export function SentinelTerminal() {
  const [entries, setEntries] = useState<LogEntry[]>([
    makeEntry("info", "Terminal ready — connect a device to see events"),
  ])
  const bottomRef = useRef<HTMLDivElement>(null)

  function append(level: LogLevel, msg: string) {
    setEntries((prev) => [...prev.slice(-199), makeEntry(level, msg)])
  }

  // Listen to BLE events bubbled from <sentinel-ble-manager>
  useEffect(() => {
    const root = document.documentElement

    function onConnected(e: Event) {
      const { deviceName } = (e as CustomEvent<SentinelConnectedEvent>).detail
      append("ok", `Connected to "${deviceName}"`)
    }

    function onDisconnected() {
      append("warn", "Disconnected")
    }

    function onStateChange(e: Event) {
      const { state } = (e as CustomEvent<SentinelStateChangeEvent>).detail
      const info = DEVICE_STATE_INFO[state as DeviceState]
      const label = info?.label ?? `0x${state.toString(16).padStart(2, "0")}`
      append("info", `State → ${label}`)
    }

    function onNotify(e: Event) {
      const { uuid, value } = (e as CustomEvent<SentinelGattNotifyEvent>).detail
      const hex = dataViewToHex(value)
      append(
        "rx",
        `Notify 0x${uuid.toString(16).toUpperCase().padStart(4, "0")}: ${hex}`,
      )
    }

    function onError(e: Event) {
      const { message } = (e as CustomEvent<SentinelErrorEvent>).detail
      append("error", `Error: ${message}`)
    }

    root.addEventListener("sentinel-connected", onConnected)
    root.addEventListener("sentinel-disconnected", onDisconnected)
    root.addEventListener("sentinel-state-change", onStateChange)
    root.addEventListener("sentinel-gatt-notify", onNotify)
    root.addEventListener("sentinel-error", onError)

    return () => {
      root.removeEventListener("sentinel-connected", onConnected)
      root.removeEventListener("sentinel-disconnected", onDisconnected)
      root.removeEventListener("sentinel-state-change", onStateChange)
      root.removeEventListener("sentinel-gatt-notify", onNotify)
      root.removeEventListener("sentinel-error", onError)
    }
  }, [])

  // Auto-scroll to bottom on new entries
  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: "smooth" })
  }, [entries])

  return (
    <Card>
      <CardHeader className="pb-3">
        <CardTitle className="text-sm flex items-center justify-between">
          <span>BLE Terminal</span>
          <Button
            size="sm"
            variant="ghost"
            className="h-6 text-xs text-muted-foreground"
            onClick={() => setEntries([makeEntry("info", "Log cleared")])}
          >
            Clear
          </Button>
        </CardTitle>
      </CardHeader>
      <CardContent className="p-0">
        <ScrollArea className="h-52 px-4 pb-3">
          <div className="font-mono text-xs space-y-0.5">
            {entries.map((e) => (
              <div key={e.id} className={LEVEL_CLASS[e.level]}>
                <span className="text-muted-foreground mr-2">{e.time}</span>
                {e.msg}
              </div>
            ))}
            <div ref={bottomRef} />
          </div>
        </ScrollArea>
      </CardContent>
    </Card>
  )
}
