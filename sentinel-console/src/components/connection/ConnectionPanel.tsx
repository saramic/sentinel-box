import { useState } from "react"
import { Bluetooth, BluetoothOff, Loader2 } from "lucide-react"
import { Button } from "@/components/ui/button"
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card"
import { Alert, AlertDescription } from "@/components/ui/alert"
import { useSentinelBle } from "@/ble/useSentinelBle"

export function ConnectionPanel() {
  const { connected, deviceName, lastError, connect, disconnect } =
    useSentinelBle()
  const [scanning, setScanning] = useState(false)

  async function handleConnect(acceptAll: boolean) {
    setScanning(true)
    try {
      await connect(acceptAll)
    } finally {
      setScanning(false)
    }
  }

  return (
    <Card>
      <CardHeader>
        <CardTitle>Connect to SentinelBox</CardTitle>
        <CardDescription>
          {connected
            ? `Connected to ${deviceName}`
            : "Pair with a nearby SentinelBox device over Bluetooth."}
        </CardDescription>
      </CardHeader>
      <CardContent className="flex flex-col gap-4">
        {!connected ? (
          <div className="flex gap-2">
            <Button
              onClick={() => handleConnect(false)}
              disabled={scanning}
              className="w-fit"
            >
              {scanning ? (
                <>
                  <Loader2 className="mr-2 h-4 w-4 animate-spin" />
                  Scanning…
                </>
              ) : (
                <>
                  <Bluetooth className="mr-2 h-4 w-4" />
                  Pair SentinelBox
                </>
              )}
            </Button>
            <Button
              variant="outline"
              onClick={() => handleConnect(true)}
              disabled={scanning}
              className="w-fit"
            >
              <Bluetooth className="mr-2 h-4 w-4" />
              Show All Devices
            </Button>
          </div>
        ) : (
          <Button
            variant="destructive"
            onClick={() => disconnect()}
            className="w-fit"
          >
            <BluetoothOff className="mr-2 h-4 w-4" />
            Disconnect
          </Button>
        )}

        {lastError && (
          <Alert variant="destructive">
            <AlertDescription>{lastError}</AlertDescription>
          </Alert>
        )}

        {!navigator.bluetooth && (
          <Alert>
            <AlertDescription>
              Web Bluetooth is not available. Use Chrome or Edge on desktop, and
              ensure the page is served over HTTPS or localhost.
            </AlertDescription>
          </Alert>
        )}
      </CardContent>
    </Card>
  )
}
