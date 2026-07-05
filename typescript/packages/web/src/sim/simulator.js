// =============================================================================
// simulator.js — Simulador x86-64 AT&T en JavaScript puro.
// =============================================================================
// Interpreta, paso a paso y sobre una CPU virtual pequeña, el ensamblador
// x86-64 en sintaxis AT&T (GNU) que produce el compilador C++ estilo Lab10 de
// este proyecto. NO ensambla ni ejecuta un binario real: lee el texto del `.s`
// y lo simula, generando snapshots (registros / stack / flags / memoria /
// salida) para que la web los muestre.
//
// Alcance deliberadamente limitado al subconjunto que genera ESTE compilador
// (ver c++/src/codegen.cpp): enteros de 64 bits, marco %rbp, paso de argumentos
// SysV en registros, `printf@PLT` y `malloc@PLT` interceptados (sin SO real).
// No pretende ser un x86 completo.
//
// Flujo de simulación (separado del flujo nativo real g++ -no-pie):
//   C# subset → compilador C++ → ensamblador AT&T (texto) → este simulador →
//   snapshots para la web.
//
// Convenciones AT&T respetadas:
//   · Orden de operandos: `op  fuente, destino`  (movq $5, %rax  ⇒ rax = 5).
//   · Inmediatos con `$`, registros con `%`, memoria `desp(base,índice,escala)`.
//   · Etiquetas de datos vía `leaq etiqueta(%rip), %reg`.
//
// API pública:
//   const sim = new X86AttSimulator(asmText);
//   sim.step();           // ejecuta una instrucción (según %rip)
//   sim.run();            // ejecuta hasta `ret` de main / halted
//   sim.snapshot();       // estado completo para la UI
//   sim.reset();          // reinicia la CPU sin re-parsear
// =============================================================================

// ---- Mapa de registros: alias de ancho -> registro de 64 bits ---------------
// Solo se incluyen los anchos que el compilador realmente emite (64, 32 y 8
// bits) y los registros que usa (System V + temporales).
const REG64 = [
  'rax', 'rbx', 'rcx', 'rdx', 'rsi', 'rdi', 'rbp', 'rsp',
  'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15',
];

// alias -> { base, width }  (width en bytes: 8, 4, 1)
const REG_ALIAS = (() => {
  const m = new Map();
  for (const r of REG64) m.set(r, { base: r, width: 8 });
  // 32 bits: %eax..%edi y %r8d..%r15d
  const low32 = {
    eax: 'rax', ebx: 'rbx', ecx: 'rcx', edx: 'rdx',
    esi: 'rsi', edi: 'rdi', ebp: 'rbp', esp: 'rsp',
  };
  for (const [a, b] of Object.entries(low32)) m.set(a, { base: b, width: 4 });
  for (let i = 8; i <= 15; i++) m.set(`r${i}d`, { base: `r${i}`, width: 4 });
  // 8 bits (byte bajo): %al..%dl, %sil, %dil, %bpl, %spl y %r8b..%r15b
  const low8 = {
    al: 'rax', bl: 'rbx', cl: 'rcx', dl: 'rdx',
    sil: 'rsi', dil: 'rdi', bpl: 'rbp', spl: 'rsp',
  };
  for (const [a, b] of Object.entries(low8)) m.set(a, { base: b, width: 1 });
  for (let i = 8; i <= 15; i++) m.set(`r${i}b`, { base: `r${i}`, width: 1 });
  return m;
})();

// Direcciones base de las regiones de memoria simulada (valores arbitrarios pero
// alineados a 8 y bien separados para que se distingan en la UI).
const STACK_TOP = 0x7fffff00; // la pila crece hacia abajo desde aquí
const HEAP_BASE = 0x10000000; // malloc reparte desde aquí hacia arriba
const DATA_BASE = 0x00400000; // .data / .rodata (formatos y literales string)
const HALT_RA = -1;           // dirección de retorno centinela: ret => halt

export class X86AttSimulator {
  /**
   * @param {string} assemblyText  ensamblador AT&T generado por el compilador.
   * @param {{maxSteps?: number}} [options]
   */
  constructor(assemblyText, options = {}) {
    this.source = String(assemblyText || '');
    this.maxSteps = options.maxSteps ?? 2_000_000;
    this._parse(this.source);
    this.reset();
  }

  // ===========================================================================
  // Parseo del ensamblador AT&T
  // ===========================================================================
  // Separa secciones (.data/.rodata/.text), etiquetas (`main:`, `.Lstr0:`,
  // `.Lfmt_int:`), strings de `.string` y la lista de instrucciones de .text.
  _parse(text) {
    this.instrs = [];                 // [{ line, op, args[], raw }]
    this.labels = new Map();          // etiqueta de código -> índice en instrs
    this.dataAddr = new Map();        // etiqueta de datos  -> dirección simulada
    this.dataStrings = new Map();     // dirección -> string decodificado
    this.dataOrder = [];              // [{ label, address, value }] (para la UI)

    let section = '.text';
    let dataPtr = DATA_BASE;
    let pendingDataLabels = [];       // etiquetas de datos aún sin .string

    const lines = text.split('\n');
    for (let i = 0; i < lines.length; i++) {
      const lineNo = i + 1;           // 1-based, coincide con el .s mostrado
      let line = stripComment(lines[i]).trim();
      if (!line) continue;

      // Directiva de sección.
      if (line === '.data' || line === '.text' ||
          line === '.rodata' || line.startsWith('.section')) {
        if (line.startsWith('.section')) {
          section = line.includes('.rodata') ? '.rodata'
                  : line.includes('.data') ? '.data' : '.text';
        } else {
          section = line;
        }
        continue;
      }

      // Etiqueta al inicio de la línea (puede llevar resto: `.Lfmt_int: .string ...`).
      const lab = line.match(/^([.\w$]+):\s*(.*)$/);
      if (lab) {
        const name = lab[1];
        const rest = lab[2].trim();
        if (section === '.text') {
          this.labels.set(name, this.instrs.length);
        } else {
          pendingDataLabels.push(name);
        }
        if (!rest) continue;
        line = rest;                  // procesa lo que sigue a la etiqueta
      }

      // Directivas de datos: `.string "..."` define un literal en memoria.
      if (line.startsWith('.string') || line.startsWith('.asciz')) {
        const str = parseStringDirective(line);
        const addr = dataPtr;
        const labelsHere = pendingDataLabels.length ? pendingDataLabels : [`.Ldata${addr.toString(16)}`];
        for (const l of labelsHere) {
          this.dataAddr.set(l, addr);
          this.dataOrder.push({ label: l, address: addr, value: str });
        }
        this.dataStrings.set(addr, str);
        pendingDataLabels = [];
        dataPtr += alignUp(str.length + 1, 8); // +1 por el terminador NUL
        continue;
      }

      // Otras directivas (.globl, .align, .note, etc.): se ignoran.
      if (line.startsWith('.')) {
        pendingDataLabels = [];
        continue;
      }

      // Instrucción: `op  arg0, arg1`.
      if (section === '.text') {
        const sp = line.search(/\s/);
        const op = (sp === -1 ? line : line.slice(0, sp)).toLowerCase();
        const argStr = sp === -1 ? '' : line.slice(sp + 1).trim();
        const args = splitArgs(argStr);
        this.instrs.push({ line: lineNo, op, args, raw: line });
      }
    }
  }

  // ===========================================================================
  // CPU virtual
  // ===========================================================================
  reset() {
    /** @type {Record<string, number>} registros de 64 bits */
    this.regs = {};
    for (const r of REG64) this.regs[r] = 0;
    this.regs.rsp = STACK_TOP;
    this.regs.rbp = 0;

    this.flags = { ZF: false, SF: false, OF: false, CF: false };
    this.memory = new Map();   // dirección (múltiplo de 8) -> valor de 64 bits
    this.heapPtr = HEAP_BASE;  // siguiente dirección que devolverá malloc
    this.output = '';
    this.steps = 0;
    this.halted = false;
    this.exitCode = 0;
    this.error = null;

    // %rip = índice de la instrucción a ejecutar. Se arranca en `main`.
    this.regs.rip = this.labels.has('main') ? this.labels.get('main') : 0;
    // Dirección de retorno centinela: cuando `main` haga ret, se detiene.
    this._push(HALT_RA);
  }

  // ---- Memoria ---------------------------------------------------------------
  _read(addr) {
    return this.memory.get(addr) ?? 0;
  }
  _write(addr, value) {
    this.memory.set(addr, toI64(value));
  }
  _push(value) {
    this.regs.rsp -= 8;
    this._write(this.regs.rsp, value);
  }
  _pop() {
    const v = this._read(this.regs.rsp);
    this.regs.rsp += 8;
    return v;
  }

  // ---- Registros con ancho ---------------------------------------------------
  _getReg(name) {
    const a = REG_ALIAS.get(name);
    if (!a) throw new Error(`registro desconocido: %${name}`);
    const v = this.regs[a.base];
    if (a.width === 8) return v;
    if (a.width === 4) return umod(v, 0x100000000);          // 32 bits bajos
    return umod(v, 0x100);                                    // 8 bits bajos
  }
  _setReg(name, value) {
    const a = REG_ALIAS.get(name);
    if (!a) throw new Error(`registro desconocido: %${name}`);
    if (a.width === 8) {
      this.regs[a.base] = toI64(value);
    } else if (a.width === 4) {
      // Escribir un registro de 32 bits pone a cero los 32 bits altos (x86-64).
      this.regs[a.base] = umod(value, 0x100000000);
    } else {
      // Escribir el byte bajo conserva el resto del registro.
      const cur = this.regs[a.base];
      const hi = cur - umod(cur, 0x100);
      this.regs[a.base] = hi + umod(value, 0x100);
    }
  }

  // ===========================================================================
  // Decodificación de operandos
  // ===========================================================================
  // Devuelve { kind, ... }:
  //   imm  -> { kind:'imm', value }
  //   reg  -> { kind:'reg', name }
  //   mem  -> { kind:'mem', disp, base, index, scale, sym }
  //   sym  -> { kind:'sym', name }   (objetivo de jmp/call)
  _operand(tok) {
    const t = tok.trim();
    if (t.startsWith('$')) return { kind: 'imm', value: parseImm(t.slice(1)) };
    if (t.startsWith('%')) return { kind: 'reg', name: t.slice(1) };
    // Memoria: [desp]'('base[,índice[,escala]]')'  o  símbolo(%rip)
    const mem = t.match(/^([-\w.$]*)\(([^)]*)\)$/);
    if (mem) {
      const dispTok = mem[1];
      const inside = mem[2].split(',').map((s) => s.trim());
      const base = inside[0] ? inside[0].replace(/^%/, '') : null;
      const index = inside[1] ? inside[1].replace(/^%/, '') : null;
      const scale = inside[2] ? parseInt(inside[2], 10) : 1;
      // desp puede ser numérico (-8, 0, 16) o un símbolo (.Lfmt_int con %rip).
      const dispNum = dispTok === '' ? 0 : Number(dispTok);
      const numeric = dispTok === '' || !Number.isNaN(dispNum);
      return {
        kind: 'mem',
        disp: numeric ? (dispTok === '' ? 0 : dispNum) : 0,
        sym: numeric ? null : dispTok,
        base, index, scale,
      };
    }
    // Símbolo pelado: etiqueta de salto o función (call/jmp/jXX).
    return { kind: 'sym', name: t };
  }

  // Dirección efectiva de un operando de memoria.
  _ea(op) {
    if (op.sym) {
      // símbolo(%rip): dirección del dato (resuelta en el parseo).
      if (this.dataAddr.has(op.sym)) return this.dataAddr.get(op.sym);
      throw new Error(`símbolo de datos no encontrado: ${op.sym}`);
    }
    let addr = op.disp;
    if (op.base) addr += this._getReg(op.base);
    if (op.index) addr += this._getReg(op.index) * (op.scale || 1);
    return addr;
  }

  // Lee el valor de un operando (imm / reg / mem).
  _val(op) {
    if (op.kind === 'imm') return op.value;
    if (op.kind === 'reg') return this._getReg(op.name);
    if (op.kind === 'mem') return this._read(this._ea(op));
    throw new Error(`operando no leíble: ${op.kind}`);
  }
  // Escribe en un operando destino (reg / mem).
  _store(op, value) {
    if (op.kind === 'reg') return this._setReg(op.name, value);
    if (op.kind === 'mem') return this._write(this._ea(op), value);
    throw new Error(`operando no escribible: ${op.kind}`);
  }

  // ===========================================================================
  // Flags
  // ===========================================================================
  _flagsSub(a, b) {
    const res = toI64(a - b);
    this.flags.ZF = res === 0;
    this.flags.SF = res < 0;
    this.flags.OF = ((a < 0) !== (b < 0)) && ((res < 0) !== (a < 0));
    this.flags.CF = uval(a) < uval(b);
  }
  _flagsLogic(res) {
    res = toI64(res);
    this.flags.ZF = res === 0;
    this.flags.SF = res < 0;
    this.flags.OF = false;
    this.flags.CF = false;
  }
  // Condición de un sufijo setXX / jXX a partir de los flags.
  _cond(suffix) {
    const f = this.flags;
    switch (suffix) {
      case 'e':  return f.ZF;
      case 'ne': return !f.ZF;
      case 'l':  return f.SF !== f.OF;
      case 'le': return f.ZF || (f.SF !== f.OF);
      case 'g':  return !f.ZF && (f.SF === f.OF);
      case 'ge': return f.SF === f.OF;
      default:   throw new Error(`condición desconocida: ${suffix}`);
    }
  }

  // ===========================================================================
  // Ejecución
  // ===========================================================================
  /** Ejecuta una sola instrucción (la que indica %rip). Devuelve false si ya
   *  estaba detenido o si esta instrucción detuvo la simulación. */
  step() {
    if (this.halted) return false;
    if (this.steps >= this.maxSteps) {
      this._halt(`límite de pasos alcanzado (${this.maxSteps})`);
      return false;
    }
    const ip = this.regs.rip;
    if (ip < 0 || ip >= this.instrs.length) {
      this._halt('rip fuera de rango');
      return false;
    }
    const ins = this.instrs[ip];
    this.regs.rip = ip + 1;          // avance por defecto (salvo salto/call/ret)
    this.steps++;
    try {
      this._exec(ins);
    } catch (e) {
      this._halt(`error en línea ${ins.line}: ${e.message}`);
      return false;
    }
    return !this.halted;
  }

  /** Ejecuta hasta detenerse (ret de main, halt o error). Devuelve el snapshot. */
  run() {
    while (!this.halted) {
      if (!this.step()) break;
    }
    return this.snapshot();
  }

  _halt(error) {
    this.halted = true;
    if (error) this.error = error;
  }

  _exec(ins) {
    const { op, args } = ins;
    const a = args.map((t) => this._operand(t)); // operandos decodificados

    // Salto condicional (jXX) y setXX comparten sufijo de condición.
    if (op[0] === 'j' && op !== 'jmp') {
      if (this._cond(op.slice(1))) this.regs.rip = this._target(a[0]);
      return;
    }
    if (op.startsWith('set')) {
      this._store(a[0], this._cond(op.slice(3)) ? 1 : 0);
      return;
    }

    switch (op) {
      // ---- Movimientos ------------------------------------------------------
      case 'movq':
      case 'movl':
        this._store(a[1], this._val(a[0]));
        return;
      case 'movzbq': // mueve byte bajo con extensión por ceros
        this._store(a[1], umod(this._val(a[0]), 0x100));
        return;
      case 'leaq':   // carga la dirección efectiva (no el contenido)
        this._store(a[1], this._ea(a[0]));
        return;

      // ---- Pila -------------------------------------------------------------
      case 'pushq':
        this._push(this._val(a[0]));
        return;
      case 'popq':
        this._store(a[0], this._pop());
        return;

      // ---- Aritmética / lógica (destino = destino OP fuente) ----------------
      case 'addq': {
        const r = toI64(this._val(a[1]) + this._val(a[0]));
        this._store(a[1], r); this._flagsLogic(r); return;
      }
      case 'subq': {
        const d = this._val(a[1]), s = this._val(a[0]);
        const r = toI64(d - s); this._store(a[1], r); this._flagsSub(d, s); return;
      }
      case 'imulq': {
        const r = toI64(this._val(a[1]) * this._val(a[0]));
        this._store(a[1], r); this._flagsLogic(r); return;
      }
      case 'andq': {
        const r = toI64(bigAnd(this._val(a[1]), this._val(a[0])));
        this._store(a[1], r); this._flagsLogic(r); return;
      }
      case 'orq': {
        const r = toI64(bigOr(this._val(a[1]), this._val(a[0])));
        this._store(a[1], r); this._flagsLogic(r); return;
      }
      case 'xorq': {
        const r = toI64(bigXor(this._val(a[1]), this._val(a[0])));
        this._store(a[1], r); this._flagsLogic(r); return;
      }
      case 'xorl': {
        const r = umod(bigXor(this._val(a[1]), this._val(a[0])), 0x100000000);
        this._store(a[1], r); this._flagsLogic(r); return;
      }
      case 'negq': {
        const r = toI64(-this._val(a[0])); this._store(a[0], r); this._flagsLogic(r); return;
      }
      case 'notq': {
        const r = toI64(-this._val(a[0]) - 1); this._store(a[0], r); return; // ~x = -x-1
      }
      case 'cqto':  // signo de %rax -> %rdx (rdx:rax = sext(rax))
        this.regs.rdx = this.regs.rax < 0 ? -1 : 0;
        return;

      // ---- División ---------------------------------------------------------
      case 'idivq': {
        const div = this._val(a[0]);
        if (div === 0) throw new Error('división por cero');
        const q = Math.trunc(this.regs.rax / div);
        const r = this.regs.rax - q * div;
        this.regs.rax = toI64(q);
        this.regs.rdx = toI64(r);
        return;
      }

      // ---- Comparación ------------------------------------------------------
      case 'cmpq':
        this._flagsSub(this._val(a[1]), this._val(a[0]));
        return;
      case 'testq': {
        this._flagsLogic(bigAnd(this._val(a[1]), this._val(a[0])));
        return;
      }

      // ---- Control de flujo -------------------------------------------------
      case 'jmp':
        this.regs.rip = this._target(a[0]);
        return;
      case 'call':
        this._call(a[0]);
        return;
      case 'ret': {
        const ra = this._pop();
        if (ra === HALT_RA) { this.exitCode = toI64(this.regs.rax); this._halt(null); return; }
        this.regs.rip = ra;
        return;
      }
      case 'leave': // movq %rbp,%rsp ; popq %rbp
        this.regs.rsp = this.regs.rbp;
        this.regs.rbp = this._pop();
        return;

      case 'nop':
        return;

      default:
        throw new Error(`instrucción no soportada: ${op}`);
    }
  }

  // Índice de salto de un operando símbolo (etiqueta de código).
  _target(op) {
    const name = op.kind === 'sym' ? op.name : op.name;
    if (!this.labels.has(name)) throw new Error(`etiqueta desconocida: ${name}`);
    return this.labels.get(name);
  }

  // call: intercepta printf/malloc; en otro caso, llamada interna real.
  _call(op) {
    const name = op.name;
    if (name === 'printf@PLT' || name === 'printf') { this._printf(); return; }
    if (name === 'malloc@PLT' || name === 'malloc') { this._malloc(); return; }
    if (!this.labels.has(name)) throw new Error(`llamada a símbolo desconocido: ${name}`);
    this._push(this.regs.rip);        // dirección de retorno = siguiente instr
    this.regs.rip = this.labels.get(name);
  }

  // ===========================================================================
  // Llamadas de biblioteca interceptadas (SysV ABI, sin SO real)
  // ===========================================================================
  // printf(%rdi=formato, %rsi,%rdx,%rcx,%r8,%r9 = argumentos).
  _printf() {
    const fmtAddr = this.regs.rdi;
    const fmt = this.dataStrings.get(fmtAddr);
    if (fmt === undefined) throw new Error('printf: formato no apunta a un string conocido');
    const argRegs = ['rsi', 'rdx', 'rcx', 'r8', 'r9'];
    let ai = 0;
    let out = '';
    for (let i = 0; i < fmt.length; i++) {
      if (fmt[i] !== '%') { out += fmt[i]; continue; }
      i++;
      // Salta banderas/anchura/longitud simples (l, ld, d, c, s, %, u, x, zu...).
      let spec = '';
      while (i < fmt.length && /[-0-9.lhzu]/.test(fmt[i])) { spec += fmt[i]; i++; }
      const conv = fmt[i];
      if (conv === '%') { out += '%'; continue; }
      const reg = argRegs[ai++];
      const v = reg ? this.regs[reg] : 0;
      if (conv === 'd' || conv === 'i' || conv === 'u') out += String(toI64(v));
      else if (conv === 'l') { /* ya consumido por spec, raro */ out += String(toI64(v)); }
      else if (conv === 'c') out += String.fromCharCode(umod(v, 0x100));
      else if (conv === 's') out += this.dataStrings.get(v) ?? this._cstr(v);
      else if (conv === 'x') out += umod(v, 0x100000000).toString(16);
      else out += '%' + spec + (conv ?? '');
    }
    this.output += out;
    this.regs.rax = out.length;       // printf devuelve nº de caracteres
  }

  // Lee una cadena terminada en NUL desde memoria byte a byte (fallback para %s
  // cuando el puntero no es una etiqueta de datos conocida).
  _cstr(addr) {
    let s = '';
    for (let k = 0; k < 4096; k++) {
      const byte = umod(this._read(addr + k), 0x100);
      if (byte === 0) break;
      s += String.fromCharCode(byte);
    }
    return s;
  }

  // malloc(%rdi = nº de bytes) -> %rax = dirección ficticia de heap.
  _malloc() {
    const size = this.regs.rdi;
    const addr = this.heapPtr;
    this.heapPtr += alignUp(Math.max(8, size), 8);
    this.regs.rax = addr;
  }

  // ===========================================================================
  // Snapshot para la web
  // ===========================================================================
  snapshot() {
    const ip = this.regs.rip;
    const ins = ip >= 0 && ip < this.instrs.length ? this.instrs[ip] : null;

    // Pila: desde %rsp hacia la cima (direcciones crecientes), en pasos de 8.
    const stack = [];
    for (let addr = this.regs.rsp; addr < STACK_TOP && stack.length < 64; addr += 8) {
      stack.push({ address: addr, value: this._read(addr) });
    }
    // Heap: celdas asignadas (entre HEAP_BASE y heapPtr) que tienen valor.
    const heap = [];
    for (let addr = HEAP_BASE; addr < this.heapPtr && heap.length < 64; addr += 8) {
      heap.push({ address: addr, value: this._read(addr) });
    }

    const regs = {};
    for (const r of REG64) regs[r] = this.regs[r];
    regs.rip = this.regs.rip;

    return {
      regs,
      flags: { ...this.flags },
      stack,
      heap,
      data: this.dataOrder.map((d) => ({ ...d })),
      output: this.output,
      currentLine: ins ? ins.line : null,
      currentInstr: ins ? ins.raw : null,
      steps: this.steps,
      halted: this.halted,
      exitCode: this.exitCode,
      error: this.error,
    };
  }
}

// =============================================================================
// Utilidades de parseo / aritmética
// =============================================================================

// Quita comentarios de línea (`#` y `;`) respetando comillas de `.string`.
function stripComment(line) {
  let inStr = false;
  for (let i = 0; i < line.length; i++) {
    const c = line[i];
    if (c === '"' && line[i - 1] !== '\\') inStr = !inStr;
    else if (!inStr && (c === '#' || c === ';')) return line.slice(0, i);
  }
  return line;
}

// Separa operandos por comas de nivel superior (ignora las que van dentro de
// paréntesis, p. ej. `(%rax, %rcx, 8)`).
function splitArgs(s) {
  if (!s) return [];
  const out = [];
  let depth = 0, cur = '';
  for (const c of s) {
    if (c === '(') { depth++; cur += c; }
    else if (c === ')') { depth--; cur += c; }
    else if (c === ',' && depth === 0) { out.push(cur.trim()); cur = ''; }
    else cur += c;
  }
  if (cur.trim()) out.push(cur.trim());
  return out;
}

// Decodifica el literal de una directiva `.string "..."` (escapes C básicos).
function parseStringDirective(line) {
  const m = line.match(/"((?:[^"\\]|\\.)*)"/);
  if (!m) return '';
  return m[1].replace(/\\(.)/g, (_, c) => {
    switch (c) {
      case 'n': return '\n';
      case 't': return '\t';
      case 'r': return '\r';
      case '0': return '\0';
      case '\\': return '\\';
      case '"': return '"';
      default: return c;
    }
  });
}

// Inmediato AT&T: decimal o hex (`$0x10`), con signo opcional.
function parseImm(s) {
  const t = s.trim();
  const n = t.startsWith('0x') || t.startsWith('-0x')
    ? parseInt(t, 16)
    : Number(t);
  if (Number.isNaN(n)) throw new Error(`inmediato inválido: $${s}`);
  return n;
}

// ---- Aritmética de 64 bits (modelo con Number; los programas académicos del
// compilador usan enteros pequeños). toI64 normaliza a entero con signo. -------
function toI64(n) {
  return Math.trunc(n);
}
function alignUp(n, a) {
  return Math.ceil(n / a) * a;
}
// Módulo no negativo (para anchos de 32/8 bits sin las trampas de los bitwise
// de 32 bits de JS sobre valores grandes).
function umod(v, m) {
  const r = Math.trunc(v) % m;
  return r < 0 ? r + m : r;
}
// Valor "sin signo" aproximado para CF (no crítico en este subconjunto).
function uval(v) {
  return v < 0 ? v + 0x10000000000000000 : v;
}
// Operaciones lógicas seguras para 64 bits usando BigInt internamente.
function bigAnd(a, b) { return Number(BigInt(Math.trunc(a)) & BigInt(Math.trunc(b))); }
function bigOr(a, b) { return Number(BigInt(Math.trunc(a)) | BigInt(Math.trunc(b))); }
function bigXor(a, b) { return Number(BigInt(Math.trunc(a)) ^ BigInt(Math.trunc(b))); }

export default X86AttSimulator;
