import { spawn } from 'node:child_process'
import { once } from 'node:events'
import process from 'node:process'
import { createServer } from 'vite'

// Keeping Vite in this process avoids orphaned pnpm/cmd server trees on Windows.
const server = await createServer({ server: { port: 1438, strictPort: true } })
await server.listen()
try {
  const child = spawn(process.execPath, ['node_modules/@playwright/test/cli.js', 'test', ...process.argv.slice(2)], { stdio: 'inherit' })
  const [code] = await once(child, 'exit')
  process.exitCode = code ?? 1
} finally {
  await server.close()
}
