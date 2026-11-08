#!/usr/bin/env python3
import json
import subprocess
from pathlib import Path

root = Path.cwd()
core = root / "Core"
main = core / "Src" / "main.c"
files = [main, *sorted(p for p in core.rglob("*")
                       if p.is_file() and p != main)]

document = [
    '#set page(paper: "a4", margin: 15mm, numbering: "1")',
    '#set text(font: "Menlo", size: 8pt)',
    '#set par(leading: 3pt)',
]

for index, path in enumerate(files):
    if index:
        document.append("#pagebreak()")

    name = path.relative_to(root).as_posix()
    language = "c" if path.suffix in {".c", ".cpp", ".h"} else "cmake"
    document.extend([
        f'#text(size: 12pt, weight: "bold")[{name}]',
        '#v(8pt)',
        f'#raw(read({json.dumps("/" + name)}), '
        f'lang: "{language}", block: true)',
    ])

subprocess.run(
    ["typst", "compile", "--root", str(root), "-", "Core.pdf"],
    input="\n".join(document),
    text=True,
    check=True,
)
