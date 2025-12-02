#!/usr/bin/env python3
import sys
import subprocess
from pathlib import Path

SLANGC = "slangc"

STAGE_INFO = {
    "cs": { "profile": "cs_6_0", "suffix": ".cs.spv" },
    "vx": { "profile": "vs_6_0", "suffix": ".vx.spv" },
    "px": { "profile": "ps_6_0", "suffix": ".px.spv" },
}

def detect_stage(shader_path: Path):
    parts = shader_path.name.split(".")
    if len(parts) >= 3:
        stage = parts[-2]
        if stage in STAGE_INFO:
            return stage
    return None

def compile_shader(shader: Path, out_dir: Path):
    stage = detect_stage(shader)
    if not stage:
        print(f"[Skip] Unknown stage in: {shader.name}")
        return

    info = STAGE_INFO[stage]
    base_name = shader.stem.rsplit(".", 1)[0]
    output_file = out_dir / (base_name + info["suffix"])

    cmd = [
        SLANGC,
        str(shader),
        "-target", "spirv",
        "-profile", info["profile"],
        "-entry", "main",
        "-o", str(output_file)
    ]

    print(f"[Slang] {stage.upper()} -> {output_file}")
    subprocess.run(cmd, check=True)

def main():
    if len(sys.argv) < 3:
        print("Usage: shader_compile.py <source_dir> <output_dir>")
        sys.exit(1)

    source_dir = Path(sys.argv[1])
    out_dir = Path(sys.argv[2])

    out_dir.mkdir(parents=True, exist_ok=True)

    for shader in source_dir.rglob("*.slang"):
        compile_shader(shader, out_dir)

if __name__ == "__main__":
    main()
