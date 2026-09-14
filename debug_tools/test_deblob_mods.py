#!/usr/bin/env python3
"""Test replacement assets through the game's real archive/factory/synthesis path.

Requires a built US game, extracted archive and graphical session. All generated
assets and settings stay in a fresh output directory; never uses installed mods.
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import zipfile

DL_PATH = "reloc_fighters_main/LinkModel/dLinkModel_Gfx_0x1D88"
PARENT = "reloc_fighters_main/LinkModel"


def make_cases(archive):
    with zipfile.ZipFile(archive) as z:
        original = z.read(DL_PATH)
        commands = list(struct.iter_unpack("<II", original[72:]))
        vtx = next(i for i, (w0, _) in enumerate(commands) if w0 >> 24 == 0x32)
        reference = commands[vtx:vtx + 2]

        def dl(cmds):
            return original[:72] + b"".join(struct.pack("<II", *c) for c in cmds)

        vertex_path = next(p for p in z.namelist() if p.startswith(PARENT + "/dLinkModel_Vtx_"))
        vertex = bytearray(z.read(vertex_path))
        assert len(vertex) == 68 + struct.unpack_from("<I", vertex, 64)[0] * 16
        vertex[68 + 12] ^= 0x20  # First vertex's red/normal-X byte, not padding.
        broken = reference.copy()
        broken[0] = (broken[0][0], 0x100000)
        # These move the vertex pointer away from every original slot. The
        # 0x98-byte case reproduces the size in the contributor's crash report.
        cases = {
            "vanilla": ({}, True, None),
            "shorter-dl": ({DL_PATH: dl([(0, 0)] * 17 + reference + [(0xDF000000, 0)])}, True, 0x98),
            "same-size-dl": ({DL_PATH: dl([(0, 0)] * 53 + reference + [(0xDF000000, 0)])}, True, 0x1B8),
            "longer-dl": ({DL_PATH: dl([(0, 0)] * 56 + reference + [(0xDF000000, 0)])}, True, 0x1D0),
            "same-size-vertex": ({vertex_path: vertex}, True, None),
            "invalid-vtx-offset": ({DL_PATH: dl(broken + [(0xDF000000, 0)])}, False, None),
            "truncated-dl": ({DL_PATH: original[:100]}, False, None),
        }
        mesh, verts = "custom/probe/mesh", "custom/probe/vertices"
        xml_vertices = '<Vertex Version="0">' + ''.join(
            f'<Vtx X="{x}" Y="{y}" Z="0" S="0" T="0" R="255" G="64" B="32" A="255"/>'
            for x, y in ((0, 0), (-50, 0), (50, 0), (0, 100))) + '</Vertex>'
        xml_mesh = (f'<DisplayList Version="0"><LoadVertices Path="{verts}" Count="3" '
                    'VertexBufferIndex="0" VertexOffset="1"/><Triangle1 V00="0" V01="1" V02="2" '
                    'Flag0="0"/><EndDisplayList/></DisplayList>')
        call = f'<DisplayList Version="0"><CallDisplayList Path="{mesh}"/><EndDisplayList/></DisplayList>'
        xml = {DL_PATH: call, mesh: xml_mesh, verts: xml_vertices}
        cases["xml-new-mesh"] = (xml, True, 16)
        cases["invalid-vtx-range"] = ({**xml, mesh: xml_mesh.replace('VertexOffset="1"', 'VertexOffset="2"')}, False, None)
        cases["empty-vertex"] = ({**xml, verts: '<Vertex Version="0"/>'}, False, None)
        cases["wrong-reference-type"] = ({**xml, mesh: xml_vertices}, False, None)
        cases["cyclic-dl"] = ({DL_PATH: call, mesh: call}, False, None)
        cases["missing-asset"] = ({DL_PATH: call}, False, None)
        return cases


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--build", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    ap.add_argument("--case", action="append")
    args = ap.parse_args()
    build, output = args.build.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    archive = build / "BattleShip.o2r"
    cases = make_cases(archive)
    if args.case and set(args.case) - cases.keys():
        ap.error("unknown case")
    results = {}
    for name, (replacements, expected, size) in cases.items():
        if args.case and name not in args.case:
            continue
        run = output / name
        app = run / "BattleShip"
        mods, traces = app / "mods", app / "debug_traces"
        mods.mkdir(parents=True)
        traces.mkdir()
        # Mods are discovered beside the executable. A real copy (not a
        # symlink resolved back to the build) isolates that path as well.
        shutil.copy2(build / "BattleShip", app / "BattleShip")
        shutil.copy2(build / "f3d.o2r", app / "f3d.o2r")
        if replacements:
            with zipfile.ZipFile(mods / "regression.o2r", "w", zipfile.ZIP_DEFLATED) as z:
                for path, data in replacements.items():
                    z.writestr(path, data)
        shutil.copy2(archive, app / archive.name)
        shutil.copy2(build / "BattleShip.o2r.recipe", app / "BattleShip.o2r.recipe")
        env = dict(os.environ, XDG_DATA_HOME=str(run), SHIP_HOME=str(app), SSB64_SYNTH_SELFTEST="1",
                   SSB64_SYNTH_INSPECT="324", SSB64_DUMP_SYNTH_RELOC_FILE_ID="324")
        with (run / "output.log").open("w") as log:
            proc = subprocess.run([str(app / "BattleShip")], cwd=app, env=env,
                                  stdout=log, stderr=subprocess.STDOUT, timeout=120)
        report = traces / "synth_verify_results.json"
        verdict = json.loads(report.read_text()) if report.exists() else {}
        files = {f["path"]: f["ok"] for f in verdict.get("files", [])}
        ok = (proc.returncode == (0 if expected else 1) and files.get(PARENT) == expected
              and len(files) == 106 and verdict.get("fail", -1) == sum(not v for v in files.values()))
        if replacements:
            ok = ok and "mounted mod archive ->" in (app / "ssb64.log").read_text()
        if expected:
            ok = ok and verdict.get("fail") == 0
        else:
            log = (app / "logs/BattleShip.log").read_text(errors="replace")
            ok = ok and "[deblob]" in log
        if expected and size is not None:
            inspect_path = traces / "synth_inspect_324.json"
            inspect = json.loads(inspect_path.read_text()) if inspect_path.exists() else {}
            s = next((s for s in inspect.get("slices", []) if s["path"] == DL_PATH), {})
            ok = ok and s.get("actual_size") == size
            # Exactly one actual pointer in this replacement; no stale vanilla
            # slots may survive or overwrite its no-ops/terminator.
            if "intern_slots" in inspect:
                slots = [x for x in inspect["intern_slots"]
                         if s["layout_offset"] <= x["slot"] < s["layout_offset"] + size]
                ok = ok and len(slots) == 1 and slots[0]["slot"] == s["layout_offset"] + size - 12
            else:
                ok = False
        results[name] = {"ok": bool(ok), "exit": proc.returncode, "expected": expected,
                         "synthesis": verdict}
        print(f"{name}: {'PASS' if ok else 'FAIL'} (exit {proc.returncode})", flush=True)
    (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return 0 if all(r["ok"] for r in results.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())
