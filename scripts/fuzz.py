#!/usr/bin/env python3
"""Seed the lexer/parser corpus from functional fixtures and run libFuzzer."""

import argparse
import hashlib
import os
from pathlib import Path
import subprocess


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fuzzer", type=Path, required=True)
    parser.add_argument("--fixtures", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--seconds", type=int, default=180)
    parser.add_argument("--runs", type=int, default=0)
    args = parser.parse_args()
    if args.seconds <= 0 or args.runs < 0:
        parser.error("seconds must be positive and runs must be nonnegative")
    work = args.work.resolve()
    corpus = work / "corpus"
    artifacts = work / "artifacts"
    corpus.mkdir(parents=True, exist_ok=True)
    artifacts.mkdir(parents=True, exist_ok=True)
    seeds = list(args.fixtures.rglob("*.cw")) + list(args.fixtures.rglob("*.simp"))
    if not seeds:
        raise RuntimeError("No functional fixture seeds found")
    for path in seeds:
        data = path.read_bytes()
        (corpus / hashlib.sha256(data).hexdigest()).write_bytes(data)
    stress_seeds = [
        "start { print(" + "(" * 4000 + "1" + ")" * 4000 + ") }",
        "start { print(" + "-" * 4000 + "1) }",
        "start { print(" + "!" * 4000 + "true) }",
        "start {" + "{" * 4000 + "}" * 4000 + "}",
        "namespace N {" * 1000 + "}" * 1000,
        "start { print(" + "f(" * 1000 + "1" + ")" * 1000 + ") }",
        "start { print(" + "+".join(["1"] * 7000) + ") }",
        "start { print(value" + ".field" * 2000 + ") }",
        "start { inline(int value" + ".field" * 2000 + ") {} }",
    ]
    for source in stress_seeds:
        data = source.encode()
        (corpus / hashlib.sha256(data).hexdigest()).write_bytes(data)
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    command = [str(args.fuzzer.resolve()), str(corpus),
               f"-artifact_prefix={artifacts}/", f"-max_total_time={args.seconds}",
               "-max_len=16384", "-timeout=5", "-rss_limit_mb=2048", "-seed=1"]
    if args.runs:
        command.append(f"-runs={args.runs}")
    print(f"Seeding {len(seeds)} fixtures; artifacts: {artifacts}", flush=True)
    subprocess.run(command, env=env, check=True, timeout=args.seconds + 60)


if __name__ == "__main__":
    main()
