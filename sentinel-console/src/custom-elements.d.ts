/**
 * Teach React's JSX type checker about our Lit custom elements.
 *
 * Without this, TypeScript raises "Property 'sentinel-ble-manager' does not
 * exist on type 'JSX.IntrinsicElements'".
 *
 * We add the elements to React's IntrinsicElements via module augmentation.
 * The ref type matches the class registered via @customElement.
 */
import type { SentinelBleManager } from "./ble/SentinelBleManager"

declare module "react" {
  namespace JSX {
    interface IntrinsicElements {
      "sentinel-ble-manager": React.DetailedHTMLProps<
        React.HTMLAttributes<SentinelBleManager>,
        SentinelBleManager
      > & { ref?: React.Ref<SentinelBleManager> }
    }
  }
}
