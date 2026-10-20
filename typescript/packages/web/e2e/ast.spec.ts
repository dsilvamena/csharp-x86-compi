import { test, expect } from '@playwright/test';

// La pestaña AST muestra un árbol interactivo (plegable, tipos coloreados) además
// del volcado de texto crudo. Se parsea desde el árbol textual de printAst.

test('la vista AST muestra un árbol interactivo plegable', async ({ page }) => {
  await page.goto('/');
  await expect(page.locator('body')).toContainText('AT&T', { timeout: 15_000 });

  await page.getByRole('button', { name: 'AST', exact: true }).click();
  await expect(page.locator('body')).toContainText('Árbol de sintaxis abstracta');
  await expect(page.locator('body')).toContainText('nodos');

  // Nodos como pastillas coloreadas por categoría.
  await expect(page.getByText('MethodDecl', { exact: true }).first()).toBeVisible();

  // «Colapsar» oculta los nodos internos (MethodDecl queda dentro de Program plegado).
  await page.getByRole('button', { name: 'Colapsar', exact: true }).click();
  await expect(page.getByText('MethodDecl', { exact: true })).toHaveCount(0);

  // «Expandir» los vuelve a mostrar.
  await page.getByRole('button', { name: 'Expandir', exact: true }).click();
  await expect(page.getByText('MethodDecl', { exact: true }).first()).toBeVisible();

  // El modo «Texto» conserva el volcado crudo de printAst.
  await page.getByRole('button', { name: 'Texto', exact: true }).click();
  await expect(page.locator('pre')).toContainText('Program');
});
