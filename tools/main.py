from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Iterator, Mapping

import html
import re
from pycparser import c_ast, parse_file
from pycparser.c_generator import CGenerator


# ===================== Data model =====================

@dataclass
class Sym:
    kind: str                          # "var" or "func"
    type_str: str
    defs: list[tuple[str, int, bool]] = field(default_factory=list)  # [(file, line, is_static)]
    extern_in: set[str] = field(default_factory=set)                 # files where extern/prototype appears
    used_in: set[str] = field(default_factory=set)                   # c files referencing the symbol


# ===================== Helpers =====================

def render_type_without_name(gen: CGenerator, decl: c_ast.Decl) -> str:
    s = gen.visit(decl.type)
    if decl.name:
        s = re.sub(rf"\b{re.escape(decl.name)}\b", "", s)
    s = re.sub(r"\s+", " ", s).replace(" *", " *").strip()
    return s


def iter_c_files(root: Path) -> Iterator[Path]:
    for p in sorted(root.rglob("*.c")):
        if p.is_file():
            yield p


def display_name(p: Path, base: Path) -> str:
    try:
        return str(p.resolve().relative_to(base.resolve()))
    except Exception:
        return p.name


def path_is_under(p: Path, root: Path) -> bool:
    try:
        p.resolve().relative_to(root.resolve())
        return True
    except Exception:
        return False


# ===================== Collectors =====================

class TUCollector(c_ast.NodeVisitor):
    """
    Collect only top-level items from a preprocessed .c translation unit.
    We do NOT descend into structs/enums/etc., so struct members will never be collected.
    """
    def __init__(self, tu_path: Path, src_root: Path) -> None:
        self.tu_path = tu_path.resolve()
        self.src_root = src_root.resolve()
        self.top_level_decls: list[c_ast.Decl] = []  # global vars + function prototypes
        self.func_defs: list[c_ast.FuncDef] = []     # function definitions

    # Only traverse the immediate children of FileAST.ext (true top-level)
    def visit_FileAST(self, node: c_ast.FileAST) -> None:
        for ext in node.ext or []:
            if isinstance(ext, c_ast.FuncDef):
                self.func_defs.append(ext)
            elif isinstance(ext, c_ast.Decl):
                # typedefs are not interesting
                if "typedef" in (ext.storage or []):
                    continue
                self.top_level_decls.append(ext)
            # do NOT recurse; prevents picking up struct/union members etc.

class UseCollector(c_ast.NodeVisitor):
    """
    Tracks uses of symbols (vars + funcs) within a TU.
    - Ignores references whose coord originates from a different file.
    - Tracks lexical scopes to avoid counting locals.
    - Block-scope 'extern T name;' is treated as a USE of a global, not a local.
    """
    def __init__(self, tu_path: Path, global_names: set[str]) -> None:
        self.tu_path = tu_path.resolve()
        self.global_names = global_names
        self.used: set[str] = set()
        self.scope_stack: list[set[str]] = [set()]

    def _in_this_tu(self, coord) -> bool:
        return bool(coord) and Path(coord.file).resolve() == self.tu_path

    def _push(self) -> None:
        self.scope_stack.append(set())

    def _pop(self) -> None:
        self.scope_stack.pop()

    def _declare_local(self, name: str) -> None:
        self.scope_stack[-1].add(name)

    def _is_local(self, name: str) -> bool:
        return any(name in scope for scope in reversed(self.scope_stack))

    def visit_FuncDef(self, node: c_ast.FuncDef) -> None:
        self._push()
        if isinstance(node.decl.type, c_ast.FuncDecl) and node.decl.type.args:
            for param in node.decl.type.args.params or []:
                if isinstance(param, c_ast.Decl) and param.name:
                    self._declare_local(param.name)
        self.visit(node.body)
        self._pop()

    def visit_Compound(self, node: c_ast.Compound) -> None:
        self._push()
        for item in node.block_items or []:
            if isinstance(item, c_ast.Decl) and item.name:
                if "extern" in (item.storage or []):
                    if self._in_this_tu(item.coord) and item.name in self.global_names:
                        self.used.add(item.name)  # treat as global use
                else:
                    self._declare_local(item.name)
            self.visit(item)
        self._pop()

    def visit_Decl(self, node: c_ast.Decl) -> None:
        if node.init is not None:
            self.visit(node.init)

    def visit_ID(self, node: c_ast.ID) -> None:
        if not self._in_this_tu(node.coord):
            return
        name = node.name
        if name in self.global_names and not self._is_local(name):
            self.used.add(name)


# ===================== Index building =====================

def build_index(
    src_root: Path,
    fake_includes: Path,
    extra_includes: Iterable[Path] = (),
    cpp: str = "clang",
    std: str = "c99",
) -> tuple[Mapping[str, Sym], Mapping[str, Sym]]:
    """
    Returns:
      vars_index: name -> Sym(kind='var')
      funcs_index: name -> Sym(kind='func')
    Only counts definitions that physically live under src_root in .c files.
    Externs/prototypes from headers are observed via preprocessing and can disqualify.
    """
    gen = CGenerator()
    vars_index: dict[str, Sym] = {}
    funcs_index: dict[str, Sym] = {}

    # Pass 1: top-level defs and extern/prototypes
    for tu in iter_c_files(src_root):
        ast = parse_file(
            str(tu),
            use_cpp=True,
            cpp_path=cpp,
            cpp_args=[
                "-E",
                f"-std={std}",
                f"-I{fake_includes}",
                *[f"-I{inc}" for inc in extra_includes],
            ],
        )
        tu_disp = display_name(tu, src_root)
        coll = TUCollector(tu, src_root)
        coll.visit(ast)

        for d in coll.top_level_decls:
            if not d.name:
                continue

            coord_file = Path(d.coord.file) if d.coord and d.coord.file else tu
            coord_file_res = coord_file.resolve()
            coord_file_disp = display_name(coord_file, src_root)

            if isinstance(d.type, c_ast.FuncDecl):
                # Function prototype: treat as "extern/prototype" if not in same .c
                name = d.name
                tstr = gen.visit(d.type)
                sym = funcs_index.setdefault(name, Sym(kind="func", type_str=tstr))
                if coord_file_res != tu.resolve():  # header or other file
                    sym.extern_in.add(coord_file_disp)
                if not sym.type_str:
                    sym.type_str = tstr
                continue

            # Global var at file scope
            name = d.name
            tstr = render_type_without_name(gen, d)
            sym = vars_index.setdefault(name, Sym(kind="var", type_str=tstr))

            if "extern" in (d.storage or []):
                sym.extern_in.add(coord_file_disp)
            else:
                # Count as a definition only if it physically resides under src/ and is a .c
                if path_is_under(coord_file, src_root) and coord_file.suffix == ".c":
                    is_static = "static" in (d.storage or [])
                    line = d.coord.line if d.coord else 0
                    sym.defs.append((coord_file_disp, line, is_static))

            if not sym.type_str:
                sym.type_str = tstr

        # Function definitions (always in a .c)
        for fdef in coll.func_defs:
            decl = fdef.decl
            if not decl.name:
                continue
            name = decl.name
            tstr = gen.visit(decl.type)
            is_static = "static" in (decl.storage or [])
            line = decl.coord.line if decl.coord else 0
            sym = funcs_index.setdefault(name, Sym(kind="func", type_str=tstr))
            sym.defs.append((tu_disp, line, is_static))
            if not sym.type_str:
                sym.type_str = tstr

    # Pass 2: cross-TU usage (both vars + funcs)
    all_names: set[str] = set(vars_index.keys()) | set(funcs_index.keys())
    for tu in iter_c_files(src_root):
        ast = parse_file(
            str(tu),
            use_cpp=True,
            cpp_path=cpp,
            cpp_args=[
                "-E",
                f"-std={std}",
                f"-I{fake_includes}",
                *[f"-I{inc}" for inc in extra_includes],
            ],
        )
        uc = UseCollector(tu, all_names)
        uc.visit(ast)
        tu_disp = display_name(tu, src_root)
        for name in uc.used:
            if name in vars_index:
                vars_index[name].used_in.add(tu_disp)
            if name in funcs_index:
                funcs_index[name].used_in.add(tu_disp)

    return vars_index, funcs_index


# ===================== HTML output =====================

def write_html(
    vars_index: Mapping[str, Sym],
    funcs_index: Mapping[str, Sym],
    out_path: Path,
) -> None:
    def esc(s: str) -> str:
        return html.escape(s, quote=True)

    # Globals eligible to become static:
    #   - exactly one definition in src/*.c
    #   - that definition is NOT already static
    #   - no extern/prototype outside the defining .c (extern_in - {def_file} == Ø)
    #   - no usage from other .c files (used_in - {def_file} == Ø)
    globals_rows_items: list[tuple[str, int, str]] = []
    for name, sym in vars_index.items():
        if len(sym.defs) != 1:
            continue
        def_file, def_line, is_static = sym.defs[0]
        if is_static:
            continue
        if any(e != def_file for e in sym.extern_in):
            continue
        if any(u != def_file for u in sym.used_in):
            continue

        row = (
            f"<tr>"
            f"<td><code>{esc(name)}</code></td>"
            f"<td><code>{esc(sym.type_str)}</code></td>"
            f"<td>{esc(def_file)}</td>"
            f"<td>{def_line}</td>"
            f"</tr>"
        )
        globals_rows_items.append((def_file, def_line, row))

    globals_rows_items.sort(key=lambda t: (t[0], t[1]))
    globals_rows = [h for _, _, h in globals_rows_items]

    # Functions eligible to become static:
    #   - exactly one definition in src/*.c
    #   - NOT already static
    #   - no prototype outside the defining .c (extern_in - {def_file} == Ø)
    #   - no usage from other .c files (used_in - {def_file} == Ø)
    funcs_rows_items: list[tuple[str, int, str]] = []
    for name, sym in funcs_index.items():
        if len(sym.defs) != 1:
            continue
        def_file, def_line, is_static = sym.defs[0]
        if is_static:
            continue
        if any(e != def_file for e in sym.extern_in):
            continue
        if any(u != def_file for u in sym.used_in):
            continue

        row = (
            f"<tr>"
            f"<td><code>{esc(name)}</code></td>"
            f"<td><code>{esc(sym.type_str)}</code></td>"
            f"<td>{esc(def_file)}</td>"
            f"<td>{def_line}</td>"
            f"</tr>"
        )
        funcs_rows_items.append((def_file, def_line, row))

    funcs_rows_items.sort(key=lambda t: (t[0], t[1]))
    funcs_rows = [h for _, _, h in funcs_rows_items]

    html_doc = f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Static candidates</title>
<style>
  body {{ font-family: system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial, sans-serif; }}
  table {{ border-collapse: collapse; width: 100%; margin-bottom: 2rem; }}
  th, td {{ border: 1px solid #ccc; padding: 6px 8px; vertical-align: top; }}
  th {{ background: #f6f6f6; text-align: left; }}
  code {{ font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, "Liberation Mono", monospace; }}
</style>
</head>
<body>
<h2>Globals that can be made static</h2>
<table>
  <thead>
    <tr><th>Name</th><th>Type</th><th>File</th><th>Line</th></tr>
  </thead>
  <tbody>
    {"".join(globals_rows)}
  </tbody>
</table>

<h2>Functions that can be made static</h2>
<table>
  <thead>
    <tr><th>Name</th><th>Type</th><th>File</th><th>Line</th></tr>
  </thead>
  <tbody>
    {"".join(funcs_rows)}
  </tbody>
</table>
</body>
</html>
"""
    out_path.write_text(html_doc, encoding="utf-8")


# ===================== CLI =====================

def main() -> None:
    here = Path(__file__).resolve()
    src_root = (here.parent / "../src").resolve()
    fake_includes = (here.parent / "utils" / "fake_libc_include").resolve()
    extra_includes = [Path("/opt/homebrew/Cellar/raylib/5.5/include")]

    vars_index, funcs_index = build_index(
        src_root=src_root,
        fake_includes=fake_includes,
        extra_includes=extra_includes,
        cpp="clang",
        std="c99",
    )
    out_html = here.parent / "globals.html"
    write_html(vars_index, funcs_index, out_html)


if __name__ == "__main__":
    main()
