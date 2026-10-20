import assert from 'node:assert/strict';
import { readFile, mkdtemp, writeFile, rm } from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import { test } from 'node:test';
import handler from '../api/compile.js';
import { X86AttSimulator } from '../typescript/packages/web/src/sim/simulator.js';

async function request(body, method = 'POST') {
  const response = {
    headers: {},
    setHeader(name, value) { this.headers[name] = value; },
    status(code) { this.statusCode = code; return this; },
    json(value) { this.body = value; return this; },
  };
  await handler({ method, body }, response);
  return response;
}

// Exercise the actual production entry point and compiler, not a mock response.
test('Vercel handler compiles every valid fixture and reports every negative fixture', async () => {
  const { readdir } = await import('node:fs/promises');
  const directory = new URL('../c++/inputs/', import.meta.url);
  const files = (await readdir(directory)).filter((name) => name.endsWith('.cs'));
  assert.ok(files.length > 0);
  for (const name of files) {
    const source = await readFile(new URL(name, directory), 'utf8');
    const res = await request({ source });
    assert.equal(res.statusCode, 200, name);
    assert.equal(res.body.success, !name.startsWith('err_'), name);
    assert.equal(res.headers['Cache-Control'], 'no-store');
    if (res.body.success) {
      assert.ok(res.body.asm.includes('main:'), name);
      assert.ok(res.body.asmOpt.includes('main:'), name);
      assert.ok(res.body.tokens.length > 0, name);
      assert.ok(res.body.ast.length > 0, name);
      assert.equal(res.body.asmSourceMap.length, res.body.asm.trimEnd().split('\n').length, name);
    } else {
      assert.ok(res.body.errors.length > 0, name);
    }
  }
});

test('optimized and unoptimized assembly reproduce reference stdout in the browser simulator', async () => {
  const { readdir } = await import('node:fs/promises');
  const directory = new URL('../c++/tests/expected/', import.meta.url);
  for (const name of (await readdir(directory)).filter((name) => name.endsWith('.txt'))) {
    const source = await readFile(new URL(`../c++/inputs/${name.replace('.txt', '.cs')}`, import.meta.url), 'utf8');
    const expected = (await readFile(new URL(name, directory), 'utf8')).trimEnd();
    const res = await request({ source });
    assert.equal(res.statusCode, 200, name);
    assert.equal(res.body.success, true, name);
    for (const variant of ['asm', 'asmOpt']) {
      const snapshot = new X86AttSimulator(res.body[variant], { maxSteps: 1_000_000 }).run();
      assert.equal(snapshot.error, null, `${name} (${variant})`);
      assert.equal(snapshot.output.trimEnd(), expected, `${name} (${variant})`);
    }
  }
});

test('validates HTTP method, source type and UTF-8 byte limit', async () => {
  const get = await request(undefined, 'GET');
  assert.equal(get.statusCode, 405);
  assert.equal(get.headers.Allow, 'POST');
  for (const body of [undefined, null, {}, { source: 42 }]) {
    assert.equal((await request(body)).statusCode, 400);
  }
  assert.equal((await request({ source: 'ñ'.repeat(128 * 1024 + 1) })).statusCode, 413);
});

test('accepts text/plain and buffered source bodies', async () => {
  const source = 'class Program { static int Main() { return 0; } }';
  assert.equal((await request(source)).body.success, true);
  assert.equal((await request(Buffer.from(source))).body.success, true);
});

test('compiler startup, malformed output, excessive output and timeout return controlled errors', async () => {
  const directory = await mkdtemp(path.join(os.tmpdir(), 'compiler-handler-'));
  const previousBin = process.env.COMPILER_BIN;
  const previousTimeout = process.env.COMPILE_TIMEOUT_MS;
  try {
    process.env.COMPILER_BIN = path.join(directory, 'missing');
    assert.equal((await request({ source: '' })).statusCode, 503);
    // Real child processes exercise stream handling and termination.
    const executable = path.join(directory, 'compiler');
    process.env.COMPILER_BIN = executable;
    async function script(code) {
      await writeFile(executable, `#!${process.execPath}\n${code}\n`, { mode: 0o755 });
    }
    await script('console.log("invalid json");');
    assert.equal((await request({ source: '' })).statusCode, 502);
    await script('process.stdout.write("x".repeat(4 * 1024 * 1024));');
    assert.equal((await request({ source: '' })).statusCode, 413);
    await script('setInterval(() => {}, 1000);');
    process.env.COMPILE_TIMEOUT_MS = '100';
    assert.equal((await request({ source: '' })).statusCode, 504);
  } finally {
    if (previousBin === undefined) delete process.env.COMPILER_BIN;
    else process.env.COMPILER_BIN = previousBin;
    if (previousTimeout === undefined) delete process.env.COMPILE_TIMEOUT_MS;
    else process.env.COMPILE_TIMEOUT_MS = previousTimeout;
    await rm(directory, { recursive: true, force: true });
  }
});
