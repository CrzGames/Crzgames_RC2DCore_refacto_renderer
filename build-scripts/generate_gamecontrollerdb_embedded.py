#!/usr/bin/env python3
from pathlib import Path

# Description:
# Génère un fichier C embarquant la base de données des manettes SDL3
# à partir du fichier texte gamecontrollersdl3-db.txt fourni par SDL3.
# Utile pour embarquer la base de données dans l'exécutable final,
# évitant ainsi de dépendre d'un fichier externe ou que l'utilisateur doit lui-même fournir.

ROOT = Path(__file__).resolve().parents[1]

IN_TXT = ROOT / "platforms/all/gamecontroller-db/gamecontrollersdl3-db.txt"
OUT_C  = ROOT / "src/RC2D/RC2D_gamecontrollerdb_embedded.c"

# Ces noms DOIVENT matcher tes extern dans RC2D_internal.h
SYM_DATA = "rc2d_gamecontrollerdb_data"
SYM_SIZE = "rc2d_gamecontrollerdb_size"

data = IN_TXT.read_bytes()
size = len(data)

OUT_C.parent.mkdir(parents=True, exist_ok=True)

lines = []
lines.append("// AUTO-GENERATED FILE. DO NOT EDIT MANUALLY.")
lines.append("// Source: platforms/all/gamecontroller-db/gamecontrollersdl3-db.txt")
lines.append("")
lines.append("#include <stddef.h>")
lines.append('#include "RC2D/RC2D_internal.h"')
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
