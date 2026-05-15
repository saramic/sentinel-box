import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { Badge } from "@/components/ui/badge"
import { useSentinelBle } from "@/ble/useSentinelBle"
import { DEVICE_STATE_INFO } from "@/ble/gatt"

export function DeviceStateCard() {
  const { connected, deviceState } = useSentinelBle()
  const info = DEVICE_STATE_INFO[deviceState]

  if (!connected) return null

  return (
    <Card>
      <CardHeader className="pb-2">
        <CardTitle className="text-sm font-medium text-muted-foreground">
          Device State
        </CardTitle>
      </CardHeader>
      <CardContent className="flex items-center gap-3">
        {/* Live colour dot matching firmware LED */}
        <span
          className="h-3 w-3 rounded-full flex-shrink-0"
          style={{ backgroundColor: info.color }}
          aria-hidden="true"
        />
        <div className="flex flex-col gap-0.5">
          <Badge variant="outline" className="w-fit font-mono text-xs">
            {info.label}
          </Badge>
          <p className="text-xs text-muted-foreground">{info.description}</p>
        </div>
      </CardContent>
    </Card>
  )
}
