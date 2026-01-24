#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

IN_TXT = ROOT / "platforms/all/gamecontroller-db/gamecontrollersdl3-db.txt"
OUT_C  = ROOT / "src/RC2D/RC2D_gamecontrollerdb_embedded.c"
OUT_H  = ROOT / "include/RC2D/RC2D_gamecontrollerdb_embedded.h"

SYM = "RC2D_gamecontrollerdb"

data = IN_TXT.read_bytes()
size = len(data)

OUT_H.parent.mkdir(parents=True, exist_ok=True)

OUT_H.write_text(
f"""#pragma once
#include <stddef.h>

extern const unsigned char {SYM}_data[];
extern const size_t {SYM}_size;
""",
encoding="utf-8"
)

lines = []
lines.append("// AUTO-GENERATED FILE. DO NOT EDIT MANUALLY.")
lines.append("// Source: platforms/all/gamecontroller-db/gamecontrollersdl3-db.txt")
lines.append("")
lines.append("#include <stddef.h>")
lines.append('#include "RC2D/RC2D_gamecontrollerdb_embedded.h"')
lines.append("")
lines.append(f"const unsigned char {SYM}_data[{size}] = {{")

for i in range(0, size, 24):
    chunk = data[i:i+24]
    lines.append("  " + ", ".join(str(b) for b in chunk) + ",")

lines.append("};")
lines.append(f"const size_t {SYM}_size = {size};")
lines.append("")

OUT_C.write_text("\n".join(lines), encoding="utf-8")

print(f"Generated:\n- {OUT_H}\n- {OUT_C}")
