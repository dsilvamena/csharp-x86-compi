import { useEffect, useMemo, useRef, type RefObject } from 'react';

// Auto-scroll compartido para los listados x86 (pestañas «x86» y «Simulación»).
// Cuando el hover viene del editor C#, el bloque .s de esa sentencia puede quedar
// fuera de la ventana scrolleable; este hook lo desplaza para que se vea. Si el
// bloque ya se muestra entero no toca nada, y no scrollea cuando el puntero está
// sobre el propio listado (ahí el hover lo originas tú leyendo el .s).
//
// Devuelve los handlers onEnter/onLeave que el contenedor scrolleable debe cablear
// para saber si el puntero está dentro. `listRef.current.children[i]` debe ser la
// línea .s de índice i (un div por línea), como en ambos listados.
export function useProvenanceScroll(
  listRef: RefObject<HTMLDivElement | null>,
  sourceMap: (number | null)[],
  hoverLine: number | null,
) {
  const pointerInside = useRef(false);

  // Bloque .s (primera/última línea) producido por la sentencia bajo el hover.
  const firstHitIdx = useMemo(
    () => (hoverLine == null ? -1 : sourceMap.findIndex((s) => s === hoverLine)),
    [sourceMap, hoverLine],
  );
  const lastHitIdx = useMemo(() => {
    if (hoverLine == null) return -1;
    for (let i = sourceMap.length - 1; i >= 0; i--) if (sourceMap[i] === hoverLine) return i;
    return -1;
  }, [sourceMap, hoverLine]);

  // Si el bloque no se ve entero, desplaza el listado para dejar su PRIMERA línea
  // cerca del tope — mostrando el inicio del bloque y todo lo que quepa debajo.
  useEffect(() => {
    const cont = listRef.current;
    if (!cont || hoverLine == null || pointerInside.current || firstHitIdx < 0) return;
    const firstEl = cont.children[firstHitIdx] as HTMLElement | undefined;
    const lastEl = (cont.children[lastHitIdx] as HTMLElement | undefined) ?? firstEl;
    if (!firstEl) return;
    const cTop = cont.getBoundingClientRect().top;
    const top = firstEl.getBoundingClientRect().top - cTop;       // px desde el tope visible
    const bottom = (lastEl ?? firstEl).getBoundingClientRect().bottom - cTop;
    const fullyVisible = top >= 0 && bottom <= cont.clientHeight;
    if (fullyVisible) return;
    cont.scrollTop += top - 8;  // primera línea ~8px bajo el tope
  }, [listRef, hoverLine, firstHitIdx, lastHitIdx]);

  return {
    onListEnter: () => { pointerInside.current = true; },
    onListLeave: () => { pointerInside.current = false; },
  };
}
