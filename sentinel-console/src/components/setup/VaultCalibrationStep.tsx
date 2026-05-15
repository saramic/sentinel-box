import { useState } from "react"
import { Slider } from "@/components/ui/slider"
import { Button } from "@/components/ui/button"
import { Card, CardContent } from "@/components/ui/card"
import { useSentinelBle } from "@/ble/useSentinelBle"
import { CHAR_VAULT_STEPS, CHAR_SETUP_CMD, SetupCmd } from "@/ble/gatt"
import type { SentinelConfig } from "@/ble/config-codec"

interface Props {
  config: SentinelConfig
  onChange: (cfg: SentinelConfig) => void
}

export function VaultCalibrationStep({ config, onChange }: Props) {
  const { writeChar } = useSentinelBle()
  const [testing, setTesting] = useState(false)
  const [error, setError] = useState<string | null>(null)

  async function handleTest() {
    setTesting(true)
    setError(null)
    try {
      const buf = new Uint8Array(2)
      new DataView(buf.buffer).setUint16(0, config.vaultSteps, true)
      await writeChar(CHAR_VAULT_STEPS, buf)
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
          Set the number of stepper motor steps from the home (unlocked)
          position to fully locked. Press "Run Test" to move the motor — adjust
          until the lock arm reaches its hard stop cleanly.
        </p>
      </div>

      <Card>
        <CardContent className="pt-6 space-y-4">
          <div className="flex items-center justify-between">
            <span className="text-sm font-medium">Vault steps</span>
            <span className="text-2xl font-mono font-bold tabular-nums">
              {config.vaultSteps}
            </span>
          </div>
          <Slider
            min={50}
            max={2000}
            step={10}
            value={[config.vaultSteps]}
            onValueChange={([v]) => onChange({ ...config, vaultSteps: v })}
            className="w-full"
          />
          <div className="flex justify-between text-xs text-muted-foreground">
            <span>50 — short travel</span>
            <span>2000 — full rotation</span>
          </div>
        </CardContent>
      </Card>

      <Button onClick={handleTest} disabled={testing}>
        {testing ? "Running…" : "Run Test (motor will move)"}
      </Button>

      {error && <p className="text-sm text-destructive">{error}</p>}
    </div>
  )
}
