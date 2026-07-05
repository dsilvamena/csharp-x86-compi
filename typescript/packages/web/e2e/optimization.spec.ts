import { test, expect } from '@playwright/test';

// La pestaña «Optimización» muestra un diff alineado entre el .s sin optimizar y
// el optimizado: lo eliminado se resalta en rojo (izquierda) y lo optimizado en
// verde (derecha). El sample «Constant folding» (int x = 10 + 5 * 2) pliega la
// aritmética en compilación, así que el diff tiene cambios reales.

test('el diff de optimización resalta las instrucciones eliminadas/plegadas', async ({ page }) => {
  await page.goto('/');
  await expect(page.locator('body')).toContainText('AT&T', { timeout: 15_000 });

  await page.selectOption('select', 'const_fold');
  await page.getByRole('button', { name: 'Optimización', exact: true }).click();

  // Cabeceras y leyenda del diff.
  await expect(page.locator('body')).toContainText('x86 — sin optimizar');
  await expect(page.locator('body')).toContainText('x86 — optimizado');
  await expect(page.locator('body')).toContainText('eliminado');

  // Hubo mejora medible (folding + peephole reducen instrucciones).
  await expect(page.locator('body')).toContainText(/Mejora:\s*\d/);

  // Y el diff pintó al menos una línea eliminada (rojo) en la columna izquierda.
  await expect(page.locator('div[class*="bg-error/10"]').first()).toBeVisible();
});
