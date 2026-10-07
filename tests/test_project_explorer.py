"""Actual Tweed model, GTK list rows, chooser and main-loop responsiveness."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from gtk_support import headless
from program_runner import environment, invoke


DRIVER = r"""
class ExplorerFixture {
    void choose(String path, int accept, int overwrite)
    void collect()
    bool alert(String fragments)
    bool activate(int position)
    bool chooser()
    void watch()
    int gap()
    int beats()
    int now()
    bool virtualized()
    bool monospace()
    bool icons()
    void stop()
    void later(callback<void()> action)
}
void ExplorerFixture.choose(String path, int accept, int overwrite) from "fixture_editor_choose"
void ExplorerFixture.collect() from "fixture_editor_collect"
bool ExplorerFixture.alert(String fragments) from "fixture_editor_alert"
bool ExplorerFixture.activate(int position) from "explorer_activate"
bool ExplorerFixture.chooser() from "explorer_chooser"
void ExplorerFixture.watch() from "explorer_watch"
int ExplorerFixture.gap() from "explorer_gap"
int ExplorerFixture.beats() from "explorer_beats"
int ExplorerFixture.now() from "explorer_now"
bool ExplorerFixture.virtualized() from "explorer_virtualized"
bool ExplorerFixture.monospace() from "fixture_editor_monospace"
bool ExplorerFixture.icons() from "explorer_icons"
void ExplorerFixture.stop() from "explorer_stop"
void ExplorerFixture.later(callback<void()> action) from "explorer_later"

class ExplorerChecks {
    TweedEditor editor
    int stage
    int deadline
    int waitUntil
    int checks
    int startedAt
    int before
    int errorPhase
    bool sawChooser
    callback<void()> nextAction
    void require(bool condition, String label) {
        if (!condition) {
            System.StandardIO().writeErrorLine(format("FAIL stage {}: {}", stage, label))
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
    bool ready(String name) {
        ProjectNode node = find(name)
        return node != null && node.complete
    }
    void again() {
        require(ExplorerFixture().now() < deadline, "operation deadline")
        ExplorerFixture().later(nextAction)
    }
    void advance() {
        stage = stage + 1
        deadline = ExplorerFixture().now() + 30000
        Gtk.Application().post(nextAction)
    }
    void activate() {
        editor.activate()
        deadline = ExplorerFixture().now() + 30000
        Gtk.Application().post(nextAction)
    }
    void next() {
        if (stage == 0) {
            require(editor.documents.length == 1, "startup document")
            ExplorerFixture().choose(path("project"), 0, 0)
            editor.menu.activateItem(editor.openFolderItem)
            advance()
            return
        }
        if (stage == 1) {
            if (editor.fileDialog != null) {
                if (ExplorerFixture().chooser()) { sawChooser = true }
                again()
                return
            }
            require(sawChooser, "parented modal folder chooser")
            require(editor.project.rootPath.equals(""), "cancel does not change root")
            ExplorerFixture().choose(path("project"), 1, 0)
            editor.menu.activateItem(editor.openFolderItem)
            advance()
            return
        }
        if (stage == 2) {
            if (editor.fileDialog != null || !ready("project")) { again()
                return }
            require(editor.project.rootPath.equals(path("project")), "selected root")
            require(find("project/sub") != null && !find("project/sub").started, "collapsed folder is not enumerated")
            require(find("project/sub/nested.txt") == null, "no recursive population")
            require(find("project/.hidden.txt") == null && find("project/.hidden-dir") == null, "hidden files and directories excluded")
            require(find("project/loop").kind == 2 && !find("project/loop").started, "symlink loop is a leaf")
            require(editor.project.tree.value(3).equals(path("project/empty")), "directories first, deterministic sorting")
            require(ExplorerFixture().icons(), "GTK folder, file, source and symlink icons")
            // Root, empty, gone, sub, then a.txt: native GTK activation is
            // double-click/Enter, not a direct call to the editor open method.
            require(ExplorerFixture().activate(4), "GTK file activation")
            require(editor.currentDocument().view.text().equals("alpha\n"), "file contents")
            before = editor.documents.length
            editor.newDocument()
            require(ExplorerFixture().activate(4), "duplicate activation")
            require(editor.documents.length == before + 1 && editor.currentDocument().path.equals(path("project/a.txt")), "existing tab focused")
            require(ExplorerFixture().monospace(), "editor preferences preserved")
            require(ExplorerFixture().activate(3), "GTK directory activation expands")
            advance()
            return
        }
        if (stage == 3) {
            if (!ready("project/sub")) { again()
                return }
            require(find("project/sub/nested.txt") != null, "expanded folder loaded")
            editor.project.tree.setExpanded(find("project/empty").id, true)
            require(System.FileSystem().remove(path("project/gone")), "remove unopened directory")
            editor.project.tree.setExpanded(find("project/gone").id, true)
            advance()
            return
        }
        if (stage == 4) {
            if (!ready("project/gone") || !ready("project/empty")) { again()
                return }
            require(!find("project/gone").error.equals(""), "missing directory error visible")
            require(find("project/empty").error.equals(""), "empty directory success")
            if (errorPhase == 0) {
                before = editor.documents.length
                editor.project.activateFile(find("project/binary.dat").id)
                require(editor.documents.length == before && editor.status.text().contains("not a text file"), "binary preserved rejection")
                errorPhase = 1
                again()
                return
            }
            if (errorPhase == 1) {
                require(ExplorerFixture().alert("Could not open|binary.dat"), "binary dialog")
                editor.project.activateFile(find("project/invalid.txt").id)
                require(editor.documents.length == before && editor.status.text().contains("not a text file"), "UTF-8 preserved rejection")
                errorPhase = 2
                again()
                return
            }
            require(ExplorerFixture().alert("Could not open|invalid.txt"), "UTF-8 dialog")
            editor.project.activateFile(find("project/loop").id)
            require(editor.status.text().contains("not a regular file"), "directory symlink not followed by tree")
            editor.project.hidden.setActive(true)
            advance()
            return
        }
        if (stage == 5) {
            if (!ready("project")) { again()
                return }
            require(find("project/.hidden.txt") != null && find("project/.hidden-dir") != null, "show hidden")
            require(!find("project/sub").started, "toggle rebuild remains lazy")
            System.File added = System.File(path("project/added.txt"), "w")
            added.write("new\n")
            added.close()
            require(System.FileSystem().remove(path("project/a.txt")), "remove file")
            editor.project.refresh()
            advance()
            return
        }
        if (stage == 6) {
            if (!ready("project")) { again()
                return }
            require(find("project/added.txt") != null && find("project/a.txt") == null && find("project/gone") == null, "refresh additions/removals")
            require(editor.currentDocument().path.equals(path("project/a.txt")), "refresh leaves open documents alone")
            editor.project.hidden.setActive(false)
            editor.project.setRoot(path("missing"))
            advance()
            return
        }
        if (stage == 7) {
            if (!ready("missing")) { again()
                return }
            require(!find("missing").error.equals(""), "missing root is visibly failed")
            editor.project.setRoot(path("denied"))
            advance()
            return
        }
        if (stage == 8) {
            if (!ready("denied")) { again()
                return }
            require(!find("denied").error.equals(""), "unreadable directory error")
            editor.project.setRoot(path("nonutf8"))
            advance()
            return
        }
        if (stage == 9) {
            if (!ready("nonutf8")) { again()
                return }
            require(find("nonutf8").error.contains("UTF-8"), "invalid filename is explicit error")
            editor.project.setRoot(path("large"))
            editor.project.refresh()
            editor.project.hidden.setActive(true)
            editor.project.setRoot(path("project"))
            ExplorerFixture().collect()
            waitUntil = ExplorerFixture().now() + 250
            advance()
            return
        }
        if (stage == 10) {
            if (!ready("project") || ExplorerFixture().now() < waitUntil) { again()
                return }
            require(find("large/file00000.txt") == null && find("project/added.txt") != null, "cancelled stale results excluded")
            editor.project.setRoot(path("large"))
            ExplorerFixture().watch()
            startedAt = ExplorerFixture().now()
            advance()
            return
        }
        if (stage == 11) {
            require(ExplorerFixture().gap() < 300, "GUI heartbeat gap below 300ms for 20,000 entries")
            if (!ready("large")) { again()
                return }
            require(editor.project.tree.childCount(1) == 20000, "all large-directory entries delivered")
            require(ExplorerFixture().beats() > 10, "main loop serviced during population")
            require(ExplorerFixture().virtualized(), "large model uses fewer than 1000 row widgets")
            System.StandardIO().writeLine(format("RESPONSIVENESS {}ms maximum gap; {}ms total; {} heartbeats",
                ExplorerFixture().gap(), ExplorerFixture().now() - startedAt, ExplorerFixture().beats()))
            editor.project.refresh()
            editor.project.hidden.setActive(false)
            editor.project.setRoot(path("project"))
            waitUntil = ExplorerFixture().now() + 200
            advance()
            return
        }
        if (stage == 12) {
            if (!ready("project") || ExplorerFixture().now() < waitUntil) { again()
                return }
            require(find("large/file00000.txt") == null, "refresh/root replacement invalidates active delivery")
            editor.project.setRoot(path("large"))
            advance()
            return
        }
        if (stage == 13) {
            if (editor.project.tree.childCount(1) < 128) { again()
                return }
            require(!find("large").complete, "close while result delivery is still active")
            editor.closeCurrentDocument()
            require(editor.pendingCloseDocument == null, "clean document close intact")
            ExplorerFixture().stop()
            editor.window.close()
            require(editor.window.disposed(), "close during enumeration")
            ExplorerFixture().collect()
            System.StandardIO().writeLine(format("PASS {} Project Explorer checks", checks))
        }
    }
}

class ExplorerLifetime {
    ExplorerLifetime _self()
    TweedEditor editor
    Gtk.DirectoryScan scan
    int deliveries
    String mode
    void close() {
        if (deliveries != 1) { System.Process().exit(1) }
        ExplorerFixture().collect()
        editor.window.close()
        System.StandardIO().writeLine("PASS cancel inside batch callback")
    }
    void received(int count, String error) {
        deliveries = deliveries + 1
        if (count <= 0 || !error.equals("")) { System.Process().exit(1) }
        ExplorerFixture().collect()
        if (mode.equals("scan-cancel")) {
            scan.cancel()
            scan.cancel()
            Gtk.Application().post(_self().close)
        } else {
            scan.append(editor.project.tree, 0, 1, count, editor.project.icons)
            Gtk.Application().shutdown()
            ExplorerFixture().collect()
            System.StandardIO().writeLine("PASS shutdown inside batch callback")
        }
    }
    void request(int id) {
        Gtk.Application().shutdown()
        ExplorerFixture().collect()
        System.StandardIO().writeLine("PASS shutdown inside expansion callback")
    }
    void leaf(int id) {}
    void activate() {
        editor.activate()
        mode = System.Process().getEnv("EXPLORER_LIFETIME")
        if (mode.equals("folder-close")) {
            editor.chooseFolder()
            Gtk.FileDialog pending = editor.fileDialog
            editor.window.close()
            if (!editor.window.disposed() || pending.pending()) { System.Process().exit(1) }
            ExplorerFixture().collect()
            System.StandardIO().writeLine("PASS parent closes folder chooser")
            return
        }
        if (mode.equals("tree-shutdown")) {
            Gtk.Tree tree = Gtk.Tree(_self().request, _self().leaf)
            tree.add(0, 1, "root", "folder-symbolic", true, "", 1)
            tree.setExpanded(1, true)
            return
        }
        editor.project.tree.clear(0)
        scan = Gtk.DirectoryScan(editor.project.tree,
            System.FileSystem().join(System.FileSystem().getCwd(), "large"),
            true, _self().received)
    }
}
ExplorerLifetime ExplorerLifetime._self() from "simp_gtk_self"
"""


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--clang", required=True)
    parser.add_argument("--include", type=Path, required=True)
    parser.add_argument("--native", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--sanitize", default="")
    parser.add_argument("--xvfb", required=True)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=args.work, prefix="explorer-") as directory:
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
                         str(Path(__file__).with_name("project_explorer_fixture.c").resolve()),
                         "-o", str(native)], work, env)
        assert result.returncode == 0, result.stderr
        text = args.source.read_text()
        index = text.rindex("\nstart {")
        setup = text[index:].replace(
            "app.onActivate(editor.activate)",
            "ExplorerChecks checks = ExplorerChecks()\n"
            "    checks.editor = editor\n"
            "    checks.nextAction = checks.next\n"
            "    if (System.Process().getEnv(\"EXPLORER_LIFETIME\").equals(\"\")) {\n"
            "        app.onActivate(checks.activate)\n"
            "    } else {\n"
            "        ExplorerLifetime lifetime = ExplorerLifetime()\n"
            "        lifetime.editor = editor\n"
            "        app.onActivate(lifetime.activate)\n"
            "    }")
        source = work / "explorer.simp"
        source.write_text(text[:index] + DRIVER + setup)
        project = work / "project"
        project.mkdir()
        for name in ("empty", "gone", "sub", ".hidden-dir"):
            (project / name).mkdir()
        (project / "a.txt").write_text("alpha\n")
        (project / "z.simp").write_text("start {}\n")
        (project / "sub/nested.txt").write_text("nested\n")
        (project / ".hidden.txt").write_text("hidden\n")
        (project / "binary.dat").write_bytes(b"text\0binary")
        (project / "invalid.txt").write_bytes(b"bad\xff")
        (project / "loop").symlink_to(project, target_is_directory=True)
        denied = work / "denied"
        denied.mkdir()
        denied.chmod(0)
        invalid = work / "nonutf8"
        invalid.mkdir()
        descriptor = os.open(os.fsencode(invalid) + b"/bad\xff", os.O_CREAT | os.O_WRONLY, 0o600)
        os.close(descriptor)
        large = work / "large"
        large.mkdir()
        for i in range(20000):
            (large / f"file{i:05d}.txt").touch()
        result = invoke([str(args.compiler.resolve()), str(source), str(native),
                         "-o", str(work / "explorer")], work, env)
        assert result.returncode == 0, result.stderr
        try:
            with headless(args.xvfb, work, env) as gui_env:
                try:
                    result = invoke([str(work / "explorer")], work, gui_env, timeout=120)
                except subprocess.TimeoutExpired as failure:
                    raise AssertionError(f"Explorer timeout: {failure.stdout!r}; {failure.stderr!r}") from failure
                assert result.returncode == 0, result
                assert result.stdout.splitlines()[-1].startswith("PASS "), result
                assert not result.stderr, result.stderr
                print(result.stdout)
                for mode in ("folder-close", "scan-cancel", "scan-shutdown", "tree-shutdown"):
                    case_env = dict(gui_env, EXPLORER_LIFETIME=mode)
                    result = invoke([str(work / "explorer")], work, case_env, timeout=15)
                    assert result.returncode == 0 and "PASS " in result.stdout, result
                    assert not result.stderr, result.stderr
                    print(result.stdout.strip())
        finally:
            denied.chmod(0o700)


if __name__ == "__main__":
    main()
