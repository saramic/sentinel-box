import { LedOverrideCard } from "./debug/LedOverrideCard"
import { GattExplorer } from "./debug/GattExplorer"
import { SentinelTerminal } from "./debug/SentinelTerminal"

export function DebugTab() {
  return (
    <div className="space-y-4">
      <LedOverrideCard />
      <GattExplorer />
      <SentinelTerminal />
    </div>
  )
}
