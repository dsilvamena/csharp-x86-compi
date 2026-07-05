import { test, expect } from '@playwright/test';

// End-to-end against the REAL binary: Vite proxies /api → the Node backend, which
// spawns `c++/compiler --json`. The default sample (Fibonacci) must compile and
// render across the tabs; an err_* program must surface the error contract.

test('the default sample compiles and renders every tab', async ({ page }) => {
  await page.goto('/');

  // x86 is the default tab — wait for real AT&T assembly to arrive.
  await expect(page.locator('body')).toContainText('AT&T', { timeout: 15_000 });
  await expect(page.locator('body')).toContainText('rbp'); // %rbp prologue
  await expect(page.locator('body')).toContainText('printf'); // Console.WriteLine

  // Tokens
  await page.getByRole('button', { name: 'Tokens', exact: true }).click();
  await expect(page.locator('body')).toContainText('class');

  // AST (textual printAst tree)
  await page.getByRole('button', { name: 'AST', exact: true }).click();
  await expect(page.locator('body')).toContainText('Program');

  // Optimization report (real counters)
  await page.getByRole('button', { name: 'Optimización', exact: true }).click();
  await expect(page.locator('body')).toContainText('Reporte de optimización');

  // Simulación tab: the in-browser JS simulator runs the AT&T and shows CPU state.
  // It opens paused at step 0 (source-level debugger); "Ejecutar" runs to the end.
  await page.getByRole('button', { name: 'Simulación', exact: true }).click();
  await expect(page.locator('body')).toContainText('Registros');
  await page.getByRole('button', { name: 'Ejecutar', exact: true }).click();
  await expect(page.locator('body')).toContainText('6765'); // Fib(20), simulated

  // Errors tab: a clean program reports none.
  await page.getByRole('button', { name: /Errores/, exact: false }).click();
  await expect(page.locator('body')).toContainText('No errors');
});

test('the Run button runs the in-browser simulator (no Docker) and shows output', async ({ page }) => {
  await page.goto('/');
  // Wait for the default sample (Fibonacci → 6765) to compile.
  await expect(page.locator('body')).toContainText('AT&T', { timeout: 15_000 });

  // The primary "Run" button jumps to the Simulación tab and executes the
  // generated AT&T in the JavaScript simulator — entirely client-side.
  await page.getByRole('button', { name: 'Run', exact: true }).click();
  await expect(page.locator('body')).toContainText('Simulación x86-64');
  await expect(page.locator('body')).toContainText('rax');   // registers panel
  // The sim opens paused at step 0; run it to the end to produce stdout.
  await page.getByRole('button', { name: 'Ejecutar', exact: true }).click();
  await expect(page.locator('body')).toContainText('6765');  // simulated stdout
});

test('hovering an x86 line highlights its C# provenance (asmSourceMap)', async ({ page }) => {
  await page.goto('/');
  // x86 is the default tab; wait for the AT&T listing.
  await expect(page.locator('body')).toContainText('AT&T', { timeout: 15_000 });

  // Hover the printf call line: it maps to the Console.WriteLine source line, so
  // that line (and its siblings from the same statement) get the .prov-line wash.
  await page.getByText('call printf@PLT').first().hover();
  await expect(page.locator('.prov-line').first()).toBeVisible();
});

test('hovering a lower C# statement auto-scrolls the x86 listing to its block', async ({ page }) => {
  await page.goto('/');
  await expect(page.locator('body')).toContainText('AT&T', { timeout: 15_000 });

  const listing = page.getByTestId('asm-listing');
  const before = await listing.evaluate((el) => el.scrollTop);
  expect(before).toBe(0); // arranca arriba

  // Hover the Console.WriteLine statement in the editor: its x86 block lives near
  // the bottom of the listing, so the view must scroll to bring its first line in.
  await page.locator('.view-line', { hasText: 'Console.WriteLine' }).first().hover();
  await expect.poll(() => listing.evaluate((el) => el.scrollTop)).toBeGreaterThan(0);
  // The first line of the highlighted block ends up visible inside the listing.
  await expect(page.locator('.prov-line').first()).toBeInViewport();
});

test('the x86 tab native-run button assembles + executes in Docker and shows stdout', async ({ page }) => {
  await page.goto('/');
  // Wait for the default sample (Fibonacci → 6765) to compile (x86 is default tab).
  await expect(page.locator('body')).toContainText('AT&T', { timeout: 15_000 });

  // Oracle / native path: the x86 tab keeps a Docker run button (gcc:13).
  await page.getByRole('button', { name: /Ejecutar \(Docker\)/ }).click();
  // The "Salida" panel appears and shows the real program output (Docker run).
  await expect(page.locator('body')).toContainText('Salida', { timeout: 25_000 });
  await expect(page.locator('body')).toContainText('6765', { timeout: 25_000 });
});

test('POST /api/run returns the real native stdout', async ({ request }) => {
  const source = `class Program {
  static int Main() {
    Console.WriteLine(2 + 3);
    return 0;
  }
}`;
  const res = await request.post('/api/run', {
    headers: { 'content-type': 'application/json' },
    data: { source },
    timeout: 30_000,
  });
  expect(res.ok()).toBeTruthy();
  const json = await res.json();
  expect(json.compiled).toBe(true);
  expect(json.ok).toBe(true);
  expect(json.output).toContain('5');
  expect(json.exitCode).toBe(0);
});

test('backend returns the error contract for an invalid program', async ({ request }) => {
  const source = `class Program {
  static int Main() {
    x = 5;
    return 0;
  }
}`;
  const res = await request.post('/api/compile', {
    headers: { 'content-type': 'application/json' },
    data: { source },
  });
  expect(res.ok()).toBeTruthy();
  const json = await res.json();
  expect(json.success).toBe(false);
  expect(json.errors[0].phase).toBe('semantic');
  expect(typeof json.errors[0].line).toBe('number');
  expect(json.optReport).toBeNull();
});
