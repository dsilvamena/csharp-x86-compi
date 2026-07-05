const STAGES = ['Lexer', 'Parser', 'Semantic', 'IR', 'x86-64', 'Optimizer', 'Simulator'];

export function Hero() {
  return (
    <div className="hero-glow relative overflow-hidden border-b border-hair">
      <div className="mx-auto max-w-[1320px] px-7 pb-[34px] pt-[54px]">
        <span className="inline-flex items-center gap-2 text-[11.5px] uppercase tracking-[0.18em] text-text2">
          <span className="h-1.5 w-1.5 rounded-full bg-accent" style={{ boxShadow: '0 0 10px var(--color-accent)' }} />
          Compiler pipeline · client-side
        </span>
        <h1 className="mt-4 text-[42px] font-semibold leading-[1.05] tracking-[-0.025em]">
          Compile C# into x86.
          <br />
          <span className="font-serif italic font-normal text-accent" style={{ letterSpacing: 0 }}>
            Inspect every compiler stage.
          </span>
        </h1>
        <p className="mt-3.5 max-w-[620px] text-[15.5px] leading-[1.55] text-text2">
          A web-based compiler pipeline for lexical analysis, parsing, semantic checks, optimization
          and x86 code generation.
        </p>
        <div className="mt-5 flex flex-wrap items-center gap-[7px]">
          {STAGES.map((s, i) => (
            <span key={s} className="flex items-center gap-[7px]">
              <span
                className="rounded-full border px-3 py-[5px] text-[12px] text-text"
                style={{
                  borderColor: 'color-mix(in srgb, var(--color-accent) 45%, var(--border))',
                  background: 'color-mix(in srgb, var(--color-accent) 9%, var(--color-panel))',
                }}
              >
                {s}
              </span>
              {i < STAGES.length - 1 && <span className="text-[12px] text-text3">→</span>}
            </span>
          ))}
        </div>
      </div>
    </div>
  );
}
