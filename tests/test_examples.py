"""Compile every checked-in example; run programs against explicit expectations."""

import argparse
import json
from pathlib import Path
import shutil
import tempfile

from program_runner import check_program, environment


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--examples", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--compiler", type=Path)
    parser.add_argument("--work", type=Path)
    parser.add_argument("--case")
    args = parser.parse_args()
    cases = json.loads(args.manifest.read_text())
    sources = {p.relative_to(args.examples).as_posix()
               for p in args.examples.rglob("*.simp")}
    assert sources == set(cases), (
        f"Example expectations differ: missing={sources - set(cases)}, "
        f"obsolete={set(cases) - sources}")
    if args.list:
        print(";".join(sorted(cases)))
        return
    args.work.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=args.work, prefix="example-") as directory:
        work = Path(directory)
        project = work / "project"
        shutil.copytree(args.examples, project)
        env = environment(work)
        case = dict(cases[args.case])
        source = project / args.case
        if "driver" in case:
            source = project / "driver.simp"
            source.write_text(case["driver"])
        if args.case == "scanner.simp":
            case["stdin"] = (project / "input_test.txt").read_text()
        check_program(args.compiler.resolve(), source, project, case, env)


if __name__ == "__main__":
    main()
