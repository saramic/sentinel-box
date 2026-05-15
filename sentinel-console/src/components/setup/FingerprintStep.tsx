import { useState } from "react"
import { Card, CardContent } from "@/components/ui/card"
import { Input } from "@/components/ui/input"
import { Button } from "@/components/ui/button"
import { Badge } from "@/components/ui/badge"
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select"
import { useSentinelBle } from "@/ble/useSentinelBle"
import {
  CHAR_FP_SLOT,
  CHAR_FP_METADATA,
  CHAR_SETUP_CMD,
  SetupCmd,
  FpRole,
} from "@/ble/gatt"
import type { SentinelConfig, FpSlot } from "@/ble/config-codec"

interface Props {
  config: SentinelConfig
  onChange: (cfg: SentinelConfig) => void
}

export function FingerprintStep({ config, onChange }: Props) {
  const { writeChar } = useSentinelBle()
  const [enrollingSlot, setEnrollingSlot] = useState<number | null>(null)
  const [error, setError] = useState<string | null>(null)

  function updateSlot(i: number, patch: Partial<FpSlot>) {
    const fps = config.fingerprints.map((fp, idx) =>
      idx === i ? { ...fp, ...patch } : fp,
    )
    onChange({ ...config, fingerprints: fps })
  }

  async function handleEnrol(i: number) {
    setEnrollingSlot(i)
    setError(null)
    try {
      const fp = config.fingerprints[i]
      await writeChar(CHAR_FP_SLOT, i)
      const meta = new Uint8Array(9)
      const nameBytes = new TextEncoder().encode(fp.name.slice(0, 8))
      meta.set(nameBytes, 0)
      meta[8] = fp.role
      await writeChar(CHAR_FP_METADATA, meta)
      await writeChar(CHAR_SETUP_CMD, SetupCmd.StartFpEnrol)
      updateSlot(i, { enrolled: true })
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e))
    } finally {
      setEnrollingSlot(null)
    }
  }

  function handleClear(i: number) {
    updateSlot(i, { name: "", enrolled: false, role: FpRole.Adult })
  }

  const enrolledCount = config.fingerprints.filter((fp) => fp.enrolled).length

  return (
    <div className="space-y-6">
      <div>
        <h3 className="text-lg font-medium">Fingerprint Enrolment</h3>
        <p className="text-sm text-muted-foreground mt-1">
          Enrol up to 10 fingerprints. Give each a name and role, then press
          Enrol — the device will prompt for finger placement.{" "}
          <span className="font-medium">{enrolledCount} / 10 enrolled.</span>
        </p>
      </div>

      <div className="grid grid-cols-2 gap-3">
        {config.fingerprints.map((fp, i) => (
          <Card key={i} className={fp.enrolled ? "border-green-500/40" : ""}>
            <CardContent className="pt-4 space-y-3">
              <div className="flex items-center justify-between">
                <span className="text-sm font-medium text-muted-foreground">
                  Slot {i + 1}
                </span>
                {fp.enrolled ? (
                  <Badge
                    variant="outline"
                    className="text-green-500 border-green-500/50"
                  >
                    enrolled
                  </Badge>
                ) : (
                  <Badge variant="outline">empty</Badge>
                )}
              </div>

              <Input
                placeholder="Name (max 8 chars)"
                value={fp.name}
                onChange={(e) => updateSlot(i, { name: e.target.value })}
                maxLength={8}
                className="h-8 text-sm"
              />

              <Select
                value={String(fp.role)}
                onValueChange={(v) =>
                  updateSlot(i, { role: Number(v) as FpRole })
                }
              >
                <SelectTrigger className="h-8 text-sm">
                  <SelectValue />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value={String(FpRole.Adult)}>Adult</SelectItem>
                  <SelectItem value={String(FpRole.Child)}>Child</SelectItem>
                </SelectContent>
              </Select>

              <div className="flex gap-2">
                <Button
                  size="sm"
                  className="flex-1"
                  onClick={() => handleEnrol(i)}
                  disabled={enrollingSlot !== null}
                >
                  {enrollingSlot === i ? "Enrolling…" : "Enrol"}
                </Button>
                {fp.enrolled && (
                  <Button
                    size="sm"
                    variant="outline"
                    onClick={() => handleClear(i)}
                    disabled={enrollingSlot !== null}
                  >
                    Clear
                  </Button>
                )}
              </div>
            </CardContent>
          </Card>
        ))}
      </div>

      {error && <p className="text-sm text-destructive">{error}</p>}
    </div>
  )
}
