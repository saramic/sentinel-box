import { useState } from "react"
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { Badge } from "@/components/ui/badge"
import { useSentinelBle } from "@/ble/useSentinelBle"
import { CHAR_LED_OVERRIDE, LedColor } from "@/ble/gatt"

const LED_BUTTONS: Array<{ label: string; value: LedColor; style: string }> = [
  {
    label: "Off",
    value: LedColor.Off,
    style: "bg-zinc-800 text-zinc-300 hover:bg-zinc-700 border border-zinc-600",
  },
  {
    label: "Red",
    value: LedColor.Red,
    style: "bg-red-700 text-white hover:bg-red-600",
  },
  {
    label: "Green",
    value: LedColor.Green,
    style: "bg-green-700 text-white hover:bg-green-600",
  },
  {
    label: "Blue",
    value: LedColor.Blue,
    style: "bg-blue-700 text-white hover:bg-blue-600",
  },
]

export function LedOverrideCard() {
  const { connected, writeChar } = useSentinelBle()
  const [active, setActive] = useState<LedColor | null>(null)
  const [busy, setBusy] = useState(false)

  async function handleClick(color: LedColor) {
    if (!connected || busy) return
    setBusy(true)
    await writeChar(CHAR_LED_OVERRIDE, color)
    setActive(color)
    setBusy(false)
  }

  return (
    <Card>
      <CardHeader className="pb-3">
        <CardTitle className="text-sm flex items-center gap-2">
          LED Override
          {!connected && (
            <Badge variant="secondary" className="text-xs">
              disconnected
            </Badge>
          )}
        </CardTitle>
      </CardHeader>
      <CardContent>
        <div className="flex flex-wrap gap-2">
          {LED_BUTTONS.map(({ label, value, style }) => (
            <button
              key={label}
              disabled={!connected || busy}
              onClick={() => handleClick(value)}
              className={`px-4 py-2 rounded-md text-sm font-medium transition-opacity disabled:opacity-40 disabled:cursor-not-allowed ${style} ${
                active === value
                  ? "ring-2 ring-offset-2 ring-offset-background ring-white/50"
                  : ""
              }`}
            >
              {label}
            </button>
          ))}
        </div>
        {active !== null && connected && (
          <p className="mt-2 text-xs text-muted-foreground">
            Last sent:{" "}
            <span className="font-mono">
              {LED_BUTTONS.find((b) => b.value === active)?.label} (0x
              {active.toString(16).padStart(2, "0")})
            </span>
          </p>
        )}
      </CardContent>
    </Card>
  )
}
