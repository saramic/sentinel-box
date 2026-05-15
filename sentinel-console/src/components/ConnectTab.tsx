import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card"

export function ConnectTab() {
  return (
    <Card>
      <CardHeader>
        <CardTitle>Connect to SentinelBox</CardTitle>
        <CardDescription>
          Pair with a nearby SentinelBox device over Bluetooth.
        </CardDescription>
      </CardHeader>
      <CardContent>
        <p className="text-sm text-muted-foreground">
          Connection panel — coming in Phase 4.
        </p>
      </CardContent>
    </Card>
  )
}
