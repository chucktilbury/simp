#!/usr/bin/env python3
"""Offline integration tests for the simpkg command."""

from __future__ import annotations

import argparse
import hashlib
import importlib.machinery
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import tomllib
import unittest
from unittest import mock


def run(command: list[str], *, cwd: Path | None = None, env: dict | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(
        command,
        cwd=cwd,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.DEVNULL,
        timeout=30,
        check=False,
    )


class SimpkgTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.git = shutil.which("git")
        if cls.git is None:
            raise RuntimeError("git is required for simpkg integration tests")

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix=".simpkg-test-", dir=Path.cwd())
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
            "with open(os.environ['SIMPKG_TEST_GIT_LOG'], 'a') as log:\n"
            "    log.write(json.dumps(args) + '\\n')\n"
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
        self.git_log = self.root / "git.log"
        self.base_environment["SIMPKG_TEST_GIT_LOG"] = str(self.git_log)
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

    def legacy_project(self) -> None:
        (self.project / "simpkg.toml").unlink()
        (self.project / "simpkg.lock").unlink()
        (self.project / "modules/modules.toml").write_text(
            '[modules]\nmath = ["0.1.0"]\nsystem = ["0.1.0"]\n',
            encoding="utf-8",
        )

    def lock(self) -> dict:
        return tomllib.loads((self.project / "simpkg.lock").read_text())

    def project_manifest(self) -> dict:
        return tomllib.loads((self.project / "simpkg.toml").read_text())

    def script_module(self):
        loader = importlib.machinery.SourceFileLoader("simpkg_test_subject", str(self.args.simpkg))
        spec = importlib.util.spec_from_loader(loader.name, loader)
        module = importlib.util.module_from_spec(spec)
        loader.exec_module(module)
        return module

    def make_remote(
        self,
        repository: str,
        manifests: dict[str, str],
        *,
        create_source: bool = True,
        source_content: str = "class Greeter { int answer() { return 42 } }\n",
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
                source_path.write_text(source_content, encoding="utf-8")
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

    def test_init_creates_manifest_and_complete_builtin_lock(self) -> None:
        self.assertEqual(self.project_manifest(), {"schema": 1, "sources": {}})
        self.assertFalse((self.project / "modules/modules.toml").exists())
        lock = self.lock()
        self.assertEqual(lock["schema"], 1)
        self.assertEqual(lock["modules"]["math"], ["0.1.0"])
        self.assertEqual(lock["modules"]["system"], ["0.1.0"])
        self.assertNotIn("String", lock["modules"])
        for entry in lock["packages"].values():
            self.assertEqual(entry["repo"], "builtin")
            self.assertEqual(entry["ref"], "builtin")
            self.assertEqual(len(entry["sha256"]), 64)
        verified = self.simpkg("install")
        self.assertEqual(verified.returncode, 0, verified.stderr)
        
    def test_init_allowlists_installed_standard_modules_and_documents_string(self) -> None:
        policy = (self.project / "modules" / "modules.toml").read_text(encoding="utf-8")
        self.assertIn('math = ["0.1.0"]', policy)
        self.assertIn('system = ["0.1.0"]', policy)
        self.assertIn("String is provided by the compiler builtin/runtime", policy)
        self.assertNotIn("\nString =", policy)
        source = self.project / "string.simp"
        source.write_text(
            "class Text : String { Text() { super String() } }\n"
            "start {\n"
            "String value = Text()\n"
            'value.append("built in")\n'
            "print(value)\n"
            "}\n",
            encoding="utf-8",
        )
        executable = self.project / "string-program"
        compiled = run(
            [str(self.args.compiler), str(source), "-o", str(executable)],
            cwd=self.project,
            env=self.base_environment,
        )
        self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
        result = run([str(executable)], cwd=self.project, env=self.base_environment)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "built in")

    def test_init_does_not_overwrite_existing_policy(self) -> None:
        policy_path = self.project / "simpkg.toml"
        original = policy_path.read_text(encoding="utf-8")
        result = self.simpkg("init")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("refusing to overwrite", result.stderr)
        self.assertEqual(policy_path.read_text(encoding="utf-8"), original)

    def test_multiple_bundled_versions_select_highest_stable_singleton(self) -> None:
        module = self.script_module()
        stdlib = self.root / "stdlib"
        shutil.copytree(module.STDLIB_DIRECTORY, stdlib)
        for version in ("1.0.0", "2.0.0", "3.0.0-rc.1"):
            root = stdlib / "bundled" / version
            root.mkdir(parents=True)
            (root / "simp-package.toml").write_text(self.manifest(name="bundled", version=version))
            (root / "greeting.simp").write_text("class Greeter {}\n")
        project = self.root / "multiple-builtin-project"
        with mock.patch.object(module, "STDLIB_DIRECTORY", stdlib):
            module.command_init(str(project))
            lock_text = (project / "simpkg.lock").read_text()
            lock = tomllib.loads(lock_text)
            self.assertEqual(lock["modules"]["bundled"], ["2.0.0"])
            self.assertTrue(all(len(versions) == 1 for versions in lock["modules"].values()))
            self.assertIn("bundled", lock["packages"])
            self.assertIn("[packages.bundled]", lock_text)
            self.assertNotIn("@", lock_text)
            packages = module.read_lock(project, module.read_project(project))
            self.assertEqual(module.inspect_installed(project, packages), [])
            document = {"dependencies": {"top": {"repo": "acme/top", "version": "=1.0.0"}}, "sources": {}}
            root = self.root / "builtin-dependent"
            root.mkdir()
            (root / "simp-package.toml").write_text(self.manifest(
                name="top", extras='[dependencies]\nbundled = "=1.0.0"\n'))
            (root / "greeting.simp").write_text("class Greeter {}\n")
            manifest = module.read_manifest(root, check_directory_version=False)
            with self.assertRaisesRegex(module.SimpkgError, "exact version conflict"):
                module.resolve_graph(document, self.root, seed=("top", manifest, root, "1" * 40), yes=True)
        if self.args.compiler is not None:
            source = project / "app.simp"
            source.write_text('start { print("singleton lock") }\n')
            environment = self.base_environment.copy()
            environment.pop("SIMP_MODULE_DIR", None)
            compiled = run([str(self.args.compiler), "--check-only", str(source)],
                           cwd=self.root, env=environment)
            self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
        shutil.rmtree(stdlib / "bundled/1.0.0")
        shutil.rmtree(stdlib / "bundled/2.0.0")
        with mock.patch.object(module, "STDLIB_DIRECTORY", stdlib):
            with self.assertRaisesRegex(module.SimpkgError, "no stable SemVer version"):
                module.command_init(str(self.root / "unstable-builtin-project"))

    def test_init_validates_builtin_dependency_edges_before_writing_lock(self) -> None:
        module = self.script_module()
        stdlib = self.root / "builtin-deps"
        for name, extras in (("base", ""), ("dependent", '[dependencies]\nbase = "=1.0.0"\n')):
            root = stdlib / name / "1.0.0"
            root.mkdir(parents=True)
            (root / "simp-package.toml").write_text(self.manifest(name=name, extras=extras))
            (root / "greeting.simp").write_text("class Greeter {}\n")
        project = self.root / "builtin-deps-project"
        with mock.patch.object(module, "STDLIB_DIRECTORY", stdlib):
            module.command_init(str(project))
            packages = module.read_lock(project, module.read_project(project))
            self.assertEqual(packages["dependent"]["dependencies"], ["base=1.0.0"])
            self.assertEqual(module.inspect_installed(project, packages), [])
            manifest_path = stdlib / "dependent/1.0.0/simp-package.toml"
            manifest_path.write_text(self.manifest(name="dependent", extras='[dependencies]\nbase = "=2.0.0"\n'))
            invalid = self.root / "invalid-builtin-deps-project"
            with self.assertRaisesRegex(module.SimpkgError, "missing dependency base=2.0.0"):
                module.command_init(str(invalid))
            self.assertFalse((invalid / "simpkg.toml").exists())
            self.assertFalse((invalid / "simpkg.lock").exists())

    def test_add_explicit_and_highest_stable_versions_preserves_fallback_order(self) -> None:
        self.legacy_project()
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
        explicit = self.simpkg("add", "acme/greeting", "1.0.0", "--yes")
        self.assertEqual(explicit.returncode, 0, explicit.stderr)
        implicit = self.simpkg("add", "acme/greeting", "--yes")
        self.assertEqual(implicit.returncode, 0, implicit.stderr)
        self.assertTrue((self.project / "modules/greeting/1.0.0/greeting.simp").is_file())
        self.assertTrue((self.project / "modules/greeting/2.0.0/greeting.simp").is_file())
        policy = policy_path.read_text(encoding="utf-8")
        self.assertIn('greeting = ["2.0.0", "1.0.0", "9.0.0", "8.0.0"]', policy)
        self.assertIn('math = ["0.1.0"]', policy)
        prerelease = self.simpkg("add", "acme/greeting", "3.0.0-rc.1", "--yes")
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
                result = self.simpkg("add", f"acme/{repository}", "1.0.0", "--yes")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("simp-package.toml", result.stderr)
                self.assertEqual(
                    list((self.project / "modules").glob(".simpkg-*")),
                    [],
                )

    def test_add_duplicate_version_does_not_clobber_existing_files(self) -> None:
        self.legacy_project()
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
        result = self.simpkg("add", "acme/greeting", "1.0.0", "--yes")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("already installed", result.stdout)
        self.assertEqual(sentinel.read_text(encoding="utf-8"), "keep me")

    def test_failed_clone_cleans_temporary_checkout(self) -> None:
        result = self.simpkg("add", "acme/missing", "1.0.0", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("could not clone", result.stderr)
        self.assertEqual(list((self.project / "modules").glob(".simpkg-*")), [])
        self.assertEqual(list((self.project / "modules").glob("missing")), [])

    def test_dependency_must_be_installed_and_allowed_before_install(self) -> None:
        self.legacy_project()
        self.make_remote(
            "dependent",
            {
                "1.0.0": self.manifest(
                    name="dependent",
                    extras='[dependencies]\nmissing = "=1.0.0"\n',
                )
            },
        )
        result = self.simpkg("add", "acme/dependent", "1.0.0", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("required dependency missing = 1.0.0 is not allowed", result.stderr)
        self.assertFalse((self.project / "modules/dependent").exists())

    def test_network_requires_consent_and_offline_dry_run_does_not_fetch(self) -> None:
        for arguments in (
            ("add", "acme/unknown", "1.0.0"),
            ("add", "acme/unknown"),
            ("add", "acme/unknown", "--dry-run"),
        ):
            with self.subTest(arguments=arguments):
                result = self.simpkg(*arguments)
                self.assertIn("Initial plan", result.stdout)
                if "--dry-run" in arguments:
                    self.assertEqual(result.returncode, 0, result.stderr)
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn("consent", result.stderr)
                self.assertFalse(self.git_log.exists())
        self.legacy_project()
        result = self.simpkg("add", "acme/unknown")
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(self.git_log.exists())

    def create_graph(self) -> None:
        self.make_remote("leaf", {"1.0.0": self.manifest(name="leaf")})
        self.make_remote("middle", {"1.0.0": self.manifest(
            name="middle",
            extras='[dependencies]\nleaf = "=1.0.0"\n[sources]\nleaf = "acme/leaf"\n',
        )})
        self.make_remote("top", {"1.0.0": self.manifest(
            name="top",
            extras='[dependencies]\nmiddle = "=1.0.0"\n[sources]\nmiddle = "acme/middle"\n',
        )})

    def test_transitive_graph_direct_manifest_and_deterministic_lock(self) -> None:
        self.create_graph()
        result = self.simpkg("add", "acme/top", "1.0.0", "--yes")
        self.assertEqual(result.returncode, 0, result.stderr)
        manifest = self.project_manifest()
        self.assertEqual(manifest["dependencies"], {"top": {"repo": "acme/top", "version": "=1.0.0"}})
        lock = self.lock()
        self.assertEqual(lock["manifest-sha256"], hashlib.sha256(
            (self.project / "simpkg.toml").read_bytes()).hexdigest())
        for name in ("leaf", "middle", "top"):
            entry = lock["packages"][name]
            self.assertEqual(entry["repo"], f"acme/{name}")
            self.assertEqual(entry["ref"], "refs/tags/1.0.0")
            self.assertEqual(len(entry["commit"]), 40)
            self.assertEqual(lock["modules"][name], ["1.0.0"])
            self.assertTrue((self.project / f"modules/{name}/1.0.0/greeting.simp").is_file())
        self.assertEqual(lock["packages"]["top"]["dependencies"], ["middle=1.0.0"])
        self.assertEqual(lock["packages"]["middle"]["dependencies"], ["leaf=1.0.0"])
        self.assertLess(result.stdout.index("Transitive source plan: leaf=1.0.0"),
                        result.stdout.index("Fetch acme/leaf"))
        text = (self.project / "simpkg.lock").read_bytes()
        self.assertIn(b"[packages.top]", text)
        self.assertNotIn(b'[packages."top"]', text)
        self.assertIn(b'top = ["1.0.0"]', text)
        self.assertIn("[dependencies.top]", (self.project / "simpkg.toml").read_text())
        again = self.simpkg("add", "acme/top", "1.0.0", "--yes")
        self.assertEqual(again.returncode, 0, again.stderr)
        self.assertEqual((self.project / "simpkg.lock").read_bytes(), text)
        graph = result.stdout.split("Resolved graph:\n")[1].split("Installed graph")[0]
        self.assertEqual(graph.splitlines(), sorted(graph.splitlines()))
        self.assertFalse((self.project / "modules/modules.toml").exists())

    def test_dry_run_with_consent_resolves_without_persistent_writes(self) -> None:
        self.create_graph()
        before = {path.relative_to(self.project): path.read_bytes() for path in self.project.rglob("*") if path.is_file()}
        result = self.simpkg("add", "acme/top", "--yes", "--dry-run")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("leaf=1.0.0", result.stdout)
        after = {path.relative_to(self.project): path.read_bytes() for path in self.project.rglob("*") if path.is_file()}
        self.assertEqual(before, after)
        self.assertEqual(list((self.project / "modules").iterdir()), [])
        self.assertEqual(list(self.project.glob(".simpkg-*")), [])

    def test_unmapped_dependency_never_fetches_an_inferred_repository(self) -> None:
        self.make_remote("top", {"1.0.0": self.manifest(
            name="top", extras='[dependencies]\nunknown = "=1.0.0"\n',
        )})
        before = (self.project / "simpkg.lock").read_bytes()
        result = self.simpkg("add", "acme/top", "1.0.0", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unmapped dependency unknown=1.0.0", result.stderr)
        self.assertNotIn("acme/unknown", self.git_log.read_text())
        self.assertEqual((self.project / "simpkg.lock").read_bytes(), before)
        self.assertEqual(list((self.project / "modules").iterdir()), [])

    def test_project_sources_map_package_dependencies(self) -> None:
        self.make_remote("leaf", {"1.0.0": self.manifest(name="leaf")})
        self.make_remote("top", {"1.0.0": self.manifest(
            name="top", extras='[dependencies]\nleaf = "=1.0.0"\n',
        )})
        (self.project / "simpkg.toml").write_text('schema = 1\n[sources]\nleaf = "acme/leaf"\n')
        (self.project / "simpkg.lock").unlink()
        result = self.simpkg("add", "acme/top", "1.0.0", "--yes")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.lock()["packages"]["leaf"]["repo"], "acme/leaf")

    def test_graph_conflicts_and_cycles_roll_back(self) -> None:
        self.make_remote("leaf", {
            "1.0.0": self.manifest(name="leaf"),
            "2.0.0": self.manifest(name="leaf", version="2.0.0"),
        })
        self.make_remote("middle", {"1.0.0": self.manifest(
            name="middle", extras='[dependencies]\nleaf = "=2.0.0"\n[sources]\nleaf = "acme/leaf"\n',
        )})
        self.make_remote("conflict", {"1.0.0": self.manifest(
            name="conflict",
            extras='[dependencies]\nleaf = "=1.0.0"\nmiddle = "=1.0.0"\n[sources]\nleaf = "acme/leaf"\nmiddle = "acme/middle"\n',
        )})
        self.make_remote("cycle", {"1.0.0": self.manifest(
            name="cycle", extras='[dependencies]\ncycle = "=1.0.0"\n[sources]\ncycle = "acme/cycle"\n',
        )})
        self.make_remote("repo_conflict", {"1.0.0": self.manifest(
            name="repo_conflict",
            extras='[dependencies]\nleaf = "=1.0.0"\n[sources]\nleaf = "acme/different"\n',
        )})
        for repo, error in (("conflict", "conflict"), ("cycle", "cycle"), ("repo_conflict", "repository conflict")):
            with self.subTest(repo=repo):
                (self.project / "simpkg.toml").write_text('schema = 1\n[sources]\nleaf = "acme/leaf"\n')
                (self.project / "simpkg.lock").unlink(missing_ok=True)
                result = self.simpkg("add", f"acme/{repo}", "1.0.0", "--yes")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(error, result.stderr)
                self.assertFalse((self.project / "simpkg.lock").exists())
                self.assertEqual(list((self.project / "modules").iterdir()), [])
        self.assertNotIn("acme/different", self.git_log.read_text())

    def test_new_add_selects_highest_stable_exact_version(self) -> None:
        self.make_remote("greeting", {
            "1.0.0": self.manifest(),
            "2.0.0": self.manifest(version="2.0.0"),
            "3.0.0-rc.1": self.manifest(version="3.0.0-rc.1"),
        })
        result = self.simpkg("add", "acme/greeting", "--yes")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.project_manifest()["dependencies"]["greeting"]["version"], "=2.0.0")
        self.assertEqual(self.lock()["modules"]["greeting"], ["2.0.0"])
        for version in ("bad", "01.0.0", "1.0.0-01"):
            self.git_log.unlink(missing_ok=True)
            bad = self.simpkg("add", "acme/greeting", version, "--yes")
            self.assertNotEqual(bad.returncode, 0)
            self.assertIn("SemVer", bad.stderr)
            self.assertFalse(self.git_log.exists())

    def test_frozen_install_restores_commit_without_moving_tag_and_preserves_lock(self) -> None:
        self.make_remote("greeting", {"1.0.0": self.manifest()})
        added = self.simpkg("add", "acme/greeting", "1.0.0", "--yes")
        self.assertEqual(added.returncode, 0, added.stderr)
        entry = self.lock()["packages"]["greeting"]
        manifest_bytes = (self.project / "simpkg.toml").read_bytes()
        lock_bytes = (self.project / "simpkg.lock").read_bytes()
        working = self.root / "greeting-work"
        (working / "greeting.simp").write_text("changed tag contents\n")
        self.git_run(["-C", str(working), "add", "."])
        self.git_run(["-C", str(working), "-c", "user.name=Test", "-c", "user.email=test@example.invalid",
                      "commit", "-q", "-m", "move tag"])
        self.git_run(["-C", str(working), "tag", "-f", "1.0.0"])
        self.git_run(["-C", str(working), "push", "-q", "--force", str(self.root / "greeting.git"), "refs/tags/1.0.0"])
        shutil.rmtree(self.project / "modules/greeting/1.0.0")
        denied = self.simpkg("install")
        self.assertNotEqual(denied.returncode, 0)
        self.assertIn("consent", denied.stderr)
        self.git_log.unlink()
        restored = self.simpkg("install", "--yes")
        self.assertEqual(restored.returncode, 0, restored.stderr)
        self.assertIn(entry["commit"], self.git_log.read_text())
        self.assertEqual((self.project / "modules/greeting/1.0.0/greeting.simp").read_text(),
                         "class Greeter { int answer() { return 42 } }\n")
        self.assertEqual((self.project / "simpkg.toml").read_bytes(), manifest_bytes)
        self.assertEqual((self.project / "simpkg.lock").read_bytes(), lock_bytes)
        self.git_log.unlink()
        verified = self.simpkg("install")
        self.assertEqual(verified.returncode, 0, verified.stderr)
        self.assertFalse(self.git_log.exists())

    def test_integrity_mismatch_never_overwrites_installed_files(self) -> None:
        self.make_remote("greeting", {"1.0.0": self.manifest()})
        added = self.simpkg("add", "acme/greeting", "1.0.0", "--yes")
        self.assertEqual(added.returncode, 0, added.stderr)
        source = self.project / "modules/greeting/1.0.0/greeting.simp"
        source.write_text("tampered\n")
        lock_bytes = (self.project / "simpkg.lock").read_bytes()
        self.git_log.unlink()
        result = self.simpkg("install", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("integrity mismatch", result.stderr)
        self.assertFalse(self.git_log.exists())
        result = self.simpkg("add", "acme/greeting", "1.0.0", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("refusing to overwrite", result.stderr)
        self.assertEqual(source.read_text(), "tampered\n")
        self.assertEqual((self.project / "simpkg.lock").read_bytes(), lock_bytes)

    def test_lock_schemas_staleness_metadata_and_hash_tampering_reject_offline(self) -> None:
        lock_path = self.project / "simpkg.lock"
        original = lock_path.read_text()
        manifest_path = self.project / "simpkg.toml"
        original_manifest = manifest_path.read_text()
        for content, error in (
            (original.replace("schema = 1", "schema = 2"), "schema"),
            (original.replace("schema = 1", "schema = true"), "schema"),
            (original.replace('math = ["0.1.0"]', 'math = ["9.0.0"]'), "modules"),
            (original.replace('repo = "builtin"', 'repo = "acme/evil"', 1), "commit"),
            (original.replace('dependencies = []', 'dependencies = ["missing=1.0.0"]', 1), "missing dependency"),
            (original.replace(self.lock()["packages"]["math"]["sha256"], "0" * 64), "integrity mismatch"),
        ):
            with self.subTest(error=error):
                lock_path.write_text(content)
                result = self.simpkg("install", "--yes")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(error, result.stderr)
                self.assertFalse(self.git_log.exists())
                lock_path.write_text(original)
        manifest_path.write_text(original_manifest + "\n")
        stale = self.simpkg("install", "--yes")
        self.assertNotEqual(stale.returncode, 0)
        self.assertIn("stale lock", stale.stderr)
        stale_add = self.simpkg("add", "acme/unknown", "1.0.0", "--yes")
        self.assertNotEqual(stale_add.returncode, 0)
        self.assertIn("stale lock", stale_add.stderr)
        self.assertFalse(self.git_log.exists())
        manifest_path.write_text("schema = 2\n")
        invalid = self.simpkg("install")
        self.assertNotEqual(invalid.returncode, 0)
        self.assertIn("manifest schema", invalid.stderr)

    def test_install_without_lock_requires_consent_then_resolves(self) -> None:
        self.make_remote("greeting", {"1.0.0": self.manifest()})
        (self.project / "simpkg.lock").unlink()
        content = 'schema = 1\n[dependencies.greeting]\nrepo = "acme/greeting"\nversion = "=1.0.0"\n'
        (self.project / "simpkg.toml").write_text(content)
        denied = self.simpkg("install")
        self.assertNotEqual(denied.returncode, 0)
        self.assertFalse(self.git_log.exists())
        dry = self.simpkg("install", "--dry-run", "--yes")
        self.assertEqual(dry.returncode, 0, dry.stderr)
        self.assertFalse((self.project / "simpkg.lock").exists())
        self.assertFalse((self.project / "modules/greeting").exists())
        installed = self.simpkg("install", "--yes")
        self.assertEqual(installed.returncode, 0, installed.stderr)
        self.assertEqual((self.project / "simpkg.toml").read_text(), content)
        self.assertEqual(self.lock()["packages"]["greeting"]["version"], "1.0.0")

    def test_nested_directory_discovers_project_root(self) -> None:
        nested = self.project / "src/nested"
        nested.mkdir(parents=True)
        again = self.simpkg("init", cwd=nested)
        self.assertNotEqual(again.returncode, 0)
        self.assertIn("refusing to overwrite", again.stderr)
        self.make_remote("greeting", {"1.0.0": self.manifest()})
        added = self.simpkg("add", "acme/greeting", "1.0.0", "--yes", cwd=nested)
        self.assertEqual(added.returncode, 0, added.stderr)
        for command in ("install", "list"):
            result = self.simpkg(command, cwd=nested)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("greeting", result.stdout if command == "list" else self.simpkg("list", cwd=nested).stdout)
        self.assertFalse((nested / "modules").exists())
        env = self.simpkg("env", cwd=nested)
        self.assertIn(str(self.project / "modules"), env.stdout)

    def test_symlinks_and_existing_untracked_content_are_rejected(self) -> None:
        self.make_remote("greeting", {"1.0.0": self.manifest()})
        destination = self.project / "modules/greeting/1.0.0"
        destination.mkdir(parents=True)
        sentinel = destination / "keep"
        sentinel.write_text("do not overwrite\n")
        refused = self.simpkg("add", "acme/greeting", "1.0.0", "--yes")
        self.assertNotEqual(refused.returncode, 0)
        self.assertIn("integrity mismatch", refused.stderr)
        self.assertEqual(sentinel.read_text(), "do not overwrite\n")
        shutil.rmtree(destination)
        outside = self.root / "outside"
        outside.mkdir()
        destination.symlink_to(outside, target_is_directory=True)
        refused = self.simpkg("add", "acme/greeting", "1.0.0", "--yes")
        self.assertNotEqual(refused.returncode, 0)
        self.assertIn("unsafe", refused.stderr)
        self.assertEqual(list(outside.iterdir()), [])
        destination.unlink()
        destination.parent.rmdir()
        destination.parent.symlink_to(outside, target_is_directory=True)
        parent_escape = self.simpkg("add", "acme/greeting", "1.0.0", "--yes")
        self.assertNotEqual(parent_escape.returncode, 0)
        self.assertIn("unsafe", parent_escape.stderr)
        self.assertEqual(list(outside.iterdir()), [])

    def test_checkout_symlink_rejected_without_install(self) -> None:
        self.make_remote("unsafe", {"1.0.0": self.manifest(name="unsafe")})
        working = self.root / "unsafe-work"
        (working / "escape").symlink_to("../../outside")
        self.git_run(["-C", str(working), "add", "."])
        self.git_run(["-C", str(working), "-c", "user.name=Test", "-c", "user.email=test@example.invalid",
                      "commit", "-q", "-m", "unsafe"])
        self.git_run(["-C", str(working), "tag", "-f", "1.0.0"])
        self.git_run(["-C", str(working), "push", "-q", "--force", str(self.root / "unsafe.git"), "refs/tags/1.0.0"])
        result = self.simpkg("add", "acme/unsafe", "1.0.0", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unsafe package entry", result.stderr)
        self.assertEqual(list((self.project / "modules").iterdir()), [])

    def test_tree_hash_contract_and_rejection_of_git_and_special_entries(self) -> None:
        module = self.script_module()
        root = self.root / "tree"
        root.mkdir()
        (root / "z").write_bytes(b"last")
        (root / "a").write_bytes(b"first")
        digest = hashlib.sha256()
        for name, content in ((b"a", b"first"), (b"z", b"last")):
            digest.update(name)
            digest.update(b"\0")
            digest.update(content)
            digest.update(b"\0")
        self.assertEqual(module.tree_digest(root), digest.hexdigest())
        (root / "a").chmod(0o755)
        (root / "empty").mkdir()
        self.assertEqual(module.tree_digest(root), digest.hexdigest())
        git_dir = root / "empty/.git"
        git_dir.mkdir()
        with self.assertRaisesRegex(module.SimpkgError, "unsafe package entry"):
            module.tree_digest(root)
        git_dir.rmdir()
        fifo = root / "pipe"
        os.mkfifo(fifo)
        with self.assertRaisesRegex(module.SimpkgError, "unsafe package entry"):
            module.tree_digest(root)
        fifo.unlink()
        (root / "link").symlink_to("a")
        with self.assertRaisesRegex(module.SimpkgError, "unsafe package entry"):
            module.tree_digest(root)

    def test_interactive_graph_requires_specific_transitive_repository_approval(self) -> None:
        module = self.script_module()
        root = self.root / "top"
        root.mkdir()
        (root / "simp-package.toml").write_text(self.manifest(
            name="top", extras='[dependencies]\nleaf = "=1.0.0"\n[sources]\nleaf = "acme/leaf"\n'))
        (root / "greeting.simp").write_text("class Greeter {}\n")
        manifest = module.read_manifest(root, check_directory_version=False)
        document = {"dependencies": {"top": {"repo": "acme/top", "version": "=1.0.0"}}, "sources": {}}
        with mock.patch.object(module.sys.stdin, "isatty", return_value=True), \
                mock.patch("builtins.input", return_value="n") as prompt, \
                mock.patch.object(module, "checkout_package") as checkout:
            with self.assertRaisesRegex(module.SimpkgError, "explicit network consent"):
                module.resolve_graph(document, self.root, seed=("top", manifest, root, "1" * 40))
            checkout.assert_not_called()
            self.assertIn("acme/leaf", prompt.call_args.args[0])
    def test_atomic_configuration_failure_rolls_back_packages_and_manifest(self) -> None:
        module = self.script_module()
        packages = self.lock()["packages"]
        staged_root = self.root / "staged"
        staged_root.mkdir()
        (staged_root / "simp-package.toml").write_text(self.manifest())
        (staged_root / "greeting.simp").write_text("class Greeter {}\n")
        manifest = module.read_manifest(staged_root, check_directory_version=False)
        packages["greeting"] = module.package_record(manifest, "acme/greeting", "1" * 40, module.tree_digest(staged_root))
        original_manifest = (self.project / "simpkg.toml").read_bytes()
        original_lock = (self.project / "simpkg.lock").read_bytes()
        replace = module.os.replace
        failed = False

        def fail_once(source, destination):
            nonlocal failed
            if Path(destination).name == "simpkg.lock" and not failed:
                failed = True
                raise OSError("injected lock write failure")
            return replace(source, destination)

        with mock.patch.object(module.os, "replace", side_effect=fail_once):
            with self.assertRaisesRegex(OSError, "injected lock write failure"):
                module.persist(self.project, packages, {"greeting": staged_root},
                               'schema = 1\n[dependencies.greeting]\nrepo = "acme/greeting"\nversion = "=1.0.0"\n')
        self.assertEqual((self.project / "simpkg.toml").read_bytes(), original_manifest)
        self.assertEqual((self.project / "simpkg.lock").read_bytes(), original_lock)
        self.assertFalse((self.project / "modules/greeting").exists())
        self.assertEqual(list(self.project.glob(".simpkg-write-*")), [])

    def test_frozen_restore_rejects_tampered_hash_without_installing(self) -> None:
        self.make_remote("greeting", {"1.0.0": self.manifest()})
        added = self.simpkg("add", "acme/greeting", "1.0.0", "--yes")
        self.assertEqual(added.returncode, 0, added.stderr)
        lock = self.lock()
        path = self.project / "simpkg.lock"
        tampered = path.read_text().replace(lock["packages"]["greeting"]["sha256"], "0" * 64)
        path.write_text(tampered)
        shutil.rmtree(self.project / "modules/greeting")
        result = self.simpkg("install", "--yes")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("integrity mismatch", result.stderr)
        self.assertFalse((self.project / "modules/greeting").exists())
        self.assertEqual(path.read_text(), tampered)
        self.assertEqual(list(self.project.glob(".simpkg-*")), [])

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

    def test_compiler_resolves_added_package_without_environment_from_nested_absolute_source(self) -> None:
        self.make_remote("greeting", {"1.0.0": self.manifest()})
        added = self.simpkg("add", "acme/greeting", "1.0.0", "--yes")
        self.assertEqual(added.returncode, 0, added.stderr)
        nested = self.project / "src/nested"
        nested.mkdir(parents=True)
        source = nested / "app.simp"
        source.write_text(
            "import greeting as Greeter\nstart { print(Greeter().answer()) }\n",
            encoding="utf-8",
        )
        environment = self.base_environment.copy()
        environment.pop("SIMP_MODULE_DIR", None)
        result = run([str(self.args.compiler), "--check-only", str(source)],
                     cwd=self.root, env=environment)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_compiler_unaliased_class_export_and_explicit_policy_overrides(self) -> None:
        self.make_remote("greeting", {"1.0.0": self.manifest()})
        added = self.simpkg("add", "acme/greeting", "1.0.0", "--yes")
        self.assertEqual(added.returncode, 0, added.stderr)
        source = self.project / "app.simp"
        source.write_text("import greeting\nstart { print(Greeter().answer()) }\n")
        environment = self.base_environment.copy()
        environment.pop("SIMP_MODULE_DIR", None)
        compiler = str(self.args.compiler)
        default = run([compiler, "--check-only", str(source)], cwd=self.root, env=environment)
        self.assertEqual(default.returncode, 0, default.stdout + default.stderr)
        independent = self.root / "independent-modules"
        independent.mkdir()
        (independent / "modules.toml").write_text('[modules]\ngreeting = ["1.0.0"]\n')
        shutil.copytree(self.project / "modules/greeting", independent / "greeting")
        lock_path = self.project / "simpkg.lock"
        lock_path.write_text(lock_path.read_text().replace("schema = 1", "schema = 2"))
        overridden_env = {**environment, "SIMP_MODULE_DIR": str(independent)}
        from_env = run([compiler, "--check-only", str(source)], cwd=self.root, env=overridden_env)
        self.assertEqual(from_env.returncode, 0, from_env.stdout + from_env.stderr)
        from_flag = run([compiler, "--check-only", "-M", str(independent), str(source)],
                        cwd=self.root, env=environment)
        self.assertEqual(from_flag.returncode, 0, from_flag.stdout + from_flag.stderr)
        project_policy = run([compiler, "--check-only", "-M", str(self.project / "modules"), str(source)],
                             cwd=self.root, env=overridden_env)
        self.assertNotEqual(project_policy.returncode, 0)
        self.assertIn("schema", project_policy.stdout + project_policy.stderr)

    def test_compiler_graph_rejects_default_namespace_collision(self) -> None:
        namespace = "namespace Shared { class Greeter { int answer() { return 42 } } }\n"
        self.make_remote("helper", {"1.0.0": self.manifest(name="helper", export="namespace:Shared")},
                         source_content=namespace)
        self.make_remote("top", {"1.0.0": self.manifest(
            name="top", export="namespace:Shared",
            extras='[dependencies]\nhelper = "=1.0.0"\n[sources]\nhelper = "acme/helper"\n',
        )}, source_content=namespace)
        added = self.simpkg("add", "acme/top", "1.0.0", "--yes")
        self.assertEqual(added.returncode, 0, added.stderr)
        source = self.project / "app.simp"
        source.write_text("import top\nimport helper\nstart { print(Shared.Greeter().answer()) }\n")
        environment = self.base_environment.copy()
        environment.pop("SIMP_MODULE_DIR", None)
        command = [str(self.args.compiler), "--check-only", str(source)]
        collision = run(command, cwd=self.root, env=environment)
        self.assertNotEqual(collision.returncode, 0)
        self.assertIn("Shared", collision.stdout + collision.stderr)


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
