// Puente fino entre la app React (TypeScript) y el simulador x86-64 AT&T en
// JavaScript puro (../sim/simulator.js). El simulador hace todo el trabajo; aquí
// solo se fija un tope de pasos pensado para el navegador y se reexpone el tipo
// del snapshot para las vistas.
//
// Flujo de simulación:
//   compilador C++ → ensamblador AT&T (texto) → este simulador → snapshots.
import { X86AttSimulator, type Snapshot } from '../sim/simulator.js';

export type { Snapshot } from '../sim/simulator.js';

// Tope de pasos para la web: los programas del subconjunto académico terminan
// en muy pocos pasos; este límite evita que un bucle pesado (p. ej. el benchmark
// de 1e6 iteraciones) congele la pestaña. Al alcanzarlo, snapshot().error lo
// indica y la UI recomienda reducir el tamaño del programa.
export const WEB_MAX_STEPS = 1_000_000;

/** Crea un simulador para `asm` con el tope de pasos de la web. */
export function makeSimulator(asm: string): X86AttSimulator {
  return new X86AttSimulator(asm, { maxSteps: WEB_MAX_STEPS });
}

/** Simula `asm` de principio a fin y devuelve el snapshot final. */
export function simulate(asm: string): Snapshot {
  return makeSimulator(asm).run();
}

/** ¿El error del snapshot es por haber superado el tope de pasos? */
export function isStepLimit(snap: Snapshot): boolean {
  return !!snap.error && snap.error.includes('límite de pasos');
}

export { X86AttSimulator };
