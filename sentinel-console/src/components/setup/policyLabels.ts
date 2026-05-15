import { UnlockPolicy } from "@/ble/gatt"

export const POLICY_LABELS: Record<UnlockPolicy, string> = {
  [UnlockPolicy.AnyFinger]: "Any enrolled finger",
  [UnlockPolicy.OneAdult]: "One adult finger",
  [UnlockPolicy.OneAdultOneChild]: "One adult + one child finger",
  [UnlockPolicy.TwoAdults]: "Two different adult fingers",
}
