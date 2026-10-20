import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const root = fileURLToPath(new URL('../', import.meta.url));
if (process.platform !== 'linux') {
  throw new Error('El build de Vercel requiere Linux. Para desarrollo local usa npm run build.');
}

function run(command, args) {
  const result = spawnSync(command, args, { cwd: root, stdio: 'inherit' });
  if (result.error) throw result.error;
  if (result.status !== 0) process.exit(result.status || 1);
}

// A separate output and -B prevent reusing a cached or uploaded macOS binary.
// Link C++ support libraries statically to avoid libstdc++ version mismatches.
run('make', ['-B', '-C', 'c++', 'BIN=build/vercel/compiler', 'BUILD_DIR=build/vercel',
  'CXX=g++', 'CXXFLAGS=-std=c++17 -Wall -Wextra -O2 -static-libstdc++ -static-libgcc']);

const smoke = spawnSync('./c++/build/vercel/compiler', ['--json'], {
  cwd: root,
  input: 'class Program { static int Main() { Console.WriteLine(5); return 0; } }',
  encoding: 'utf8',
  timeout: 15000,
});
if (smoke.error) throw smoke.error;
if (smoke.status !== 0 || !JSON.parse(smoke.stdout).success) {
  throw new Error(`Falló la prueba del compilador Linux: ${smoke.stderr}`);
}
run('npm', ['--prefix', 'typescript', 'run', 'build']);
