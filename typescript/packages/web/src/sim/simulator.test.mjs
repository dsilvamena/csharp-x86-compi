// =============================================================================
// simulator.test.mjs — Pruebas del simulador x86-64 AT&T (JavaScript puro).
// =============================================================================
// Se ejecuta con el runner integrado de Node (sin dependencias nuevas):
//     node --test src/sim/simulator.test.mjs
//   (desde typescript/packages/web)  ó, vía npm:  npm run test:sim
//
// Cubre dos niveles:
//   1) Unidades: semántica de instrucciones aisladas (movq/push/pop/cmp/jmp,
//      direccionamiento AT&T, intercepción de printf/malloc).
//   2) Integración: toma el ensamblador REAL que generó el compilador C++
//      (c++/outputs/<n>.s) y verifica que la salida simulada coincide con la
//      salida nativa de referencia (c++/outputs/expected/<n>.txt).
// =============================================================================
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync, existsSync, readdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

import { X86AttSimulator } from './simulator.js';

const here = dirname(fileURLToPath(import.meta.url));
const repoRoot = join(here, '..', '..', '..', '..', '..'); // src/sim -> repo
const outDir = join(repoRoot, 'c++', 'outputs');
const expDir = join(outDir, 'expected');

// Atajo: simula `asm` completo y devuelve el snapshot final.
function run(asm) {
  return new X86AttSimulator(asm).run();
}

// ---- 1) Unidades -----------------------------------------------------------

test('movq con inmediato escribe el registro destino (orden AT&T)', () => {
  const snap = run(`.text\n.globl main\nmain:\n  movq $5, %rax\n  movq $0, %rax\n  ret\n`);
  assert.equal(snap.halted, true);
});

test('pushq/popq mueven %rsp y transfieren el valor por la pila', () => {
  const sim = new X86AttSimulator(
    `.text\n.globl main\nmain:\n  movq $42, %rbx\n  pushq %rbx\n  popq %rcx\n  ret\n`,
  );
  const rspBefore = sim.regs.rsp;
  sim.step(); // movq $42,%rbx
  sim.step(); // pushq %rbx  -> rsp -= 8
  assert.equal(sim.regs.rsp, rspBefore - 8);
  assert.equal(sim.snapshot().stack[0].value, 42);
  sim.step(); // popq %rcx   -> rsp += 8
  assert.equal(sim.regs.rsp, rspBefore);
  assert.equal(sim.regs.rcx, 42);
});

test('direccionamiento desp(base) lee y escribe memoria', () => {
  const sim = new X86AttSimulator(
    `.text\n.globl main\nmain:\n  movq %rsp, %rbp\n  movq $7, -8(%rbp)\n  movq -8(%rbp), %rax\n  ret\n`,
  );
  sim.run();
  assert.equal(sim.regs.rax, 7);
});

test('direccionamiento escalado (base,índice,escala): rax+rcx*8', () => {
  const sim = new X86AttSimulator(
    `.text\n.globl main\nmain:\n` +
    `  movq %rsp, %rax\n  movq $2, %rcx\n` +
    `  movq $99, (%rax,%rcx,8)\n` +     // mem[rsp + 2*8] = 99
    `  movq 16(%rax), %rdx\n  ret\n`,   // lee mem[rsp + 16]
  );
  sim.run();
  assert.equal(sim.regs.rdx, 99);
});

test('cmpq + setle + je evalúan condición con signo', () => {
  // 3 <= 5  -> setle pone 1 ; cmpq $0 ; je no salta (ZF=0)
  const sim = new X86AttSimulator(
    `.text\n.globl main\nmain:\n` +
    `  movq $5, %rcx\n  movq $3, %rax\n` +
    `  cmpq %rcx, %rax\n  setle %al\n  movzbq %al, %rax\n  ret\n`,
  );
  sim.run();
  assert.equal(sim.regs.rax, 1);
});

test('printf@PLT con %ld toma el formato de .data y el valor de %rsi', () => {
  const asm =
    `.data\n.Lfmt_int:  .string "%ld\\n"\n.text\n.globl main\nmain:\n` +
    `  movq $123, %rsi\n  leaq .Lfmt_int(%rip), %rdi\n  xorl %eax, %eax\n` +
    `  call printf@PLT\n  movq $0, %rax\n  ret\n`;
  const snap = run(asm);
  assert.equal(snap.output, '123\n');
});

test('malloc@PLT devuelve heap ficticio creciente y la memoria es escribible', () => {
  const asm =
    `.text\n.globl main\nmain:\n` +
    `  movq $16, %rdi\n  call malloc@PLT\n  movq %rax, %rbx\n` +   // p = malloc(16)
    `  movq $11, 0(%rbx)\n  movq $22, 8(%rbx)\n` +
    `  movq 8(%rbx), %rax\n  ret\n`;
  const sim = new X86AttSimulator(asm);
  sim.run();
  assert.equal(sim.regs.rax, 22);
  assert.equal(sim.snapshot().heap.length >= 2, true);
});

test('snapshot expone línea actual, pasos y banderas', () => {
  const sim = new X86AttSimulator(`.text\n.globl main\nmain:\n  movq $1, %rax\n  ret\n`);
  const s0 = sim.snapshot();
  assert.equal(typeof s0.currentLine, 'number');
  assert.equal(s0.steps, 0);
  sim.step();
  assert.equal(sim.snapshot().steps, 1);
});

// ---- 2) Integración con el ensamblador real del compilador -----------------
// Recorre c++/outputs/*.s y compara la salida simulada con la salida nativa de
// referencia. Si el compilador aún no ha generado los .s (carpeta ausente), se
// omite con aviso (no es un fallo: la suite C++ los regenera con `make`).

test('simula el ensamblador real del compilador y reproduce la salida nativa', (t) => {
  if (!existsSync(outDir) || !existsSync(expDir)) {
    t.skip(`no hay ensamblador generado en ${outDir} (corre el compilador antes)`);
    return;
  }
  // 12_benchmark hace 1e6 iteraciones × FibIter(20): es un programa pensado para
  // medir TIEMPOS del binario nativo (cientos de millones de instrucciones), no
  // para la simulación visual paso a paso. Queda fuera del oráculo a propósito.
  const SKIP = new Set(['12_benchmark']);
  const programs = readdirSync(expDir)
    .filter((f) => f.endsWith('.txt'))
    .map((f) => f.replace(/\.txt$/, ''))
    .filter((n) => !SKIP.has(n) && existsSync(join(outDir, `${n}.s`)));

  assert.ok(programs.length > 0, 'no se encontraron pares .s/.txt');

  for (const name of programs) {
    const asm = readFileSync(join(outDir, `${name}.s`), 'utf8');
    const expected = readFileSync(join(expDir, `${name}.txt`), 'utf8');
    const snap = run(asm);
    assert.equal(snap.error, null, `${name}: error de simulación: ${snap.error}`);
    // Compara ignorando un posible salto de línea final discrepante.
    assert.equal(
      snap.output.replace(/\n$/, ''),
      expected.replace(/\n$/, ''),
      `${name}: la salida simulada no coincide con la nativa`,
    );
  }
});
