import { useState, useEffect } from "react"
import { Slider } from "@/components/ui/slider"
import { Button } from "@/components/ui/button"
import { Switch } from "@/components/ui/switch"
import { Card, CardContent } from "@/components/ui/card"
import { useSentinelBle } from "@/ble/useSentinelBle"
import { CHAR_VAULT_STEPS, CHAR_SETUP_CMD, SetupCmd } from "@/ble/gatt"
import type { SentinelConfig } from "@/ble/config-codec"

const STORAGE_KEY = "sentinel-vault-positions"

interface SavedPositions {
  open: number
  closed: number
}

function loadSaved(): SavedPositions | null {
  try {
    const raw = localStorage.getItem(STORAGE_KEY)
    return raw ? (JSON.parse(raw) as SavedPositions) : null
  } catch {
    return null
  }
}

interface Props {
  config: SentinelConfig
  onChange: (cfg: SentinelConfig) => void
}

export function VaultCalibrationStep({ config, onChange }: Props) {
  const { writeChar, subscribe, readChar } = useSentinelBle()
  const [liveControl, setLiveControl] = useState(false)
  const [openSteps, setOpenSteps] = useState(
    () => loadSaved()?.open ?? 0,
  )
  const [savedPositions, setSavedPositions] = useState<SavedPositions | null>(
    loadSaved,
  )
  const [currentPos, setCurrentPos] = useState<number | null>(null)
  const [testing, setTesting] = useState(false)
  const [error, setError] = useState<string | null>(null)

  // Subscribe to F011 notifications and read initial position when live mode is enabled
  useEffect(() => {
    if (!liveControl) return
    subscribe(CHAR_VAULT_STEPS)
    readChar(CHAR_VAULT_STEPS).then((dv) => {
      if (dv && dv.byteLength >= 2) setCurrentPos(dv.getUint16(0, true))
    })
  }, [liveControl, subscribe, readChar])

  // Track live position from F011 notifications
  useEffect(() => {
    const handler = (e: Event) => {
      const { uuid, value } = (
        e as CustomEvent<{ uuid: number; value: DataView }>
      ).detail
      if (uuid === CHAR_VAULT_STEPS && value.byteLength >= 2)
        setCurrentPos(value.getUint16(0, true))
    }
    document.addEventListener("sentinel-gatt-notify", handler)
    return () => document.removeEventListener("sentinel-gatt-notify", handler)
  }, [])

  async function moveTo(steps: number) {
    const buf = new Uint8Array(2)
    new DataView(buf.buffer).setUint16(0, steps, true)
    await writeChar(CHAR_VAULT_STEPS, buf)
  }

  async function handleOpenChange([v]: number[]) {
    setOpenSteps(v)
    if (liveControl) await moveTo(v)
  }

  async function handleClosedChange([v]: number[]) {
    onChange({ ...config, vaultSteps: v })
    if (liveControl) await moveTo(v)
  }

  function handleSave() {
    const positions: SavedPositions = { open: openSteps, closed: config.vaultSteps }
    localStorage.setItem(STORAGE_KEY, JSON.stringify(positions))
    setSavedPositions(positions)
  }

  async function handleTest() {
    setTesting(true)
    setError(null)
    try {
      await moveTo(config.vaultSteps)
      await writeChar(CHAR_SETUP_CMD, SetupCmd.StartVaultTune)
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e))
    } finally {
      setTesting(false)
    }
  }

  return (
    <div className="space-y-6">
      <div>
        <h3 className="text-lg font-medium">Vault Motor Calibration</h3>
        <p className="text-sm text-muted-foreground mt-1">
          Set the open and closed positions for the stepper motor. Enable Live
          Control to move the motor by dragging the sliders, then save to
          create quick-access Open / Close buttons.
        </p>
      </div>

      {liveControl && (
        <div className="flex items-center justify-between text-sm px-1">
          <span className="text-muted-foreground">Current position</span>
          <span className="font-mono tabular-nums font-medium">
            {currentPos ?? "—"}
          </span>
        </div>
      )}

      <Card>
        <CardContent className="pt-6 space-y-4">
          <div className="flex items-center justify-between">
            <span className="text-sm font-medium">Open position</span>
            <span className="text-xl font-mono font-bold tabular-nums">
              {openSteps}
            </span>
          </div>
          <Slider
            min={0}
            max={6000}
            step={10}
            value={[openSteps]}
            onValueChange={handleOpenChange}
            className="w-full"
          />
        </CardContent>
      </Card>

      <Card>
        <CardContent className="pt-6 space-y-4">
          <div className="flex items-center justify-between">
            <span className="text-sm font-medium">Closed position</span>
            <span className="text-xl font-mono font-bold tabular-nums">
              {config.vaultSteps}
            </span>
          </div>
          <Slider
            min={0}
            max={6000}
            step={10}
            value={[config.vaultSteps]}
            onValueChange={handleClosedChange}
            className="w-full"
          />
        </CardContent>
      </Card>

      <div className="flex items-center justify-between">
        <label className="flex items-center gap-2 cursor-pointer select-none">
          <Switch checked={liveControl} onCheckedChange={setLiveControl} />
          <span className="text-sm font-medium">Live control</span>
        </label>

        <div className="flex gap-2">
          <Button variant="outline" onClick={handleSave}>
            Save positions
          </Button>
          {!liveControl && (
            <Button onClick={handleTest} disabled={testing}>
              {testing ? "Running…" : "Run Test (motor will move)"}
            </Button>
          )}
        </div>
      </div>

      {savedPositions && (
        <Card>
          <CardContent className="pt-4 pb-4">
            <div className="flex items-center justify-between">
              <div className="text-xs text-muted-foreground space-y-0.5">
                <div>Open: <span className="font-mono">{savedPositions.open}</span></div>
                <div>Closed: <span className="font-mono">{savedPositions.closed}</span></div>
              </div>
              <div className="flex gap-2">
                <Button
                  size="sm"
                  variant="outline"
                  onClick={() => moveTo(savedPositions.open)}
                >
                  Open
                </Button>
                <Button
                  size="sm"
                  onClick={() => moveTo(savedPositions.closed)}
                >
                  Close
                </Button>
              </div>
            </div>
          </CardContent>
        </Card>
      )}

      {error && <p className="text-sm text-destructive">{error}</p>}
    </div>
  )
}
