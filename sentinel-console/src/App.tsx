// Register the Lit web component before React renders
import "./ble/SentinelBleManager"
import { useRef } from "react"
import { BleProvider, useSentinelBle } from "./ble/useSentinelBle"
import type { SentinelBleManager } from "./ble/SentinelBleManager"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import { AppHeader } from "@/components/AppHeader"
import { ConnectTab } from "@/components/ConnectTab"
import { SetupTab } from "@/components/SetupTab"
import { DebugTab } from "@/components/DebugTab"

function AppShell() {
  const { connected } = useSentinelBle()

  return (
    <div className="flex min-h-screen flex-col">
      <AppHeader />

      <main className="flex-1 px-6 py-6">
        <Tabs defaultValue="connect" className="w-full">
          <TabsList className="mb-6">
            <TabsTrigger value="connect">Connect</TabsTrigger>
            <TabsTrigger value="setup" disabled={!connected}>
              Setup
            </TabsTrigger>
            <TabsTrigger value="debug" disabled={!connected}>
              Debug
            </TabsTrigger>
          </TabsList>

          <TabsContent value="connect">
            <ConnectTab />
          </TabsContent>

          <TabsContent value="setup">
            <SetupTab />
          </TabsContent>

          <TabsContent value="debug">
            <DebugTab />
          </TabsContent>
        </Tabs>
      </main>
    </div>
  )
}

function App() {
  const bleRef = useRef<SentinelBleManager | null>(null)

  return (
    <BleProvider managerRef={bleRef}>
      {/* Invisible Lit element — owns all Web Bluetooth I/O */}
      <sentinel-ble-manager ref={bleRef} />
      <AppShell />
    </BleProvider>
  )
}

export default App
