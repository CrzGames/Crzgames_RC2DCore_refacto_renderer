#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

IN_TXT = ROOT / "platforms/all/gamecontroller-db/gamecontrollersdl3-db.txt"
OUT_C  = ROOT / "src/RC2D/RC2D_gamecontrollerdb_embedded.c"

# Ces noms DOIVENT matcher tes extern dans RC2D_internal.h
SYM_DATA = "RC2D_gamecontrollerdb_data"
SYM_SIZE = "RC2D_gamecontrollerdb_size"

data = IN_TXT.read_bytes()
size = len(data)

OUT_C.parent.mkdir(parents=True, exist_ok=True)

lines = []
lines.append("// AUTO-GENERATED FILE. DO NOT EDIT MANUALLY.")
lines.append("// Source: platforms/all/gamecontroller-db/gamecontrollersdl3-db.txt")
lines.append("")
lines.append("#include <stddef.h>")
# Optionnel mais recommandé: ça force la cohérence avec tes externs
lines.append('#include "RC2D/RC2D_internal.h"')  # adapte le chemin exact si besoin
lines.append("")
lines.append(f"const unsigned char {SYM_DATA}[{size}] = {{")

for i in range(0, size, 24):
    chunk = data[i:i+24]
    lines.append("  " + ", ".join(str(b) for b in chunk) + ",")

lines.append("};")
lines.append(f"const size_t {SYM_SIZE} = {size};")
lines.append("")

OUT_C.write_text("\n".join(lines), encoding="utf-8")

print(f"Generated:\n- {OUT_C}")
