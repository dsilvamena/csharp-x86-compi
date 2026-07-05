// Diff línea a línea entre dos textos de ensamblador (sin optimizar vs optimizado)
// para el diff «lado a lado» de la pestaña Optimización. Usa la subsecuencia común
// más larga (LCS) — los .s del subconjunto académico son pequeños, así que el
// O(n·m) es de sobra. Las líneas cambiadas se emparejan en una misma fila
// (izquierda = eliminada/roja, derecha = optimizada/verde) para que se vea qué
// plegó el ConstantVisitor y qué quitó el peephole.

export interface DiffRow {
  left: string | null;   // línea del .s sin optimizar (null = hueco)
  right: string | null;  // línea del .s optimizado (null = hueco)
  leftNo: number | null;  // nº de línea 1-based en el .s sin optimizar
  rightNo: number | null; // nº de línea 1-based en el .s optimizado
  same: boolean;          // true = línea idéntica en ambos (sin resaltar)
}

export function splitDiff(aText: string, bText: string): DiffRow[] {
  const a = aText.split('\n');
  const b = bText.split('\n');
  const n = a.length;
  const m = b.length;

  // dp[i][j] = longitud de la LCS de a[i..] y b[j..].
  const dp: number[][] = Array.from({ length: n + 1 }, () => new Array<number>(m + 1).fill(0));
  for (let i = n - 1; i >= 0; i--) {
    for (let j = m - 1; j >= 0; j--) {
      dp[i][j] = a[i] === b[j] ? dp[i + 1][j + 1] + 1 : Math.max(dp[i + 1][j], dp[i][j + 1]);
    }
  }

  type Op = { t: 'same' | 'del' | 'ins'; s: string; ai: number; bi: number };
  const ops: Op[] = [];
  let i = 0;
  let j = 0;
  while (i < n && j < m) {
    if (a[i] === b[j]) { ops.push({ t: 'same', s: a[i], ai: i, bi: j }); i++; j++; }
    else if (dp[i + 1][j] >= dp[i][j + 1]) { ops.push({ t: 'del', s: a[i], ai: i, bi: -1 }); i++; }
    else { ops.push({ t: 'ins', s: b[j], ai: -1, bi: j }); j++; }
  }
  while (i < n) { ops.push({ t: 'del', s: a[i], ai: i, bi: -1 }); i++; }
  while (j < m) { ops.push({ t: 'ins', s: b[j], ai: -1, bi: j }); j++; }

  // Empareja bloques de cambio: los del/ins consecutivos van en la misma fila.
  const rows: DiffRow[] = [];
  let k = 0;
  while (k < ops.length) {
    if (ops[k].t === 'same') {
      const o = ops[k];
      rows.push({ left: o.s, right: o.s, leftNo: o.ai + 1, rightNo: o.bi + 1, same: true });
      k++;
      continue;
    }
    const dels: Op[] = [];
    const inss: Op[] = [];
    while (k < ops.length && ops[k].t !== 'same') {
      if (ops[k].t === 'del') dels.push(ops[k]); else inss.push(ops[k]);
      k++;
    }
    const len = Math.max(dels.length, inss.length);
    for (let x = 0; x < len; x++) {
      const d = dels[x];
      const ins = inss[x];
      rows.push({
        left: d ? d.s : null,
        right: ins ? ins.s : null,
        leftNo: d ? d.ai + 1 : null,
        rightNo: ins ? ins.bi + 1 : null,
        same: false,
      });
    }
  }
  return rows;
}
