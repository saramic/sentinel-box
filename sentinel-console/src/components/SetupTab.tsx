import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card"

export function SetupTab() {
  return (
    <Card>
      <CardHeader>
        <CardTitle>Device Setup</CardTitle>
        <CardDescription>
          Calibrate the vault motor, enrol fingerprints, and set access policy.
        </CardDescription>
      </CardHeader>
      <CardContent>
        <p className="text-sm text-muted-foreground">
          Setup wizard — coming in Phase 5.
        </p>
      </CardContent>
    </Card>
  )
}
