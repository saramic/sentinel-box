// Register the Lit web component before React renders
import "./ble/SentinelBleManager"
import { useRef } from "react"
import { BleProvider } from "./ble/useSentinelBle"
import type { SentinelBleManager } from "./ble/SentinelBleManager"

function App() {
  const bleRef = useRef<SentinelBleManager | null>(null)

  return (
    <BleProvider managerRef={bleRef}>
      {/* Invisible Lit element — owns all Web Bluetooth I/O */}
      <sentinel-ble-manager ref={bleRef} />

      <main className="min-h-screen bg-background p-8">
        <h1 className="text-2xl font-bold mb-4">SentinelBox Console</h1>
        <p className="text-muted-foreground">
          Phase 2 ready — BLE manager mounted. Connect tab coming in Phase 4.
        </p>
      </main>
    </BleProvider>
  )
}

export default App
