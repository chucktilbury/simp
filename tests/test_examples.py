"""Compile every checked-in example; run programs against explicit expectations."""

import argparse
import json
from pathlib import Path
import shutil
import tempfile

from program_runner import check_program, environment
from gtk_support import headless


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--examples", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--compiler", type=Path)
    parser.add_argument("--executable", type=Path)
    parser.add_argument("--work", type=Path)
    parser.add_argument("--case")
    parser.add_argument("--gtk", action="store_true")
    parser.add_argument("--sourceview", action="store_true")
    parser.add_argument("--default-shortcuts", action="store_true")
    parser.add_argument("--xvfb")
    args = parser.parse_args()
    cases = json.loads(args.manifest.read_text())
    sources = {p.relative_to(args.examples).as_posix()
               for p in args.examples.rglob("*.simp")}
    assert sources == set(cases), (
        f"Example expectations differ: missing={sources - set(cases)}, "
        f"obsolete={set(cases) - sources}")
    if args.list:
        for name, case in cases.items():
            assert case.get("requires") in (None, "gtk", "sourceview"), (
                f"{name}: unsupported requirement")
        print(";".join(sorted(name for name, case in cases.items()
                              if (args.gtk or case.get("requires") != "gtk") and
                              (args.sourceview or case.get("requires") != "sourceview"))))
        return
    args.work.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=args.work, prefix="example-") as directory:
        work = Path(directory)
        project = work / "project"
        shutil.copytree(args.examples, project)
        env = environment(work)
        case = dict(cases[args.case])
        source = project / args.case
        if case.get("requires") == "sourceview":
            (project / "editor-a.simp").write_text("start { print(\"a\") }\n")
            (project / "editor-b.simp").write_text("start { print(\"b\") }\n")
            if not args.default_shortcuts:
                (project / "tweed-shortcuts.conf").write_text(
                    "<Control><Alt>s=save\n<Control><Alt>s=open\n")
        if "driver" in case:
            source = project / "driver.simp"
            source.write_text(case["driver"])
        if args.case == "scanner.simp":
            case["stdin"] = (project / "input_test.txt").read_text()
        if case.get("requires") in ("gtk", "sourceview"):
            assert args.gtk, "GTK example was registered without SIMP_GTK"
            if case.get("requires") == "sourceview":
                assert args.sourceview, "GtkSourceView example requires SIMP_GTK_SOURCEVIEW"
            with headless(args.xvfb, work, env) as gtk_env:
                check_program(args.compiler.resolve(), source, project, case, gtk_env,
                              args.executable.resolve() if args.executable else None)
        else:
            check_program(args.compiler.resolve(), source, project, case, env)


if __name__ == "__main__":
    main()
