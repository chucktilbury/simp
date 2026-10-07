"""Exercise the actual Simple editor's menus and asynchronous native choosers."""

import argparse
from pathlib import Path
import shlex
import subprocess
import tempfile

from gtk_support import headless
from program_runner import environment, invoke


DRIVER = r"""
class Fixture {
    void choose(String path, int accept, int overwrite)
    void collect()
    void edit(String text)
    bool shortcut(String trigger)
    int height()
    void resize()
    bool entryFocus(String text)
    void cleanup()
}
void Fixture.choose(String path, int accept, int overwrite) from "fixture_editor_choose"
void Fixture.collect() from "fixture_editor_collect"
void Fixture.edit(String text) from "fixture_editor_edit"
bool Fixture.shortcut(String trigger) from "fixture_editor_shortcut"
int Fixture.height() from "fixture_editor_height"
void Fixture.resize() from "fixture_editor_resize"
bool Fixture.entryFocus(String text) from "fixture_editor_entry_focus"
void Fixture.cleanup() from "fixture_editor_cleanup"

class EditorChecks {
    TweedEditor editor
    int stage
    int checks
    int originalHeight
    EditorDocument saved
    callback<void()> nextAction
    void require(bool condition, String label) {
        if (!condition) {
            System.StandardIO().writeErrorLine(format("FAIL stage {}: {}: {}", stage, label, editor.status.text()))
            System.Process().exit(1)
        }
        checks = checks + 1
    }
    String contents(String path) {
        System.File file = System.File(path, "r")
        require(file.isOpen(), "read saved file")
        String result = file.readAll()
        file.close()
        return result
    }
    String path(String name) { return System.FileSystem().join(System.FileSystem().getCwd(), name) }
    void activate() {
        editor.activate()
        Gtk.Application().post(nextAction)
    }
    void opened(String selected) {
        Fixture().collect()
        if (stage == 11 && !selected.equals("")) {
            require(System.FileSystem().remove(selected), "remove selected file before Open")
        }
        editor.opened(selected)
        Gtk.Application().post(nextAction)
    }
    void savedAs(String selected) {
        Fixture().collect()
        editor.savedAs(selected)
        Gtk.Application().post(nextAction)
    }
    void next() {
        System.StandardIO().writeLine(format("STEP {}", stage))
        Fixture().collect()
        if (stage == 0) {
            require(editor.documents.length == 1 && editor.currentDocument().path.equals(""), "startup untitled")
            editor.menu.activateItem(editor.newItem)
            require(editor.documents.length == 2, "menu New")
            editor.currentDocument().view.setText("created\nsecond line\n")
            Fixture().choose("", 0, 0)
            editor.menu.activateItem(editor.documentItems[0] as int)
        }
        if (stage == 1) {
            require(editor.currentDocument().path.equals("") && editor.currentDocument().view.modified(), "cancel untitled Save")
            Fixture().choose(path("created.simp"), 1, 0)
            require(Fixture().shortcut("<Control>s"), "shortcut Save routes to chooser")
        }
        if (stage == 2) {
            saved = editor.currentDocument()
            require(saved.path.equals(path("created.simp")) && !saved.view.modified(), "Save creates identity")
            require(contents(saved.path).equals("created\nsecond line\n"), "Save contents")
            saved.view.setText("updated\n")
            editor.menu.activateItem(editor.documentItems[0] as int)
            require(contents(saved.path).equals("updated\n") && !saved.view.modified(), "named Save")
            Fixture().choose(path("overwrite.simp"), 1, -1)
            editor.menu.activateItem(editor.documentItems[1] as int)
        }
        if (stage == 3) {
            require(saved.path.equals(path("created.simp")) && contents(path("overwrite.simp")).equals("original\n"), "overwrite cancel preserves identity and contents")
            Fixture().choose(path("overwrite.simp"), 1, 1)
            editor.menu.activateItem(editor.documentItems[1] as int)
        }
        if (stage == 4) {
            require(saved.path.equals(path("overwrite.simp")) && contents(saved.path).equals("updated\n"), "overwrite confirmed")
            Fixture().choose(saved.path, 1, 0)
            editor.menu.activateItem(editor.openItem)
        }
        if (stage == 5) {
            require(editor.documents.length == 2 && editor.currentDocument() == saved, "duplicate Open focuses tab")
            Fixture().choose(path("other.simp"), 1, 0)
            require(Fixture().shortcut("<Control>o"), "shortcut Open routes to chooser")
        }
        if (stage == 6) {
            require(editor.documents.length == 3 && editor.currentDocument().view.text().equals("other\n"), "Open reads selected file")
            editor.openFile(path("missing.simp"))
            require(editor.status.text().contains("Open failed") && editor.documents.length == 3, "invalid open reported")
            editor.openFile(System.FileSystem().getCwd())
            require(editor.status.text().contains("not a regular file"), "directory rejected")
            editor.activateDocument(saved)
            saved.view.setText("must not overwrite other\n")
            Fixture().choose(path("other.simp"), 1, 1)
            editor.menu.activateItem(editor.documentItems[1] as int)
        }
        if (stage == 7) {
            require(editor.status.text().contains("another open document") && contents(path("other.simp")).equals("other\n") && saved.path.equals(path("overwrite.simp")), "identity collision rejected")
            editor.menu.activateItem(editor.newItem)
            editor.currentDocument().view.setText("unsaved\n")
            editor.menu.activateItem(editor.documentItems[2] as int)
            require(editor.pendingCloseDocument == editor.currentDocument(), "dirty Close prompts")
            Fixture().choose("", 0, 0)
            editor.closeSaveAction()
        }
        if (stage == 8) {
            require(editor.documents.length == 4 && editor.currentDocument().view.modified() && editor.pendingCloseDocument == null && !editor.pendingCloseAll, "close-save chooser Cancel does not close")
            editor.closeCurrentDocument()
            Fixture().choose(path("close-save.simp"), 1, 0)
            editor.closeSaveAction()
        }
        if (stage == 9) {
            require(editor.documents.length == 3 && contents(path("close-save.simp")).equals("unsaved\n"), "close-save chooser accept saves then closes")
            editor.activateDocument(saved)
            editor.closeCurrentDocument()
            editor.closeCancelAction()
            require(editor.documents.length == 3 && saved.view.modified(), "dirty Close Cancel")
            editor.closeCurrentDocument()
            editor.closeSaveAction()
            require(editor.documents.length == 2 && contents(path("overwrite.simp")).equals("must not overwrite other\n"), "dirty Close Save")
            editor.menu.activateItem(editor.newItem)
            editor.currentDocument().view.setText("discard\n")
            require(Fixture().shortcut("<Control>w"), "shortcut Close")
            editor.closeDiscardAction()
            require(editor.documents.length == 2, "dirty Close Discard")
            Fixture().choose("", 0, 0)
            editor.menu.activateItem(editor.openItem)
        }
        if (stage == 10) {
            require(editor.documents.length == 2, "Open Cancel preserves tabs")
            Fixture().choose(path("vanishing.simp"), 1, 0)
            editor.menu.activateItem(editor.openItem)
        }
        if (stage == 11) {
            require(editor.documents.length == 2 && editor.status.text().contains("Open failed"), "accepted file disappearing before Open is reported")
            EditorDocument document = editor.currentDocument()
            require(!editor.writeDocument(document, path("missing-folder/save.simp")) && document.path.equals(path("other.simp")), "invalid save preserves identity")
            editor.openFile("sftp://invalid.example/file.simp")
            require(editor.status.text().contains("only local filesystem paths"), "nonlocal Open reported")
            require(!editor.writeDocument(document, "sftp://invalid.example/file.simp") && document.path.equals(path("other.simp")), "nonlocal Save rejected")
            Fixture().edit("undo this\n")
            editor.menu.activateItem(editor.undoItem)
            require(!editor.currentDocument().view.text().equals("undo this\n"), "menu Undo")
            require(Fixture().shortcut("<Control><Shift>z"), "shortcut Redo")
            require(editor.currentDocument().view.text().equals("undo this\n"), "redo contents")
            editor.currentDocument().view.selectAll()
            require(editor.currentDocument().view.hasSelection(), "Select All")
            editor.menu.activateItem(editor.copyItem)
            editor.menu.activateItem(editor.cutItem)
            require(editor.currentDocument().view.text().equals(""), "menu Cut")
            editor.pasteAction()
            Gtk.Application().post(nextAction)
        }
        if (stage == 12) {
            require(editor.currentDocument().view.text().equals("undo this\n"), "Paste clipboard")
            editor.menu.activateItem(editor.documentItems[4] as int)
            require(editor.currentDocument().view.hasSelection(), "menu Select All")
            editor.findEntry.setText("undo")
            editor.menu.activateItem(editor.documentItems[5] as int)
            require(Fixture().entryFocus("undo"), "Find menu focuses query")
            editor.findAction()
            require(editor.currentDocument().view.hasSelection(), "Find selection")
            editor.replaceEntry.setText("replace")
            editor.menu.activateItem(editor.documentItems[6] as int)
            require(Fixture().entryFocus("replace"), "Replace menu focuses replacement")
            editor.replaceAction()
            require(editor.currentDocument().view.text().contains("replace this"), "Replace contents")
            originalHeight = Fixture().height()
            require(originalHeight > 350, "multiline viewport allocation")
            Fixture().resize()
            // Chooser cancellation yields control for GTK to allocate the resized window.
            Fixture().choose("", 0, 0)
            editor.menu.activateItem(editor.openItem)
        }
        if (stage == 13) {
            require(Fixture().height() > originalHeight + 100, "viewport grows after resize")
            editor.menu.activateItem(editor.quitItem)
            require(!editor.window.disposed() && editor.pendingCloseAll, "Quit protects dirty tabs")
            editor.closeCancelAction()
            require(!editor.window.disposed(), "Quit Cancel")
            (editor.documents[0] as EditorDocument).view.setText("unsaved window close\n")
            editor.quitAction()
            Fixture().choose("", 0, 0)
            editor.closeSaveAction()
        }
        if (stage == 14) {
            require(!editor.window.disposed() && editor.documents.length == 2 && !editor.pendingCloseAll && editor.pendingCloseDocument == null, "Quit Save As Cancel preserves all documents")
            editor.quitAction()
            editor.closeDiscardAction()
            require(editor.window.disposed(), "Quit Discard")
            Fixture().cleanup()
            print(format("PASS {} editor menu/dialog checks", checks))
            Gtk.Application().quit()
        }
        stage = stage + 1
    }
}
"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--clang", required=True)
    parser.add_argument("--include", type=Path, required=True)
    parser.add_argument("--native", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--sanitize", default="")
    parser.add_argument("--xvfb", required=True)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=args.work, prefix="editor-dialogs-") as directory:
        work = Path(directory).resolve()
        env = environment(work)
        env["GTK_USE_PORTAL"] = "0"
        native = work / "fixture.o"
        flags = shlex.split(subprocess.check_output(
            ["pkg-config", "--cflags", "gtk4"], text=True))
        if args.sanitize:
            flags += ["-fsanitize=" + args.sanitize, "-fno-omit-frame-pointer"]
        result = invoke([args.clang, "-std=c11", "-Wall", "-Wextra", "-Werror",
                         "-Wno-deprecated-declarations", "-I" + str(args.include.resolve()),
                         "-I" + str(args.native.resolve()), *flags, "-c",
                         str(Path(__file__).with_name("editor_dialog_fixture.c").resolve()),
                         "-o", str(native)], work, env)
        assert result.returncode == 0, result.stderr
        text = args.source.read_text()
        text = text.replace("window.setDefaultSize(1000, 700)",
                            "window.setDefaultSize(1000, 550)")
        index = text.rindex("\nstart {")
        setup = text[index:].replace(
            "app.onActivate(editor.activate)",
            "EditorChecks checks = EditorChecks()\n"
            "    checks.editor = editor\n"
            "    checks.nextAction = checks.next\n"
            "    editor.openResultAction = checks.opened\n"
            "    editor.saveResultAction = checks.savedAs\n"
            "    app.onActivate(checks.activate)")
        source = work / "editor.simp"
        source.write_text(text[:index] + DRIVER + setup)
        (work / "overwrite.simp").write_text("original\n")
        (work / "other.simp").write_text("other\n")
        (work / "vanishing.simp").write_text("removed before reading\n")
        result = invoke([str(args.compiler.resolve()), str(source), str(native),
                         "-o", str(work / "editor")], work, env)
        assert result.returncode == 0, result.stderr
        with headless(args.xvfb, work, env) as gui_env:
            try:
                result = invoke([str(work / "editor")], work, gui_env, timeout=60)
            except subprocess.TimeoutExpired as error:
                raise AssertionError(f"Editor timed out: {error.stderr!r}") from error
            assert result.returncode == 0, result
            assert result.stdout.splitlines()[-1].startswith("PASS "), result
            assert not result.stderr, result.stderr
            print(result.stdout.splitlines()[-1])


if __name__ == "__main__":
    main()
