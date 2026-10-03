#!/usr/bin/env python3
"""Offline integration tests for the simpkg command."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


def run(command: list[str], *, cwd: Path | None = None, env: dict | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(
        command,
        cwd=cwd,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


class SimpkgTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.git = shutil.which("git")
        if cls.git is None:
            raise RuntimeError("git is required for simpkg integration tests")

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="simpkg-test-")
        self.root = Path(self.temporary.name)
        self.project = self.root / "project"
        self.project.mkdir()
        self.bin_directory = self.root / "fake-bin"
        self.bin_directory.mkdir()
        self.remote_map: dict[str, str] = {}
        fake_git = self.bin_directory / "git"
        fake_git.write_text(
            "#!" + os.sys.executable + "\n"
            "import json, os, subprocess, sys\n"
            "args = sys.argv[1:]\n"
            "remotes = json.loads(os.environ.get('SIMPKG_TEST_GIT_REMOTES', '{}'))\n"
            "for index, argument in enumerate(args):\n"
            "    if argument.startswith('https://github.com/'):\n"
            "        if argument not in remotes:\n"
            "            print('test git: unmapped remote ' + argument, file=sys.stderr)\n"
            "            raise SystemExit(1)\n"
            "        args[index] = remotes[argument]\n"
            "raise SystemExit(subprocess.run(["
            + repr(self.git)
            + ", *args], check=False).returncode)\n",
            encoding="utf-8",
        )
        fake_git.chmod(0o755)
        self.base_environment = os.environ.copy()
        self.base_environment["PATH"] = str(self.bin_directory) + os.pathsep + self.base_environment.get("PATH", "")
        self.base_environment["SIMPKG_TEST_GIT_REMOTES"] = json.dumps(self.remote_map)
        self.base_environment.pop("XDG_CONFIG_HOME", None)
        self.base_environment["HOME"] = str(self.root / "home")
        Path(self.base_environment["HOME"]).mkdir()
        self.init_project()

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def simpkg(
        self,
        *arguments: str,
        cwd: Path | None = None,
        env: dict | None = None,
    ) -> subprocess.CompletedProcess:
        child_environment = self.base_environment.copy()
        child_environment["SIMPKG_TEST_GIT_REMOTES"] = json.dumps(self.remote_map)
        if env:
            child_environment.update(env)
        return run(
            [os.sys.executable, str(self.args.simpkg), *arguments],
            cwd=cwd or self.project,
            env=child_environment,
        )

    def init_project(self) -> None:
        result = self.simpkg("init")
        self.assertEqual(result.returncode, 0, result.stderr)

    def make_remote(
        self,
        repository: str,
        manifests: dict[str, str],
        *,
        create_source: bool = True,
    ) -> None:
        working = self.root / f"{repository}-work"
        working.mkdir()
        self.git_run(["init", "-q", str(working)])
        for version, manifest in manifests.items():
            (working / "simp-package.toml").write_text(manifest, encoding="utf-8")
            source = manifest.split('source = "', 1)[1].split('"', 1)[0]
            source_path = working / source
            source_path.parent.mkdir(parents=True, exist_ok=True)
            if create_source and not source_path.exists():
                source_path.write_text("class Greeter { int answer() { return 42 } }\n", encoding="utf-8")
            self.git_run(["-C", str(working), "add", "."])
            self.git_run(
                [
                    "-C",
                    str(working),
                    "-c",
                    "user.name=Test",
                    "-c",
                    "user.email=test@example.invalid",
                    "commit",
                    "-q",
                    "-m",
                    version,
                ]
            )
            self.git_run(["-C", str(working), "tag", version])
        bare = self.root / f"{repository}.git"
        self.git_run(["clone", "-q", "--bare", str(working), str(bare)])
        self.remote_map[f"https://github.com/acme/{repository}.git"] = bare.as_uri()

    def git_run(self, arguments: list[str]) -> None:
        result = run([self.git, *arguments])
        if result.returncode:
            raise AssertionError(f"git command failed: {result.stdout}{result.stderr}")

    @staticmethod
    def manifest(
        name: str = "greeting",
        version: str = "1.0.0",
        source: str = "greeting.simp",
        export: str = "class:Greeter",
        extras: str = "",
    ) -> str:
        return (
            "[package]\n"
            f'name = "{name}"\n'
            f'version = "{version}"\n'
            f'source = "{source}"\n'
            f'export = "{export}"\n'
            f"{extras}"
        )

    def test_init_allowlists_installed_standard_modules_and_documents_string(self) -> None:
        policy = (self.project / "modules" / "modules.toml").read_text(encoding="utf-8")
        self.assertIn('math = ["0.1.0"]', policy)
        self.assertIn('system = ["0.1.0"]', policy)
        self.assertIn("String is provided by the compiler prelude/runtime", policy)
        self.assertNotIn("\nString =", policy)

    def test_init_does_not_overwrite_existing_policy(self) -> None:
        policy_path = self.project / "modules" / "modules.toml"
        original = policy_path.read_text(encoding="utf-8")
        result = self.simpkg("init")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("refusing to overwrite", result.stderr)
        self.assertEqual(policy_path.read_text(encoding="utf-8"), original)

    def test_add_explicit_and_highest_stable_versions_preserves_fallback_order(self) -> None:
        self.make_remote(
            "greeting",
            {
                "1.0.0": self.manifest(version="1.0.0"),
                "2.0.0": self.manifest(version="2.0.0"),
                "3.0.0-rc.1": self.manifest(version="3.0.0-rc.1"),
            },
        )
        policy_path = self.project / "modules" / "modules.toml"
        policy_path.write_text(
            '[modules]\ngreeting = ["9.0.0", "8.0.0"]\nmath = ["0.1.0"]\n',
            encoding="utf-8",
        )
        explicit = self.simpkg("add", "acme/greeting", "1.0.0")
        self.assertEqual(explicit.returncode, 0, explicit.stderr)
        implicit = self.simpkg("add", "acme/greeting")
        self.assertEqual(implicit.returncode, 0, implicit.stderr)
        self.assertTrue((self.project / "modules/greeting/1.0.0/greeting.simp").is_file())
        self.assertTrue((self.project / "modules/greeting/2.0.0/greeting.simp").is_file())
        policy = policy_path.read_text(encoding="utf-8")
        self.assertIn('greeting = ["2.0.0", "1.0.0", "9.0.0", "8.0.0"]', policy)
        self.assertIn('math = ["0.1.0"]', policy)
        prerelease = self.simpkg("add", "acme/greeting", "3.0.0-rc.1")
        self.assertEqual(prerelease.returncode, 0, prerelease.stderr)
        self.assertIn(
            'greeting = ["3.0.0-rc.1", "2.0.0", "1.0.0", "9.0.0", "8.0.0"]',
            policy_path.read_text(encoding="utf-8"),
        )

    def test_add_rejects_invalid_manifests(self) -> None:
        cases = [
            ("name", self.manifest(name="not-valid"), True),
            ("version", self.manifest(version="2.0.0"), True),
            ("source", self.manifest(source="missing.simp"), False),
            ("export", self.manifest(export="method:Greeter"), True),
        ]
        for repository, manifest, create_source in cases:
            with self.subTest(repository=repository):
                self.make_remote(
                    repository,
                    {"1.0.0": manifest},
                    create_source=create_source,
                )
                result = self.simpkg("add", f"acme/{repository}", "1.0.0")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("simp-package.toml", result.stderr)
                self.assertEqual(
                    list((self.project / "modules").glob(".simpkg-*")),
                    [],
                )

    def test_add_duplicate_version_does_not_clobber_existing_files(self) -> None:
        self.make_remote("greeting", {"1.0.0": self.manifest()})
        installed = self.project / "modules/greeting/1.0.0"
        installed.mkdir(parents=True)
        (installed / "simp-package.toml").write_text(self.manifest(), encoding="utf-8")
        (installed / "greeting.simp").write_text(
            "class Greeter { int answer() { return 42 } }\n",
            encoding="utf-8",
        )
        sentinel = installed / "keep.txt"
        sentinel.write_text("keep me", encoding="utf-8")
        result = self.simpkg("add", "acme/greeting", "1.0.0")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("already installed", result.stdout)
        self.assertEqual(sentinel.read_text(encoding="utf-8"), "keep me")

    def test_failed_clone_cleans_temporary_checkout(self) -> None:
        result = self.simpkg("add", "acme/missing", "1.0.0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("could not clone", result.stderr)
        self.assertEqual(list((self.project / "modules").glob(".simpkg-*")), [])
        self.assertEqual(list((self.project / "modules").glob("missing")), [])

    def test_dependency_must_be_installed_and_allowed_before_install(self) -> None:
        self.make_remote(
            "dependent",
            {
                "1.0.0": self.manifest(
                    name="dependent",
                    extras='[dependencies]\nmissing = "=1.0.0"\n',
                )
            },
        )
        result = self.simpkg("add", "acme/dependent", "1.0.0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("required dependency missing = 1.0.0 is not allowed", result.stderr)
        self.assertFalse((self.project / "modules/dependent").exists())

    def test_env_loads_xdg_preferences_and_quotes_posix_values(self) -> None:
        xdg = self.root / "xdg"
        preferences = xdg / "simp" / "preferences.toml"
        preferences.parent.mkdir(parents=True)
        value = "single ' quote; $(touch nope)\nsecond line"
        preferences.write_text(
            "[environment]\nCUSTOM_VALUE = " + json.dumps(value) + "\n",
            encoding="utf-8",
        )
        result = self.simpkg("env", env={"XDG_CONFIG_HOME": str(xdg)})
        self.assertEqual(result.returncode, 0, result.stderr)
        shell = (
            'eval "$("$1" env)"; '
            'printf "%s\\0%s" "$CUSTOM_VALUE" "$SIMP_MODULE_DIR"'
        )
        activated = run(
            ["sh", "-c", shell, "sh", str(self.args.simpkg)],
            cwd=self.project,
            env={**self.base_environment, "XDG_CONFIG_HOME": str(xdg)},
        )
        self.assertEqual(activated.returncode, 0, activated.stderr)
        self.assertEqual(
            activated.stdout.encode(),
            value.encode() + b"\0" + str(self.project / "modules").encode(),
        )

    def test_env_explicit_preferences_and_project_module_root_override_existing_value(self) -> None:
        preferences = self.root / "custom.toml"
        preferences.write_text('[environment]\nCUSTOM_VALUE = "loaded"\n', encoding="utf-8")
        result = self.simpkg(
            "env",
            "--preferences",
            str(preferences),
            env={"SIMP_MODULE_DIR": "/unrelated"},
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("export CUSTOM_VALUE='loaded'", result.stdout)
        self.assertIn(
            f"export SIMP_MODULE_DIR='{self.project / 'modules'}'",
            result.stdout,
        )
        self.assertFalse((self.root / "not-created.toml").exists())
        missing = self.simpkg("env", "--preferences", str(self.root / "not-created.toml"))
        self.assertNotEqual(missing.returncode, 0)
        self.assertIn("does not exist", missing.stderr)

    def test_env_rejects_invalid_preferences_and_reserved_module_root(self) -> None:
        cases = [
            ("[environment\n", "cannot read TOML"),
            ('[environment]\nVALUE = 42\n', "must be a string"),
            ('[environment]\n"BAD-NAME" = "x"\n', "invalid environment variable name"),
            ('[environment]\nSIMP_MODULE_DIR = "/tmp/other"\n', "reserved"),
        ]
        for index, (content, error) in enumerate(cases):
            with self.subTest(error=error):
                path = self.root / f"invalid-{index}.toml"
                path.write_text(content, encoding="utf-8")
                result = self.simpkg("env", "--preferences", str(path))
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(error, result.stderr)

    def test_env_uses_default_home_config_path(self) -> None:
        preferences = Path(self.base_environment["HOME"]) / ".config/simp/preferences.toml"
        preferences.parent.mkdir(parents=True)
        preferences.write_text('[environment]\nFROM_HOME = "yes"\n', encoding="utf-8")
        result = self.simpkg("env")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("export FROM_HOME='yes'", result.stdout)

    def test_compiler_resolves_added_package_through_activated_environment(self) -> None:
        self.make_remote("greeting", {"1.0.0": self.manifest()})
        added = self.simpkg("add", "acme/greeting", "1.0.0")
        self.assertEqual(added.returncode, 0, added.stderr)
        source = self.project / "app.simp"
        source.write_text(
            "import greeting as Greeter\nstart { print(Greeter().answer()) }\n",
            encoding="utf-8",
        )
        shell = 'eval "$("$1" env)"; exec "$2" --check-only "$3"'
        result = run(
            [
                "sh",
                "-c",
                shell,
                "sh",
                str(self.args.simpkg),
                str(self.args.compiler),
                str(source),
            ],
            cwd=self.project,
            env=self.base_environment,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


def parse_test_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--simpkg", type=Path, required=True)
    parser.add_argument("--compiler", type=Path, required=True)
    arguments, unittest_arguments = parser.parse_known_args()
    arguments.unittest_arguments = unittest_arguments
    return arguments


if __name__ == "__main__":
    SimpkgTest.args = parse_test_arguments()
    unittest.main(argv=[__file__, *SimpkgTest.args.unittest_arguments])
