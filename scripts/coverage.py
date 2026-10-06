#!/usr/bin/env python3
"""Run CTest and produce LLVM coverage reports for compiler and runtime sources."""

import argparse
import json
import os
from pathlib import Path
import subprocess


def run(command: list[str], **kwargs):
    return subprocess.run(command, check=True, text=True, **kwargs)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--binary", type=Path, action="append", required=True)
    parser.add_argument("--profdata", default="llvm-profdata")
    parser.add_argument("--cov", default="llvm-cov")
    parser.add_argument("--run-tests", action="store_true")
    args = parser.parse_args()
    build = args.build.resolve()
    source = args.source.resolve()
    report = build / "coverage"
    profiles = report / "profiles"
    profiles.mkdir(parents=True, exist_ok=True)
    if args.run_tests:
        for profile in profiles.glob("*.profraw"):
            profile.unlink()
        run(["ctest", "--test-dir", str(build), "--output-on-failure", "-j4"])
    raw = sorted(profiles.glob("*.profraw"))
    if not raw:
        raise RuntimeError("No profiles: configure with SIMP_COVERAGE and run CTest first")
    profile_list = report / "profiles.list"
    profile_list.write_text("\n".join(str(path) for path in raw) + "\n")
    merged = report / "merged.profdata"
    run([args.profdata, "merge", "-sparse", "-f", str(profile_list), "-o", str(merged)])
    binaries = [path.resolve() for path in args.binary]
    # Generated integration executables carry the runtime's coverage mappings.
    for path in sorted((build / "tests").iterdir()):
        if path.is_file() and os.access(path, os.X_OK):
            with path.open("rb") as stream:
                if stream.read(4) == b"\x7fELF":
                    binaries.append(path)
    binaries = list(dict.fromkeys(binaries))
    objects = [argument for path in binaries[1:] for argument in ("-object", str(path))]
    common = [str(binaries[0]), *objects, f"-instr-profile={merged}"]
    sources = sorted(str(path) for path in source.iterdir()
                     if path.suffix in (".c", ".cpp"))
    with (report / "summary.txt").open("w") as output:
        run([args.cov, "report", *common, *sources], stdout=output)
    with (report / "coverage.json").open("w") as output:
        run([args.cov, "export", *common, *sources], stdout=output)
    run([args.cov, "show", *common, *sources, "-format=html",
         f"-output-dir={report / 'html'}", "-show-line-counts-or-regions"])
    data = json.loads((report / "coverage.json").read_text())
    files = [entry for unit in data["data"] for entry in unit["files"]
             if Path(entry["filename"]).parent == source]
    if not files:
        raise RuntimeError("Coverage report contains no compiler/runtime sources")
    files.sort(key=lambda entry: entry["summary"]["lines"]["percent"])
    print("Lowest line coverage (compiler and runtime):")
    for entry in files[:8]:
        lines = entry["summary"]["lines"]
        print(f"  {Path(entry['filename']).name}: {lines['percent']:.1f}% "
              f"({lines['covered']}/{lines['count']})")
    print(f"Reports: {report / 'summary.txt'} and {report / 'html/index.html'}")


if __name__ == "__main__":
    main()
