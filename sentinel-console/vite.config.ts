import path from "path"
import { defineConfig } from "vite"
import react, { reactCompilerPreset } from "@vitejs/plugin-react"
import babel from "@rolldown/plugin-babel"
import tailwindcss from "@tailwindcss/vite"

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    react(),
    // React Compiler via Babel — exclude Lit web component files that use
    // TypeScript decorators, which Babel cannot parse without extra plugins.
    babel({
      presets: [reactCompilerPreset()],
      exclude: ["**/ble/**"],
    }),
    tailwindcss(),
  ],
  resolve: {
    alias: {
      "@": path.resolve(__dirname, "./src"),
    },
  },
})
