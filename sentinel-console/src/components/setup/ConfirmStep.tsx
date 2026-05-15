import { useState } from "react"
import { Button } from "@/components/ui/button"
import { Card, CardContent } from "@/components/ui/card"
import { Badge } from "@/components/ui/badge"
import { Separator } from "@/components/ui/separator"
import { useSentinelBle } from "@/ble/useSentinelBle"
import { CHAR_CONFIG_BLOB } from "@/ble/gatt"
import { encodeConfig, type SentinelConfig } from "@/ble/config-codec"
import { POLICY_LABELS } from "./policyLabels"

function relockLabel(minutes: number): string {
  if (minutes === 0) return "Never"
  if (minutes < 60) return `${minutes} min`
  return `${minutes / 60} hr`
}

export function ConfirmStep({ config }: { config: SentinelConfig }) {
  const { writeChar } = useSentinelBle()
  const [writing, setWriting] = useState(false)
  const [written, setWritten] = useState(false)
  const [error, setError] = useState<string | null>(null)

  async function handleWrite() {
    setWriting(true)
    setError(null)
    try {
      await writeChar(CHAR_CONFIG_BLOB, encodeConfig(config))
      setWritten(true)
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e))
    } finally {
      setWriting(false)
    }
  }

  return (
    <div className="space-y-6">
      <div>
        <h3 className="text-lg font-medium">Confirm & Write</h3>
        <p className="text-sm text-muted-foreground mt-1">
          Review your configuration, then write it to the device flash.
        </p>
      </div>

      <Card>
        <CardContent className="pt-6 space-y-3">
          <div className="flex justify-between text-sm">
            <span className="text-muted-foreground">Vault steps</span>
            <span className="font-mono">{config.vaultSteps}</span>
          </div>
          <Separator />
          <div className="flex justify-between text-sm">
            <span className="text-muted-foreground">Unlock policy</span>
            <span>{POLICY_LABELS[config.unlockPolicy] ?? "Unknown"}</span>
          </div>
          <Separator />
          <div className="flex justify-between text-sm">
            <span className="text-muted-foreground">Auto-relock</span>
            <span>{relockLabel(config.relockMinutes)}</span>
          </div>
          <Separator />
          <div className="flex justify-between text-sm">
            <span className="text-muted-foreground">Enrolled fingerprints</span>
            <span>
              {config.fingerprints.filter((fp) => fp.enrolled).length} / 10
            </span>
          </div>
          <div className="flex flex-wrap gap-1 pt-1">
            {config.fingerprints.map((fp, i) =>
              fp.enrolled ? (
                <Badge key={i} variant="secondary">
                  {fp.name || `Slot ${i + 1}`}
                </Badge>
              ) : null,
            )}
          </div>
        </CardContent>
      </Card>

      {written ? (
        <p className="text-sm font-medium text-green-500">
          ✓ Configuration written to device flash.
        </p>
      ) : (
        <Button onClick={handleWrite} disabled={writing} className="w-full">
          {writing ? "Writing…" : "Write Config to Device"}
        </Button>
      )}

      {error && <p className="text-sm text-destructive">{error}</p>}
    </div>
  )
}
