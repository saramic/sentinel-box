import { useState } from "react"
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { Button } from "@/components/ui/button"
import { Input } from "@/components/ui/input"
import { Badge } from "@/components/ui/badge"
import { useSentinelBle } from "@/ble/useSentinelBle"
import {
  CHAR_DEVICE_STATUS,
  CHAR_LED_OVERRIDE,
  CHAR_SETUP_CMD,
  CHAR_VAULT_STEPS,
  CHAR_FP_SLOT,
  CHAR_FP_METADATA,
  CHAR_CONFIG_BLOB,
  LedColor,
} from "@/ble/gatt"

interface CharDef {
  uuid: number
  name: string
  props: Array<"read" | "write" | "notify">
}

const CHARS: CharDef[] = [
  {
    uuid: CHAR_DEVICE_STATUS,
    name: "Device Status",
    props: ["read", "notify"],
  },
  { uuid: CHAR_LED_OVERRIDE, name: "LED Override", props: ["read", "write"] },
  { uuid: CHAR_SETUP_CMD, name: "Setup Command", props: ["write"] },
  { uuid: CHAR_VAULT_STEPS, name: "Vault Steps", props: ["read", "write"] },
  { uuid: CHAR_FP_SLOT, name: "FP Slot", props: ["read", "write"] },
  { uuid: CHAR_FP_METADATA, name: "FP Metadata", props: ["read", "write"] },
  { uuid: CHAR_CONFIG_BLOB, name: "Config Blob", props: ["write"] },
]

function hexStr(uuid: number) {
  return "0x" + uuid.toString(16).toUpperCase().padStart(4, "0")
}

function dataViewToHex(dv: DataView): string {
  return Array.from({ length: dv.byteLength }, (_, i) =>
    dv.getUint8(i).toString(16).padStart(2, "0").toUpperCase(),
  ).join(" ")
}

function parseHexInput(s: string): Uint8Array | null {
  const clean = s.trim().replace(/\s+/g, "")
  if (!clean || clean.length % 2 !== 0) return null
  try {
    const bytes = new Uint8Array(clean.length / 2)
    for (let i = 0; i < clean.length; i += 2)
      bytes[i / 2] = parseInt(clean.slice(i, i + 2), 16)
    return bytes
  } catch {
    return null
  }
}

function CharRow({ def }: { def: CharDef }) {
  const { connected, readChar, writeChar } = useSentinelBle()
  const [value, setValue] = useState<string | null>(null)
  const [writeInput, setWriteInput] = useState("")
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState<string | null>(null)

  async function handleRead() {
    if (!connected || busy) return
    setError(null)
    setBusy(true)
    const dv = await readChar(def.uuid)
    if (dv) setValue(dataViewToHex(dv))
    else setError("read failed")
    setBusy(false)
  }

  async function handleWrite() {
    if (!connected || busy) return
    setError(null)
    const bytes = parseHexInput(writeInput)
    if (!bytes) {
      setError("invalid hex")
      return
    }
    setBusy(true)
    await writeChar(def.uuid, bytes)
    setWriteInput("")
    setBusy(false)
  }

  return (
    <div className="py-2 border-b last:border-0">
      <div className="flex items-center gap-2 mb-1">
        <span className="font-mono text-xs text-muted-foreground">
          {hexStr(def.uuid)}
        </span>
        <span className="text-sm font-medium">{def.name}</span>
        <div className="flex gap-1 ml-auto">
          {def.props.map((p) => (
            <Badge key={p} variant="outline" className="text-xs py-0">
              {p}
            </Badge>
          ))}
        </div>
      </div>

      <div className="flex items-center gap-2 flex-wrap">
        {def.props.includes("read") && (
          <Button
            size="sm"
            variant="outline"
            className="h-7 text-xs"
            disabled={!connected || busy}
            onClick={handleRead}
          >
            Read
          </Button>
        )}
        {value !== null && (
          <span className="font-mono text-xs bg-muted px-2 py-0.5 rounded">
            {value}
          </span>
        )}
        {def.props.includes("write") && (
          <>
            <Input
              className="h-7 text-xs font-mono w-44"
              placeholder="hex bytes e.g. 01 02"
              value={writeInput}
              onChange={(e) => setWriteInput(e.target.value)}
              onKeyDown={(e) => e.key === "Enter" && handleWrite()}
              disabled={!connected || busy}
            />
            <Button
              size="sm"
              className="h-7 text-xs"
              disabled={!connected || busy || !writeInput.trim()}
              onClick={handleWrite}
            >
              Write
            </Button>
          </>
        )}
        {error && <span className="text-xs text-destructive">{error}</span>}
      </div>
      {/* LED quick-write shortcuts for the LED Override characteristic */}
      {def.uuid === CHAR_LED_OVERRIDE && (
        <div className="flex gap-1 mt-1 flex-wrap">
          {(
            [
              {
                label: "Off",
                color: LedColor.Off,
                cls: "bg-zinc-700 hover:bg-zinc-600 text-zinc-200",
              },
              {
                label: "Red",
                color: LedColor.Red,
                cls: "bg-red-700 hover:bg-red-600 text-white",
              },
              {
                label: "Green",
                color: LedColor.Green,
                cls: "bg-green-700 hover:bg-green-600 text-white",
              },
              {
                label: "Blue",
                color: LedColor.Blue,
                cls: "bg-blue-700 hover:bg-blue-600 text-white",
              },
            ] as Array<{ label: string; color: LedColor; cls: string }>
          ).map(({ label, color, cls }) => (
            <button
              key={label}
              disabled={!connected || busy}
              onClick={async () => {
                setBusy(true)
                await writeChar(def.uuid, color)
                setBusy(false)
              }}
              className={`px-3 py-0.5 rounded text-xs font-medium transition-opacity disabled:opacity-40 disabled:cursor-not-allowed ${cls}`}
            >
              {label}
            </button>
          ))}
        </div>
      )}
    </div>
  )
}

export function GattExplorer() {
  const { connected } = useSentinelBle()

  return (
    <Card>
      <CardHeader className="pb-3">
        <CardTitle className="text-sm flex items-center gap-2">
          GATT Explorer
          {!connected && (
            <Badge variant="secondary" className="text-xs">
              disconnected
            </Badge>
          )}
        </CardTitle>
      </CardHeader>
      <CardContent className="px-4 pb-4">
        {CHARS.map((def) => (
          <CharRow key={def.uuid} def={def} />
        ))}
      </CardContent>
    </Card>
  )
}
