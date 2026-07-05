#!/usr/bin/env python3
import os
import platform
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INPUTS = ROOT / "inputs"
ASM = ROOT / "outputs" / "asm"
BIN_OUT = ROOT / "outputs" / "bin"


def compiler_path():
    exe = ".exe" if platform.system() == "Windows" else ""
    return ROOT / "build" / f"compiler{exe}"


def run(cmd, check=False):
    print("\n$ " + " ".join(str(c) for c in cmd))
    sys.stdout.flush()
    p = subprocess.run(cmd, cwd=ROOT)
    if check and p.returncode != 0:
      input("\nEnter para continuar...")
    return p.returncode


def ensure_compiler():
    if compiler_path().exists():
        return True
    return run(["make"]) == 0


def input_files():
    return sorted(p for p in INPUTS.glob("*.cs") if not p.name.startswith("err_"))


def choose_input():
    files = input_files()
    print("\nInputs:")
    for i, path in enumerate(files, 1):
        print(f"{i}) {path.name}")
    print("0) Volver")
    choice = input("\nInput: ").strip()
    if choice == "0" or not choice:
        return None
    try:
        idx = int(choice)
        if 1 <= idx <= len(files):
            return files[idx - 1]
    except ValueError:
        pass
    print("Opcion invalida.")
    input("\nEnter para continuar...")
    return None


def compile_input(src, extra=None):
    if not ensure_compiler():
        return 1
    extra = extra or []
    return run([compiler_path(), src, *extra])


def compile_to_asm(src, opt=True):
    if not ensure_compiler():
        return 1
    ASM.mkdir(parents=True, exist_ok=True)
    mode = "--opt" if opt else "--no-opt"
    out = ASM / f"{src.stem}.s"
    return run([compiler_path(), src, mode, "-o", out])


def link_and_run(src, opt=True):
    if compile_to_asm(src, opt) != 0:
        return
    BIN_OUT.mkdir(parents=True, exist_ok=True)
    asm = ASM / f"{src.stem}.s"
    bin_path = BIN_OUT / src.stem
    if run(["g++", "-no-pie", asm, "-o", bin_path]) != 0:
        print("\nNo se pudo enlazar. Ejecuta esto en Linux/WSL o usa Docker.")
        input("\nEnter para continuar...")
        return
    run([bin_path])
    input("\nEnter para continuar...")


def show_file(path):
    if not path.exists():
        print(f"No existe: {path}")
    else:
        print()
        print(path)
        print("-" * len(str(path)))
        print(path.read_text(errors="replace"))
    input("\nEnter para continuar...")


def action_with_input(action):
    src = choose_input()
    if src:
        action(src)


def main():
    while True:
        print("\n=== csharp-x86-compi ===")
        print("1) Compilar input")
        print("2) Compilar sin optimizaciones")
        print("3) Compilar y ejecutar")
        print("4) Compilar y ejecutar sin optimizaciones")
        print("5) Mostrar tokens")
        print("6) Mostrar AST textual")
        print("7) Generar AST DOT/PNG")
        print("8) Ver assembly generado")
        print("9) Ejecutar tests")
        print("10) Limpiar build/outputs")
        print("0) Salir")
        try:
            choice = input("\nOpcion: ").strip()
        except EOFError:
            print()
            return

        if choice == "0":
            return
        if choice == "1":
            action_with_input(lambda src: (compile_input(src), input("\nEnter para continuar...")))
        elif choice == "2":
            action_with_input(lambda src: (compile_to_asm(src, opt=False), input("\nEnter para continuar...")))
        elif choice == "3":
            action_with_input(lambda src: link_and_run(src, opt=True))
        elif choice == "4":
            action_with_input(lambda src: link_and_run(src, opt=False))
        elif choice == "5":
            action_with_input(lambda src: (compile_input(src, ["--tokens"]), input("\nEnter para continuar...")))
        elif choice == "6":
            action_with_input(lambda src: (compile_input(src, ["--ast"]), input("\nEnter para continuar...")))
        elif choice == "7":
            action_with_input(lambda src: (compile_input(src, ["--ast-dot"]), input("\nEnter para continuar...")))
        elif choice == "8":
            action_with_input(lambda src: (compile_to_asm(src), show_file(ASM / f"{src.stem}.s")))
        elif choice == "9":
            run(["make", "test"])
            input("\nEnter para continuar...")
        elif choice == "10":
            run(["make", "clean"])
            input("\nEnter para continuar...")
        else:
            print("Opcion invalida.")
            input("\nEnter para continuar...")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print()
        sys.exit(0)
