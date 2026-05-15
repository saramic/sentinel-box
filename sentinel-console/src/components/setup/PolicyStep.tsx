import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select"
import { Card, CardContent } from "@/components/ui/card"
import { UnlockPolicy } from "@/ble/gatt"
import type { SentinelConfig } from "@/ble/config-codec"
import { POLICY_LABELS } from "./policyLabels"

const RELOCK_OPTIONS: Array<{ value: number; label: string }> = [
  { value: 0, label: "Never" },
  { value: 1, label: "1 minute" },
  { value: 5, label: "5 minutes" },
  { value: 10, label: "10 minutes" },
  { value: 30, label: "30 minutes" },
  { value: 60, label: "1 hour" },
  { value: 120, label: "2 hours" },
]

interface Props {
  config: SentinelConfig
  onChange: (cfg: SentinelConfig) => void
}

export function PolicyStep({ config, onChange }: Props) {
  return (
    <div className="space-y-6">
      <div>
        <h3 className="text-lg font-medium">Access Policy</h3>
        <p className="text-sm text-muted-foreground mt-1">
          Configure who can unlock the vault and how long it stays open before
          automatically relocking.
        </p>
      </div>

      <Card>
        <CardContent className="pt-6 space-y-6">
          <div className="space-y-2">
            <p className="text-sm font-medium">Unlock policy</p>
            <Select
              value={String(config.unlockPolicy)}
              onValueChange={(v) =>
                onChange({
                  ...config,
                  unlockPolicy: Number(v) as UnlockPolicy,
                })
              }
            >
              <SelectTrigger>
                <SelectValue />
              </SelectTrigger>
              <SelectContent>
                {(Object.values(UnlockPolicy) as UnlockPolicy[]).map((p) => (
                  <SelectItem key={p} value={String(p)}>
                    {POLICY_LABELS[p]}
                  </SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>

          <div className="space-y-2">
            <p className="text-sm font-medium">Auto-relock after</p>
            <Select
              value={String(config.relockMinutes)}
              onValueChange={(v) =>
                onChange({ ...config, relockMinutes: Number(v) })
              }
            >
              <SelectTrigger>
                <SelectValue />
              </SelectTrigger>
              <SelectContent>
                {RELOCK_OPTIONS.map(({ value, label }) => (
                  <SelectItem key={value} value={String(value)}>
                    {label}
                  </SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>
        </CardContent>
      </Card>
    </div>
  )
}
