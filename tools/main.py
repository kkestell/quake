from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Iterator, Mapping

import html
import re
from pycparser import c_ast, parse_file
from pycparser.c_generator import CGenerator


@dataclass
class Sym:
    type_str: str
    defined_in: set[str] = field(default_factory=set)
    extern_in: set[str] = field(default_factory=set)
    used_in: set[str] = field(default_factory=set)


class UseCollector(c_ast.NodeVisitor):
    def __init__(self, tu_path: Path, global_names: set[str]) -> None:
        self.tu_path = tu_path.resolve()
        self.global_names = global_names
        self.used: set[str] = set()
        self.scope_stack: list[set[str]] = [set()]

    # ---- scope management ----
    def _push(self) -> None:
        self.scope_stack.append(set())

    def _pop(self) -> None:
        self.scope_stack.pop()

    def _declare(self, name: str) -> None:
        self.scope_stack[-1].add(name)

    def _is_local(self, name: str) -> bool:
        return any(name in scope for scope in reversed(self.scope_stack))

    # ---- visitors ----
    def visit_FuncDef(self, node: c_ast.FuncDef) -> None:
        self._push()
        # parameters are always local
        if isinstance(node.decl.type, c_ast.FuncDecl) and node.decl.type.args:
            for param in node.decl.type.args.params:
                if isinstance(param, c_ast.Decl) and param.name:
                    self._declare(param.name)
        self.visit(node.body)
        self._pop()

    def visit_Compound(self, node: c_ast.Compound) -> None:
        self._push()
        for block_item in node.block_items or []:
            if isinstance(block_item, c_ast.Decl) and block_item.name:
                storage = block_item.storage or []
                # IMPORTANT: block-scope 'extern' is NOT a new local object and must not shadow the global
                if "extern" not in storage:
                    self._declare(block_item.name)
                if block_item.init is not None:
                    self.visit(block_item.init)
                continue
            self.visit(block_item)
        self._pop()

    def visit_For(self, node: c_ast.For) -> None:
        self._push()
        # handle declarations in for-init (e.g., for (int i=0; ...))
        init = getattr(node, "init", None)
        if isinstance(init, c_ast.DeclList):
            for decl in init.decls or []:
                if isinstance(decl, c_ast.Decl) and decl.name:
                    storage = decl.storage or []
                    if "extern" not in storage:
                        self._declare(decl.name)
                    if decl.init is not None:
                        self.visit(decl.init)
        elif init is not None:
            self.visit(init)
        if node.cond is not None:
            self.visit(node.cond)
        if node.next is not None:
            self.visit(node.next)
        if node.stmt is not None:
            self.visit(node.stmt)
        self._pop()

    def visit_Decl(self, node: c_ast.Decl) -> None:
        # Do not treat declarations as a "use".
        # Still traverse initializers for identifier uses.
        if node.init is not None:
            self.visit(node.init)

    def visit_ID(self, node: c_ast.ID) -> None:
        if not node.coord:
            return
        coord_path = Path(node.coord.file).resolve()
        if coord_path != self.tu_path:
            return
        name = node.name
        if name in self.global_names and not self._is_local(name):
            self.used.add(name)


def render_type_without_name(gen: CGenerator, d: c_ast.Decl) -> str:
    s = gen.visit(d.type)
    s = re.sub(rf"\b{re.escape(d.name)}\b", "", s).strip()
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


def split_file_scope_decls(
    ast: c_ast.FileAST, tu_path: Path, src_root: Path
) -> tuple[list[c_ast.Decl], list[c_ast.Decl]]:
    own: list[c_ast.Decl] = []
    hdr: list[c_ast.Decl] = []
    tu_path = tu_path.resolve()
    src_root = src_root.resolve()

    def is_project_header(path: Path) -> bool:
        try:
            rp = path.resolve()
            rp.relative_to(src_root)
            return rp != tu_path
        except Exception:
            return False

    for ext in ast.ext:
        if not isinstance(ext, c_ast.Decl):
            continue
        if isinstance(ext.type, c_ast.FuncDecl):
            continue
        if "typedef" in (ext.storage or []):
            continue
        if not ext.coord or not ext.name:
            continue

        coord_path = Path(ext.coord.file).resolve()
        if coord_path == tu_path:
            own.append(ext)
        elif is_project_header(coord_path):
            hdr.append(ext)

    return own, hdr


def build_index(
    src_root: Path,
    fake_includes: Path,
    extra_includes: Iterable[Path] = (),
    cpp: str = "clang",
    std: str = "c99",
) -> Mapping[str, Sym]:
    gen = CGenerator()
    symbols: dict[str, Sym] = {}
    all_globals: set[str] = set()
    tu_decls: dict[Path, list[c_ast.Decl]] = {}

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
        own, hdr = split_file_scope_decls(ast, tu, src_root)
        tu_decls[tu] = own

        for d in hdr:
            if "extern" in (d.storage or []):
                name = d.name
                entry = symbols.setdefault(name, Sym(type_str=render_type_without_name(gen, d)))
                entry.extern_in.add(display_name(Path(d.coord.file), src_root))

        for d in own:
            # Only track names with external linkage
            if "static" in (d.storage or []):
                continue
            all_globals.add(d.name)

    for tu, decls in tu_decls.items():
        tu_disp = display_name(tu, src_root)
        for d in decls:
            if "static" in (d.storage or []):
                continue
            name = d.name
            tstr = render_type_without_name(gen, d)
            entry = symbols.setdefault(name, Sym(type_str=tstr))
            if "extern" in (d.storage or []):
                entry.extern_in.add(tu_disp)
            else:
                entry.defined_in.add(tu_disp)
            if not entry.type_str:
                entry.type_str = tstr

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
        uc = UseCollector(tu, all_globals)
        uc.visit(ast)
        tu_disp = display_name(tu, src_root)
        for name in uc.used:
            if name in symbols:
                symbols[name].used_in.add(tu_disp)

    return symbols


def write_html(symbols: Mapping[str, Sym], out_path: Path) -> None:
    def esc(s: str) -> str:
        return html.escape(s, quote=True)

    def render_ul(items: Iterable[str]) -> str:
        items = list(sorted(items))
        if not items:
            return ""
        lis = "\n".join(f"<li>{esc(p)}</li>" for p in items)
        return f"<ul>\n{lis}\n</ul>"

    def resolve_source_path(def_path: str) -> Path:
        p = Path(def_path)
        if p.is_absolute():
            return p
        # Try a few sensible roots: alongside HTML, ../src from HTML, and CWD
        candidates = [
            out_path.parent / def_path,
            (out_path.parent / "../src").resolve() / def_path,
            Path.cwd() / def_path,
        ]
        for c in candidates:
            if c.exists():
                return c
        # Fallback to assuming ../src
        return (out_path.parent / "../src").resolve() / def_path

    def first_occurrence_line(path: Path, name: str) -> int:
        """
        Return 1-based line number of the first occurrence of `name` in `path`,
        ignoring obvious comments. If not found, return a large sentinel.
        """
        try:
            text = path.read_text(encoding="utf-8", errors="ignore").splitlines()
        except Exception:
            return 10**9
        in_block = False
        pat = re.compile(rf"\b{re.escape(name)}\b")
        for i, raw in enumerate(text, 1):
            line = raw
            # crude block comment handling
            if in_block:
                if "*/" in line:
                    in_block = False
                    line = line.split("*/", 1)[1]
                else:
                    continue
            while "/*" in line:
                pre, post = line.split("/*", 1)
                if "*/" in post:
                    post = post.split("*/", 1)[1]
                    line = pre + " " + post
                else:
                    in_block = True
                    line = pre
                    break
            s = line.lstrip()
            if s.startswith("//"):
                continue
            if pat.search(line):
                return i
        return 10**9

    # Main table rows
    rows: list[str] = []
    for name in sorted(symbols):
        sym = symbols[name]
        defined = render_ul(sym.defined_in)
        extern = render_ul(sym.extern_in)
        used = render_ul(sym.used_in)
        row = (
            "<tr>"
            f"<td><code>{esc(name)}</code></td>"
            f"<td><code>{esc(sym.type_str)}</code></td>"
            f"<td>{defined}</td>"
            f"<td>{extern}</td>"
            f"<td>{used}</td>"
            "</tr>"
        )
        rows.append(row)

    # Candidates for static: has at least one definition, no externs observed,
    # and no cross-TU uses (i.e., used only within its defining TU(s)).
    cand_records: list[tuple[str, int, str, str]] = []
    for name, sym in symbols.items():
        if not sym.defined_in or sym.extern_in:
            continue
        cross_tu_uses = set(sym.used_in) - set(sym.defined_in)
        if cross_tu_uses:
            continue
        for def_path in sym.defined_in:
            abs_path = resolve_source_path(def_path)
            line_no = first_occurrence_line(abs_path, name)
            cand_records.append((def_path, line_no, name, sym.type_str))

    # Sort by file path, then line number, then name
    cand_records.sort(key=lambda t: (t[0], t[1], t[2]))

    # Render candidates table
    if cand_records:
        cand_rows = [
            "<tr>"
            f"<td>{esc(def_path)}</td>"
            f"<td>{line_no if line_no != 10**9 else ''}</td>"
            f"<td><code>{esc(name)}</code></td>"
            f"<td><code>{esc(type_str)}</code></td>"
            "</tr>"
            for def_path, line_no, name, type_str in cand_records
        ]
        candidates_table = (
            "<table>"
            "<thead><tr>"
            "<th>Defined in</th><th>Line</th><th>Name</th><th>Type</th>"
            "</tr></thead>"
            f"<tbody>{''.join(cand_rows)}</tbody>"
            "</table>"
        )
    else:
        candidates_table = (
            "<table>"
            "<thead><tr><th>Defined in</th><th>Line</th><th>Name</th><th>Type</th></tr></thead>"
            "<tbody><tr><td colspan=\"4\">None</td></tr></tbody>"
            "</table>"
        )

    html_doc = f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>File-scope objects with external linkage</title>
<style>
  body {{ font-family: system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial, sans-serif; }}
  table {{ border-collapse: collapse; width: 100%; }}
  th, td {{ border: 1px solid #ccc; padding: 6px 8px; vertical-align: top; }}
  th {{ background: #f6f6f6; text-align: left; }}
  code {{ font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, "Liberation Mono", monospace; }}
  ul {{ margin: 0; padding-left: 1.25rem; }}
  h2 {{ margin-top: 1.25rem; }}
</style>
</head>
<body>
<table>
  <thead>
    <tr>
      <th>Name</th>
      <th>Type</th>
      <th>Defined</th>
      <th>Extern</th>
      <th>Used</th>
    </tr>
  </thead>
  <tbody>
    {"".join(rows)}
  </tbody>
</table>

<h2>Variables that can be made <code>static</code></h2>
{candidates_table}

</body>
</html>
"""
    out_path.write_text(html_doc, encoding="utf-8")



def main() -> None:
    here = Path(__file__).resolve()
    src_root = (here.parent / "../src").resolve()
    fake_includes = (here.parent / "utils" / "fake_libc_include").resolve()
    extra_includes = [Path("/opt/homebrew/Cellar/raylib/5.5/include")]
    symbols = build_index(
        src_root=src_root,
        fake_includes=fake_includes,
        extra_includes=extra_includes,
        cpp="clang",
        std="c99",
    )
    out_html = here.parent / "globals.html"
    write_html(symbols, out_html)


if __name__ == "__main__":
    main()
