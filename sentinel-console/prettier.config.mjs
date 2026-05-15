/** @type {import("prettier").Config} */
const config = {
  semi: false,
  tabWidth: 2,
  trailingComma: "all",
  printWidth: 80,
  // Prettier v3 automatically respects .gitignore, so node_modules, dist and
  // pnpm-lock.yaml are already excluded without a separate .prettierignore.
}

export default config
