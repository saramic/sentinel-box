import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card"

export function DebugTab() {
  return (
    <Card>
      <CardHeader>
        <CardTitle>Debug</CardTitle>
        <CardDescription>
          Inspect GATT characteristics, override the LED, and view the BLE event
          log.
        </CardDescription>
      </CardHeader>
      <CardContent>
        <p className="text-sm text-muted-foreground">
          GATT explorer + terminal — coming in Phase 6.
        </p>
      </CardContent>
    </Card>
  )
}
