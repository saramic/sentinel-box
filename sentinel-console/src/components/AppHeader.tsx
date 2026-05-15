import { Badge } from "@/components/ui/badge"
import { Separator } from "@/components/ui/separator"
import { useSentinelBle } from "@/ble/useSentinelBle"
import { DEVICE_STATE_INFO } from "@/ble/gatt"

export function AppHeader() {
  const { connected, deviceName, deviceState } = useSentinelBle()
  const stateInfo = DEVICE_STATE_INFO[deviceState]

  return (
    <header className="sticky top-0 z-50 border-b bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/60">
      <div className="flex h-14 items-center gap-4 px-6">
        {/* Logo / title */}
        <div className="flex items-center gap-2 font-semibold">
          <span className="text-lg">🔒</span>
          <span>SentinelBox Console</span>
        </div>

        <Separator orientation="vertical" className="h-5" />

        {/* Connection status */}
        {connected ? (
          <div className="flex items-center gap-2">
            <span
              className="h-2 w-2 rounded-full"
              style={{ backgroundColor: stateInfo.color }}
              aria-hidden="true"
            />
            <Badge variant="outline" className="font-mono text-xs">
              {deviceName}
            </Badge>
            <span className="text-xs text-muted-foreground">
              {stateInfo.label}
            </span>
          </div>
        ) : (
          <Badge variant="secondary" className="text-xs">
            Not connected
          </Badge>
        )}
      </div>
    </header>
  )
}
