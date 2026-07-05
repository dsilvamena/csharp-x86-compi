import { test, expect, type Page } from '@playwright/test';

// Depurador a nivel de fuente en la pestaña «Simulación». Todo corre en el
// simulador JS del navegador (sin Docker). El sample por defecto es el
// Fibonacci iterativo:
//   1 class Program {
//   2   static int Fib(int n) {
//   ...
//   6     while (i < n) {
//   ...
//  16     Console.WriteLine(Fib(20));
//  17     return 0;
// (los números de línea C# importan para los asserts).

// Abre la pestaña Simulación una vez compilado el sample por defecto.
async function openSim(page: Page) {
  await page.goto('/');
  await expect(page.locator('body')).toContainText('AT&T', { timeout: 15_000 });
  await page.getByRole('button', { name: 'Simulación', exact: true }).click();
  await expect(page.locator('body')).toContainText('Simulación x86-64');
}

const steps = (page: Page) => page.getByTestId('sim-steps').innerText();
const csLine = (page: Page) => page.getByTestId('sim-cs-line').innerText();

test('la pestaña abre en el paso 0 (no hay que reiniciar antes de depurar)', async ({ page }) => {
  await openSim(page);
  // Sin tocar Reiniciar: el simulador arranca al inicio, listo para Step Into.
  await expect(page.getByTestId('sim-steps')).toHaveText('0');
});

test('Step Into entra en el cuerpo de Fib', async ({ page }) => {
  await openSim(page);

  // «Step Into» (modo por defecto «por sentencia C#») sí se detiene dentro del
  // callee: avanzando desde Main terminamos en alguna línea de Fib (3..12).
  const into = page.getByRole('button', { name: 'Step Into', exact: true });
  let enteredFib = false;
  for (let i = 0; i < 60; i++) {
    const m = (await csLine(page)).match(/C#:\s*(\d+)/);
    if (m && Number(m[1]) >= 3 && Number(m[1]) <= 12) { enteredFib = true; break; }
    if (await into.isDisabled()) break;
    await into.click();
  }
  expect(enteredFib).toBeTruthy();
});

test('el modo por sentencia avanza más que el modo por instrucción', async ({ page }) => {
  await openSim(page);

  // Un «Step Into» (por sentencia) avanza varias instrucciones de golpe.
  const before = Number(await steps(page));
  await page.getByRole('button', { name: 'Step Into', exact: true }).click();
  const afterStmt = Number(await steps(page));
  expect(afterStmt - before).toBeGreaterThan(1);

  // Cambia a «por instrucción x86»: un «Paso» avanza exactamente 1 instrucción.
  await page.getByRole('button', { name: 'Reiniciar', exact: true }).click();
  await page.getByRole('button', { name: 'por instrucción x86', exact: true }).click();
  const b2 = Number(await steps(page));
  await page.getByRole('button', { name: 'Paso', exact: true }).click();
  expect(Number(await steps(page)) - b2).toBe(1);
});

test('la línea de tiempo reproduce el mismo estado que llegar paso a paso', async ({ page }) => {
  await openSim(page);

  // Estado alcanzado avanzando 15 instrucciones.
  await page.getByRole('button', { name: 'por instrucción x86', exact: true }).click();
  for (let i = 0; i < 15; i++) {
    await page.getByRole('button', { name: 'Paso', exact: true }).click();
  }
  await expect(page.getByTestId('sim-steps')).toHaveText('15');
  const regsStepped = await page.getByTestId('sim-regs').innerText();

  // Retrocede al inicio y luego arrastra la línea de tiempo al paso 15: mismo estado.
  const timeline = page.getByTestId('sim-timeline');
  await timeline.fill('0');
  await expect(page.getByTestId('sim-steps')).toHaveText('0');
  await timeline.fill('15');
  await expect(page.getByTestId('sim-steps')).toHaveText('15');
  const regsScrubbed = await page.getByTestId('sim-regs').innerText();

  expect(regsScrubbed).toBe(regsStepped);
});
