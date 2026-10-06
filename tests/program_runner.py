"""Shared hermetic compile/run checks for examples and documentation."""

import os
from pathlib import Path
import subprocess


def environment(work: Path) -> dict[str, str]:
    env = os.environ.copy()
    for key in list(env):
        if key.startswith("SIMP_") or key in ("CC", "DESTDIR"):
            del env[key]
    for key, directory in (("HOME", "home"), ("XDG_CONFIG_HOME", "xdg")):
        path = work / directory
        path.mkdir(parents=True, exist_ok=True)
        env[key] = str(path)
    env["LC_ALL"] = "C"
    return env


def invoke(command: list[str], work: Path, env: dict[str, str],
           stdin: str = "") -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, cwd=work, env=env, input=stdin,
                            text=True, capture_output=True, timeout=30)
    if any(marker in result.stderr for marker in
           ("AddressSanitizer", "UndefinedBehaviorSanitizer", "runtime error:")):
        raise AssertionError(f"Instrumentation failure:\n{result.stderr}")
    return result


def check_program(compiler: Path, source: Path, work: Path, case: dict,
                  env: dict[str, str]) -> None:
    output = work / ("program.o" if case.get("object") else "program")
    command = [str(compiler), str(source), "-o", str(output)]
    if case.get("object"):
        command.append("-c")
    result = invoke(command, work, env)
    diagnostic = case.get("compile_diagnostic")
    if diagnostic:
        assert result.returncode != 0 and diagnostic in result.stderr, result
        return
    assert result.returncode == 0, f"{source} failed compilation:\n{result.stderr}"
    assert output.is_file(), f"No compiler output for {source}"
    if case.get("object"):
        return
    result = invoke([str(output)], work, env, case.get("stdin", ""))
    assert result.returncode == case.get("exit", 0), (
        f"{source}: exit {result.returncode}\n{result.stderr}")
    assert result.stdout == case["stdout"], (
        f"{source}: expected {case['stdout']!r}, got {result.stdout!r}")
    if "diagnostic" in case:
        assert case["diagnostic"] in result.stderr, result.stderr
    else:
        assert not result.stderr, result.stderr
