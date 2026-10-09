"""Cwhip and legacy Tweed projects: real menus, choosers, confirmations and restore.

Every run uses a private HOME/XDG tree and private project fixtures."""

import argparse
import hashlib
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import tomllib

from gtk_support import headless
from program_runner import environment, invoke


DRIVER = r"""
class ProjectFixture {
    void choose(String path, int accept, int overwrite)
    void collect()
    bool alert(String fragments)
    bool alertOpen()
    void later(callback<void()> action)
    int now()
    bool click(String title, String label)
    bool visible(String title)
    bool has(String title, String fragment)
    int tabWidth()
}
void ProjectFixture.choose(String path, int accept, int overwrite) from "fixture_editor_choose"
void ProjectFixture.collect() from "fixture_editor_collect"
bool ProjectFixture.alert(String fragments) from "fixture_editor_alert"
bool ProjectFixture.alertOpen() from "fixture_editor_alert_open"
void ProjectFixture.later(callback<void()> action) from "explorer_later"
int ProjectFixture.now() from "explorer_now"
bool ProjectFixture.click(String title, String label) from "project_click"
bool ProjectFixture.visible(String title) from "project_window_visible"
bool ProjectFixture.has(String title, String fragment) from "project_window_has"
int ProjectFixture.tabWidth() from "project_tab_width"

class ProjectChecks {
    CwhipEditor editor
    callback<void()> nextAction
    String mode
    int stage
    int checks
    int deadline
    void require(bool condition, String label) {
        if (!condition) {
            System.StandardIO().writeErrorLine(format("FAIL {} stage {}: {}: {}", mode, stage, label, editor.status.text()))
            System.Process().exit(1)
        }
        checks = checks + 1
    }
    String path(String name) { return System.FileSystem().join(System.FileSystem().getCwd(), name) }
    ProjectNode find(String name) {
        String expected = path(name)
        int i = 0
        while (i < editor.project.nodes.length) {
            ProjectNode node = editor.project.nodes[i] as ProjectNode
            if (node.path.equals(expected)) { return node }
            i = i + 1
        }
        int id = editor.project.tree.findValue(expected)
        if (id == 0) { return null }
        return ProjectNode(editor.project, id, expected, editor.project.tree.data(id))
    }
    bool loaded(String name) {
        ProjectNode node = find(name)
        return node != null && node.complete
    }
    bool expanded(String name) {
        ProjectNode node = find(name)
        return node != null && editor.project.tree.expanded(node.id)
    }
    bool busy() {
        return editor.projectSaves > 0 || editor.fileDialog != null || editor.savePending
    }
    void again() {
        String root = ""
        if (editor.activeProject != null) { root = editor.activeProject.root }
        require(ProjectFixture().now() < deadline,
            format("operation deadline (saves={}, dialog={}, pending={}, confirm={}, root={})",
                editor.projectSaves, editor.fileDialog != null, editor.savePending,
                editor.confirmOp, root))
        ProjectFixture().later(nextAction)
    }
    void advance() {
        stage = stage + 1
        deadline = ProjectFixture().now() + 30000
        ProjectFixture().later(nextAction)
    }
    void finish() {
        print(format("PASS {} {}", mode, checks))
        editor.quitAction()
    }
    void activate() {
        mode = System.Process().getEnv("PROJECT_CASE")
        editor.activate()
        deadline = ProjectFixture().now() + 30000
        ProjectFixture().later(nextAction)
    }
    void menu(int item) { editor.menu.activateItem(item) }
    int item(int index) { return editor.projectItems[index] as int }
    void next() {
        if (busy()) {
            again()
            return
        }
        ProjectFixture().collect()
        if (mode.equals("create")) { create() }
        if (mode.equals("restore")) { restore() }
        if (mode.equals("reopen")) { reopen() }
        if (mode.equals("reopen-off") || mode.equals("cli")) { notReopened() }
        if (mode.equals("missing")) { missing() }
        if (mode.equals("bad-config")) { badConfig() }
        if (mode.equals("bad-workspace")) { badWorkspace() }
        if (mode.equals("write-failure")) { writeFailure() }
        if (mode.equals("dirty-switch")) { dirtySwitch() }
        if (mode.equals("save-as")) { saveAs() }
        if (mode.equals("delete")) { deleteProject() }
        if (mode.equals("about")) { about() }
        if (mode.equals("switch-scan")) { switchScan() }
        if (mode.equals("legacy")) { legacy() }
    }
    void create() {
        if (stage == 0) {
            require(editor.activeProject == null && editor.documents.length == 1, "folderless startup")
            menu(item(0))
            require(!System.FileSystem().exists(path("proj/.cwhip")), "disabled Save Project without a project")
            ProjectFixture().choose(path("proj"), 0, 0)
            menu(editor.projectNewItem)
            advance()
            return
        }
        if (stage == 1) {
            require(editor.activeProject == null && !System.FileSystem().exists(path("proj/.cwhip")), "cancelled New creates nothing")
            ProjectFixture().choose(path("proj"), 1, 0)
            menu(editor.projectNewItem)
            advance()
            return
        }
        if (stage == 2) {
            if (!loaded("proj")) {
                again()
                return
            }
            require(editor.activeProject != null && editor.activeProject.root.equals(path("proj")), "New activates root")
            require(editor.activeProject.name.equals("proj"), "default project name")
            require(System.FileSystem().isFile(path("proj/.cwhip/project.toml")) &&
                System.FileSystem().isFile(path("proj/.cwhip/workspace.toml")), "metadata written")
            require(find("proj/build") != null && find("proj/x.log") != null, "no exclusions yet")
            editor.openFile(path("proj/a.simp"))
            editor.currentDocument().view.setCursor(2, 3)
            menu(item(3))
            require(ProjectFixture().visible("Configure Project"), "parented Configure window")
            editor.configureName.setText("Demo")
            editor.configureExcludes.setText("build, *.log")
            OverrideRow tabs = editor.overrideRow("tab_width")
            tabs.override.setActive(true)
            tabs.entry.setText("3")
            require(ProjectFixture().click("Configure Project", "Apply"), "Apply button")
            advance()
            return
        }
        if (stage == 3) {
            if (!loaded("proj")) {
                again()
                return
            }
            require(editor.activeProject.name.equals("Demo") && editor.project.title.text().equals("Demo"), "renamed project")
            require(ProjectFixture().tabWidth() == 3, "project override applied")
            require(find("proj/build") == null && find("proj/x.log") == null && find("proj/sub") != null, "exclusions applied")
            editor.showPreferences()
            editor.tabsEntry.setText("5")
            require(editor.settings.tabWidth == 5 && ProjectFixture().tabWidth() == 3, "project override beats user preference")
            require(ProjectFixture().has("Cwhip Preferences", "overrides: tab width"), "Preferences names active overrides")
            require(ProjectFixture().click("Configure Project", "Reset All to Inherit"), "inherit button")
            require(ProjectFixture().click("Configure Project", "Apply"), "Apply inherit")
            advance()
            return
        }
        if (stage == 4) {
            require(!editor.activeProject.hasTabs && ProjectFixture().tabWidth() == 5, "inherit follows preferences")
            editor.project.tree.setExpanded(find("proj/sub").id, true)
            editor.window.setDefaultSize(980, 720)
            editor.workspace.setPosition(300)
            advance()
            return
        }
        if (stage == 5) {
            if (!loaded("proj/sub")) {
                again()
                return
            }
            menu(item(0))
            advance()
            return
        }
        require(editor.status.text().contains("Project saved"), "Save Project")
        finish()
    }
    void restore() {
        if (stage == 0) {
            require(editor.activeProject != null && editor.activeProject.name.equals("Demo"), "command-line project")
            require(editor.documents.length == 1 && editor.currentDocument().path.equals(path("proj/a.simp")), "tabs restored")
            require(editor.currentDocument().view.line() == 2 && editor.currentDocument().view.column() == 3, "cursor restored")
            require(editor.window.defaultWidth() >= editor.activeProject.windowWidth &&
                editor.activeProject.windowWidth >= 980 &&
                editor.window.defaultHeight() == editor.activeProject.windowHeight,
                format("window size restored {}x{} {}", editor.window.defaultWidth(), editor.window.defaultHeight(), editor.activeProject.windowWidth))
            require(editor.workspace.position() == 300, "explorer width restored")
            advance()
            return
        }
        if (stage == 1) {
            if (!expanded("proj/sub") || !loaded("proj/sub")) {
                again()
                return
            }
            require(find("proj/build") == null, "exclusions restored")
            editor.showPreferences()
            editor.reopenCheck.setActive(true)
            require(editor.settings.reopenLast && editor.settings.lastRoot.equals(path("proj")), "reopen remembers root")
            advance()
            return
        }
        require(editor.preferencesMessage.text().equals("Settings saved"), format("settings saved: {}", editor.preferencesMessage.text()))
        finish()
    }
    void reopen() {
        if (stage == 0) {
            require(editor.activeProject != null && editor.activeProject.root.equals(path("proj")), "reopened last project")
            require(editor.currentDocument().path.equals(path("proj/a.simp")), "reopened tabs")
            editor.showPreferences()
            editor.reopenCheck.setActive(false)
            require(editor.settings.lastRoot.equals(""), "disabling forgets root")
            advance()
            return
        }
        finish()
    }
    void notReopened() {
        require(editor.activeProject == null, "no automatic reopen")
        if (mode.equals("cli")) { require(editor.currentDocument().path.equals(path("other.txt")), "command-line file wins") }
        finish()
    }
    void missing() {
        require(ProjectFixture().alert("Could not reopen last project|not an accessible folder"), "missing project reported")
        require(editor.activeProject == null && editor.documents.length == 1, "editor still usable")
        finish()
    }
    void badConfig() {
        require(ProjectFixture().alert("Could not open project|Malformed"), "malformed config reported")
        require(editor.activeProject == null, "malformed project not opened")
        finish()
    }
    void badWorkspace() {
        if (stage == 0) {
            require(ProjectFixture().alert("Workspace state ignored"), "malformed workspace reported")
            require(editor.activeProject != null && editor.currentDocument().path.equals(""), "nothing restored")
            menu(item(0))
            advance()
            return
        }
        require(ProjectFixture().alert("Project save failed|workspace.toml"), "invalid workspace not overwritten")
        finish()
    }
    void writeFailure() {
        if (stage == 0) {
            editor.openFile(path("proj/a.simp"))
            menu(item(0))
            advance()
            return
        }
        if (stage == 1) {
            require(ProjectFixture().alert("Project save failed"), "write failure reported")
            require(!ProjectFixture().alertOpen(), "alert dismissed")
            editor.quitAction()
            advance()
            return
        }
        require(editor.confirmOp.equals("continue-quit") && !editor.window.disposed(), "quit asks after failed workspace save")
        print(format("PASS {} {}", mode, checks))
        require(ProjectFixture().click("Cwhip Editor", "Quit Without Saving"), "quit without saving")
    }
    void dirtySwitch() {
        if (stage == 0) {
            editor.currentDocument().view.setText("changed\n")
            ProjectFixture().choose(path("proj2"), 1, 0)
            menu(editor.projectOpenItem)
            advance()
            return
        }
        if (stage == 1) {
            require(editor.pendingCloseAll && editor.switchOp.equals("open"), "dirty prompt before switching")
            editor.cancelPendingClose()
            require(editor.activeProject.root.equals(path("proj")) && editor.currentDocument().view.modified(), "cancel keeps project and edits")
            ProjectFixture().choose(path("proj2"), 1, 0)
            menu(editor.projectOpenItem)
            advance()
            return
        }
        if (stage == 2) {
            require(editor.pendingCloseAll, "dirty prompt again")
            editor.discardPendingClose()
            advance()
            return
        }
        require(editor.activeProject.root.equals(path("proj2")), "switched project")
        require(editor.documents.length == 1 && editor.currentDocument().path.equals(path("proj2/readme.txt")), "second workspace restored")
        editor.currentDocument().view.setText("dirty\n")
        editor.quitAction()
        require(editor.pendingCloseAll, "quit prompts for dirty document")
        print(format("PASS {} {}", mode, checks))
        editor.discardPendingClose()
    }
    void saveAs() {
        if (stage == 0) {
            ProjectFixture().choose(path("proj"), 1, 0)
            menu(item(1))
            advance()
            return
        }
        if (stage == 1) {
            require(ProjectFixture().alert("Choose a different folder"), "same root refused")
            ProjectFixture().choose(path("copy"), 1, 0)
            menu(item(1))
            advance()
            return
        }
        if (stage == 2) {
            require(editor.activeProject.root.equals(path("copy")) && editor.activeProject.name.equals("Demo"), "Save As activates copy")
            require(editor.currentDocument().path.equals(path("proj/a.simp")), "documents stay open")
            ProjectFixture().choose(path("taken"), 1, 0)
            menu(item(1))
            advance()
            return
        }
        if (stage == 3) {
            require(editor.confirmOp.equals("replace-save-as") && ProjectFixture().has("Cwhip Editor", "already has Cwhip project metadata"), "overwrite confirmation")
            require(ProjectFixture().click("Cwhip Editor", "Cancel"), "cancel overwrite")
            require(editor.activeProject.root.equals(path("copy")), "cancel keeps project")
            ProjectFixture().choose(path("taken"), 1, 0)
            menu(item(1))
            advance()
            return
        }
        if (stage == 4) {
            require(ProjectFixture().click("Cwhip Editor", "Replace Metadata"), "accept overwrite")
            advance()
            return
        }
        require(editor.activeProject.root.equals(path("taken")), "replaced metadata activates")
        finish()
    }
    void deleteProject() {
        if (stage == 0) {
            require(editor.settings.lastRoot.equals(path("proj")), "remembered root")
            menu(item(2))
            require(ProjectFixture().has("Cwhip Editor", path("proj")), "confirmation names root")
            require(ProjectFixture().click("Cwhip Editor", "Cancel"), "cancel delete")
            require(System.FileSystem().isFile(path("proj/.cwhip/project.toml")) && editor.activeProject != null, "cancel keeps metadata")
            menu(item(2))
            require(ProjectFixture().click("Cwhip Editor", "Delete Metadata"), "confirm delete")
            require(editor.activeProject == null && editor.project.rootPath.equals(path("proj")), "folder mode on same root")
            require(editor.settings.lastRoot.equals(""), "deleted project forgotten")
            advance()
            return
        }
        finish()
    }
    void about() {
        if (stage == 0) {
            menu(editor.aboutItem)
            String version = System.Process().getEnv("EXPECTED_VERSION")
            require(editor.version().equals(version), "version matches CMake project version")
            require(ProjectFixture().visible("About Cwhip"), "parented About window")
            require(ProjectFixture().has("About Cwhip", format("Version {}", version)) &&
                ProjectFixture().has("About Cwhip", "Cwhip Editor") &&
                ProjectFixture().has("About Cwhip", "https://cwhip.org") &&
                ProjectFixture().has("About Cwhip", "https://github.com/chucktilbury/simp"), "About contents")
            require(ProjectFixture().click("About Cwhip", "Close"), "Close button")
            require(!ProjectFixture().visible("About Cwhip"), "About hidden")
            menu(editor.aboutItem)
            require(ProjectFixture().visible("About Cwhip"), "About reopens")
            advance()
            return
        }
        finish()
    }
    void switchScan() {
        if (stage == 0) {
            require(editor.activeProject != null, "project opened")
            editor.openProjectAt(path("proj2"))
            advance()
            return
        }
        if (stage == 1) {
            require(editor.activeProject.root.equals(path("proj2")), "switched while scanning")
            ProjectFixture().choose(path("other"), 1, 0)
            menu(editor.openFolderItem)
            advance()
            return
        }
        if (!loaded("other")) {
            again()
            return
        }
        require(editor.activeProject == null && editor.project.title.text().equals("Project Explorer"), "folder mode")
        require(find("other/plain.txt") != null, "folder listing")
        finish()
    }
    void legacy() {
        if (stage == 0) {
            menu(editor.projectOpenItem)
            ProjectFixture().choose(path("legacy"), 1, 0)
            advance()
            return
        }
        if (editor.activeProject == null) {
            again()
            return
        }
        require(editor.activeProject.directory.equals(path("legacy/.tweed")),
            "legacy project metadata opened in place")
        finish()
    }
}
"""


def digest(root):
    return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(root.rglob("*")) if p.is_file() and ".cwhip" not in p.parts}


def main():
    parser = argparse.ArgumentParser()
    for name in ("--compiler", "--include", "--native", "--source", "--work", "--cmake"):
        parser.add_argument(name, type=Path, required=True)
    parser.add_argument("--clang", required=True)
    parser.add_argument("--sanitize", default="")
    parser.add_argument("--xvfb", required=True)
    args = parser.parse_args()
    version = re.search(r"project\(cwhip VERSION ([0-9.]+)", args.cmake.read_text()).group(1)
    args.work.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=args.work, prefix="project-") as directory:
        work = Path(directory).resolve()
        env = environment(work)
        env["GTK_USE_PORTAL"] = "0"
        flags = shlex.split(subprocess.check_output(
            ["pkg-config", "--cflags", "gtk4", "gtksourceview-5"], text=True))
        if args.sanitize:
            flags += ["-fsanitize=" + args.sanitize, "-fno-omit-frame-pointer"]
        native = work / "fixture.o"
        result = invoke([args.clang, "-std=c11", "-Wall", "-Wextra", "-Werror",
                         "-Wno-deprecated-declarations", "-I" + str(args.include.resolve()),
                         "-I" + str(args.native.resolve()), *flags, "-c",
                         str(Path(__file__).with_name("tweed_project_fixture.c").resolve()),
                         "-o", str(native)], work, env)
        assert result.returncode == 0, result.stderr
        text = args.source.read_text()
        index = text.rindex("\nstart {")
        setup = text[index:].replace(
            "app.onActivate(editor.activate)",
            "ProjectChecks checks = ProjectChecks()\n"
            "    checks.editor = editor\n"
            "    checks.nextAction = checks.next\n"
            "    app.onActivate(checks.activate)")
        source = work / "project.simp"
        source.write_text(text[:index] + DRIVER + setup)
        executable = work / "project"
        result = invoke([str(args.compiler.resolve()), str(source), str(native),
                         "-o", str(executable)], work, env)
        assert result.returncode == 0, result.stderr

        case = work / "case"
        case.mkdir()
        case_env = environment(case)
        settings = Path(case_env["XDG_CONFIG_HOME"]) / "cwhip/settings.toml"

        def sources(root, extra=()):
            root.mkdir()
            (root / "a.simp").write_text("class A {\n    int value = 1\n}\n")
            for name in extra:
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(name + "\n")

        proj = case / "proj"
        sources(proj, ("sub/b.txt", "build/out.txt", "x.log"))
        (case / "other").mkdir()
        (case / "other/plain.txt").write_text("plain\n")
        (case / "other.txt").write_text("other\n")
        before = digest(proj)

        def run(name, *arguments, expect_pass=True):
            run_env = dict(case_env, PROJECT_CASE=name, EXPECTED_VERSION=version)
            with headless(args.xvfb, case, run_env) as gui_env:
                try:
                    result = invoke([str(executable), *arguments], case, gui_env, timeout=100)
                except subprocess.TimeoutExpired as failure:
                    raise AssertionError(f"{name} timeout: {failure.stdout!r}; {failure.stderr!r}") from failure
            assert result.returncode == 0, f"{name}:\n{result.stdout}\n{result.stderr}"
            assert any(line.startswith(f"PASS {name} ") for line in result.stdout.splitlines()), result
            warning = re.compile(
                r"\(project:\d+\): Gtk-WARNING \*\*: [0-9:.]+: "
                r"GtkGizmo 0x[0-9a-f]+ \(tabs\) reported min height -3, "
                r"but sizes must be >= 0"
            )
            stderr_lines = [line for line in result.stderr.splitlines() if line.strip()]
            # GTK 4.18 can emit this known notebook geometry warning under Xvfb.
            assert all(warning.fullmatch(line) for line in stderr_lines), (
                f"{name}: {result.stderr}"
            )
            print(f"PASS {name}")

        def toml(path):
            return tomllib.loads(path.read_text())

        def remember(reopen, root):
            text = settings.read_text()
            text = re.split(r'^\[\"?project\"?\]', text, flags=re.M)[0]
            settings.write_text(text + f'[project]\nreopen_last = {str(reopen).lower()}\nlast_root = "{root}"\n')

        run("create")
        config = toml(proj / ".cwhip/project.toml")
        assert config["version"] == 1 and config["project"]["name"] == "Demo", config
        assert config["explorer"]["exclude"] == ["build", "*.log"], config
        assert "tab_width" not in config.get("editor", {}), "inherited setting persisted"
        state = toml(proj / ".cwhip/workspace.toml")["workspace"]
        assert state["active"] == "a.simp", state
        assert state["files"] == [{"path": "a.simp", "line": 2, "column": 3}], state
        assert state["expanded"] == ["sub"], state
        assert state["window_width"] == 980 and state["window_height"] == 720, state
        assert state["explorer_width"] == 300, state
        assert toml(settings)["editor"]["tab_width"] == 5
        assert digest(proj) == before, "source files changed"
        assert (proj / ".cwhip/project.toml").stat().st_mode & 0o777 == 0o600

        run("restore", "proj")
        assert toml(settings)["project"] == {"reopen_last": True, "last_root": str(proj)}, settings.read_text()
        run("cli", "other.txt")
        run("reopen")
        assert toml(settings)["project"] == {"reopen_last": False, "last_root": ""}
        remember(False, proj)
        run("reopen-off")
        remember(True, case / "gone")
        run("missing")
        remember(False, "")

        broken = case / "broken"
        sources(broken)
        (broken / ".cwhip").mkdir()
        (broken / ".cwhip/project.toml").write_text("version = [\n")
        run("bad-config", "broken")
        assert (broken / ".cwhip/project.toml").read_text() == "version = [\n"

        escape = case / "escape"
        sources(escape)
        (escape / ".cwhip").mkdir()
        (escape / ".cwhip/project.toml").write_text('version = 1\n[project]\nname = "Escape"\n')
        bad_state = 'version = 1\n[workspace]\nfiles = [{ path = "../proj/a.simp", line = 1, column = 1 }]\n'
        (escape / ".cwhip/workspace.toml").write_text(bad_state)
        run("bad-workspace", "escape")
        assert (escape / ".cwhip/workspace.toml").read_text() == bad_state

        if os.getuid() != 0:
            locked = case / "locked"
            sources(locked)
            (locked / ".cwhip").mkdir()
            (locked / ".cwhip/project.toml").write_text('version = 1\n[project]\nname = "Locked"\n')
            (locked / ".cwhip").chmod(0o500)
            try:
                run("write-failure", "locked")
            finally:
                (locked / ".cwhip").chmod(0o700)
            assert not (locked / ".cwhip/workspace.toml").exists()

        proj2 = case / "proj2"
        sources(proj2, ("readme.txt",))
        (proj2 / ".cwhip").mkdir()
        (proj2 / ".cwhip/project.toml").write_text('version = 1\n[project]\nname = "Second"\n')
        (proj2 / ".cwhip/workspace.toml").write_text(
            'version = 1\n[workspace]\nactive = "readme.txt"\nfiles = [{ path = "readme.txt", line = 1, column = 1 }]\n')
        run("dirty-switch", "proj")
        assert (proj / "a.simp").read_text() == "class A {\n    int value = 1\n}\n", "discarded edit saved"
        assert digest(proj) == before
        assert (proj2 / "readme.txt").read_text() == "readme.txt\n"
        assert toml(proj2 / ".cwhip/workspace.toml")["workspace"]["files"][0]["path"] == "readme.txt"

        original = {p.name: p.read_bytes() for p in (proj / ".cwhip").iterdir()}
        copy = case / "copy"
        sources(copy)
        taken = case / "taken"
        sources(taken)
        (taken / ".cwhip").mkdir()
        (taken / ".cwhip/project.toml").write_text('version = 1\n[project]\nname = "Taken"\n')
        run("save-as", "proj")
        assert {p.name: p.read_bytes() for p in (proj / ".cwhip").iterdir()} == original, "Save As changed original"
        copied = toml(copy / ".cwhip/project.toml")
        assert copied["project"]["name"] == "Demo" and copied["explorer"]["exclude"] == ["build", "*.log"]
        copied_state = toml(copy / ".cwhip/workspace.toml")["workspace"]
        original_files = toml(proj / ".cwhip/workspace.toml")["workspace"]["files"]
        assert [f["path"] for f in original_files] == ["a.simp"], original_files
        assert copied_state["files"] == original_files, copied_state
        assert copied_state["expanded"] == [], "expansion for folders missing in target kept"
        assert toml(taken / ".cwhip/project.toml")["project"]["name"] == "Demo"
        assert not (copy / "sub").exists() and digest(proj) == before

        run("switch-scan", "proj")
        assert toml(proj2 / ".cwhip/workspace.toml")["version"] == 1

        (proj / ".cwhip/notes.txt").write_text("keep\n")
        remember(True, proj)
        run("delete")
        assert not (proj / ".cwhip/project.toml").exists() and not (proj / ".cwhip/workspace.toml").exists()
        assert (proj / ".cwhip/notes.txt").read_text() == "keep\n", "unknown metadata removed"
        assert digest(proj) == before, "delete touched sources"
        assert toml(settings)["project"]["last_root"] == ""

        run("about")

        legacy = case / "legacy"
        sources(legacy)
        (legacy / ".tweed").mkdir()
        old_project = 'version = 1\n[project]\nname = "Old project"\n'
        old_workspace = 'version = 1\n[workspace]\n'
        (legacy / ".tweed/project.toml").write_text(old_project)
        (legacy / ".tweed/workspace.toml").write_text(old_workspace)
        run("legacy")
        assert (legacy / ".tweed/project.toml").read_text() == old_project
        saved_legacy_workspace = toml(legacy / ".tweed/workspace.toml")
        assert saved_legacy_workspace["version"] == 1
        assert "workspace" in saved_legacy_workspace


if __name__ == "__main__":
    main()
