import { ConnectionPanel } from "@/components/connection/ConnectionPanel"
import { DeviceStateCard } from "@/components/connection/DeviceStateCard"

export function ConnectTab() {
  return (
    <div className="flex flex-col gap-4 max-w-lg">
      <ConnectionPanel />
      <DeviceStateCard />
    </div>
  )
}
