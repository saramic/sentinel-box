import { useState } from "react"
import { Progress } from "@/components/ui/progress"
import { Button } from "@/components/ui/button"
import { Separator } from "@/components/ui/separator"
import { DEFAULT_CONFIG, type SentinelConfig } from "@/ble/config-codec"
import { VaultCalibrationStep } from "./VaultCalibrationStep"
import { FingerprintStep } from "./FingerprintStep"
import { PolicyStep } from "./PolicyStep"
import { ConfirmStep } from "./ConfirmStep"

const STEPS = ["Vault Calibration", "Fingerprints", "Policy", "Confirm & Write"]

export function SetupWizard() {
  const [step, setStep] = useState(0)
  const [config, setConfig] = useState<SentinelConfig>(DEFAULT_CONFIG)

  return (
    <div className="space-y-6">
      {/* Step indicators + progress bar */}
      <div className="space-y-2">
        <div className="flex justify-between">
          {STEPS.map((label, i) => (
            <span
              key={label}
              className={`text-xs ${
                i === step
                  ? "text-foreground font-medium"
                  : "text-muted-foreground"
              }`}
            >
              {i + 1}. {label}
            </span>
          ))}
        </div>
        <Progress value={((step + 1) / STEPS.length) * 100} />
      </div>

      <Separator />

      {/* Active step */}
      {step === 0 && (
        <VaultCalibrationStep config={config} onChange={setConfig} />
      )}
      {step === 1 && <FingerprintStep config={config} onChange={setConfig} />}
      {step === 2 && <PolicyStep config={config} onChange={setConfig} />}
      {step === 3 && <ConfirmStep config={config} />}

      <Separator />

      {/* Back / Next navigation */}
      <div className="flex justify-between">
        <Button
          variant="outline"
          onClick={() => setStep((s) => s - 1)}
          disabled={step === 0}
        >
          Back
        </Button>
        {step < STEPS.length - 1 && (
          <Button onClick={() => setStep((s) => s + 1)}>Next</Button>
        )}
      </div>
    </div>
  )
}
