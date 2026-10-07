"""Extract, compile and run complete Simple programs from Markdown."""

import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from program_runner import check_program, environment
from gtk_support import headless

FENCE = re.compile(r"^```(?:simp|simple)\s*\n(.*?)^```\s*$",
                   re.MULTILINE | re.DOTALL)
MARKER = re.compile(r"^\s*// test: (.+)$", re.MULTILINE)


def programs(doc: Path):
    for path in sorted(doc.glob("*.md")):
        text = path.read_text()
        for block in FENCE.finditer(text):
            source = block.group(1)
            line = text[:block.start()].count("\n") + 1
            name = f"{path.name}:{line}"
            complete = re.search(r"^\s*start\s*\{", source, re.MULTILINE)
            fragment = re.search(r"// Fragment\b", source)
            markers = MARKER.findall(source)
            if fragment:
                assert not markers, f"{name}: fragment has a test marker"
                continue
            if not complete:
                assert not markers, f"{name}: test marker without a start block"
                assert "Complete program" not in source, f"{name}: missing start"
                continue
            assert len(markers) == 1, f"{name}: complete program needs one // test: JSON marker"
            case = json.loads(markers[0])
            assert isinstance(case.get("stdout"), str), f"{name}: missing stdout expectation"
            yield name, source, case


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--doc", type=Path, required=True)
    parser.add_argument("--fixtures", type=Path, required=True)
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--gtk", action="store_true")
    parser.add_argument("--sourceview", action="store_true")
    parser.add_argument("--xvfb")
    args = parser.parse_args()
    cases = list(programs(args.doc))
    assert cases, "No complete documentation programs found"
    args.work.mkdir(parents=True, exist_ok=True)
    for name, source, expectation in cases:
        requirement = expectation.get("requires")
        assert requirement in (None, "gtk", "sourceview"), (
            f"{name}: unsupported requirement {requirement}")
        if requirement in ("gtk", "sourceview") and not args.gtk:
            print(f"NOT CONFIGURED optional GTK program {name} (SIMP_GTK=OFF)")
            continue
        if requirement == "sourceview" and not args.sourceview:
            print(f"NOT CONFIGURED optional GtkSourceView program {name} "
                  "(SIMP_GTK_SOURCEVIEW=OFF)")
            continue
        with tempfile.TemporaryDirectory(dir=args.work, prefix="doc-") as directory:
            work = Path(directory)
            project = work / "project"
            project.mkdir()
            if "fixture" in expectation:
                fixture = expectation["fixture"]
                assert re.fullmatch(r"[a-zA-Z0-9_-]+", fixture), fixture
                shutil.copytree(args.fixtures / fixture, project, dirs_exist_ok=True)
            case = dict(expectation)
            case["stdout"] = case["stdout"].replace("<WORK_DIR>", str(project))
            program = project / "example.simp"
            program.write_text(source)
            try:
                if requirement in ("gtk", "sourceview"):
                    with headless(args.xvfb, work, environment(work)) as env:
                        check_program(args.compiler.resolve(), program, project, case, env)
                else:
                    check_program(args.compiler.resolve(), program, project, case, environment(work))
            except (AssertionError, subprocess.TimeoutExpired) as error:
                raise AssertionError(f"{name}: {error}") from error
            print(f"PASS {name}")
    print(f"Checked {len(cases)} documentation programs")


if __name__ == "__main__":
    main()
