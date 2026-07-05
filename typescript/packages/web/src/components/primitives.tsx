import type { ReactNode } from 'react';

export function Card({ children, className = '' }: { children: ReactNode; className?: string }) {
  return (
    <div className={`overflow-hidden rounded-[13px] border border-hair bg-panel ${className}`}>
      {children}
    </div>
  );
}

export function CardHeader({
  title,
  tag,
  right,
  dot = true,
}: {
  title: ReactNode;
  tag?: ReactNode;
  right?: ReactNode;
  dot?: boolean;
}) {
  return (
    <div className="flex items-center justify-between border-b border-hair2 bg-panel2 px-3.5 py-[11px]">
      <div className="flex items-center gap-2.5 text-[12.5px] text-text2">
        {dot && <span className="h-[9px] w-[9px] rounded-full bg-accent" />}
        {title}
      </div>
      <div className="flex items-center gap-2">
        {tag && (
          <span className="rounded-[5px] border border-hair px-1.5 py-px font-mono text-[10.5px] text-text3">
            {tag}
          </span>
        )}
        {right}
      </div>
    </div>
  );
}

export function PanelLabel({ children, right }: { children: ReactNode; right?: ReactNode }) {
  return (
    <h4 className="mb-2.5 flex justify-between text-[10.5px] font-medium uppercase tracking-[0.16em] text-text3">
      <span>{children}</span>
      {right && <span className="text-text3">{right}</span>}
    </h4>
  );
}

export function Empty({ children }: { children: ReactNode }) {
  return (
    <div className="flex h-full min-h-[120px] items-center justify-center px-6 py-10 text-center text-[13px] text-text3">
      {children}
    </div>
  );
}
