"""Real menus, asynchronous native choosers, GC/teardown and view allocations."""
import argparse
from pathlib import Path
import shlex
import subprocess

from gtk_support import headless
from program_runner import environment, invoke


NATIVE = """
import gtk
class F {
    void collect()
    int dialogs()
    int finalizedObjects()
    int finalizedConfirmations()
    int receivers()
    void track()
    void respond(String path, bool accept)
    void confirm(bool accept)
    void waitSize(int token, int width, int height, callback<void()> ready)
    void assertMenu(int token)
    void resize(int token, int width, int height)
}
void F.collect() from "binding_collect"
int F.dialogs() from "binding_dialogs"
int F.finalizedObjects() from "binding_finalized_objects"
int F.finalizedConfirmations() from "binding_finalized_confirmations"
int F.receivers() from "binding_receivers"
void F.track() from "binding_track"
void F.respond(String path, bool accept) from "binding_respond"
void F.confirm(bool accept) from "binding_confirm"
void F.waitSize(int token, int width, int height, callback<void()> ready)
    from "binding_wait_size"
void F.assertMenu(int token) from "binding_assert_menu"
void F.resize(int token, int width, int height) from "binding_resize"
"""

SHORTCUT_NATIVE = """
import sourceview
class Keys {
    bool press(int token, String trigger)
}
bool Keys.press(int token, String trigger) from "binding_shortcut"
"""

MENU = """
class Handler {
    void action() { F().collect()
        print("action;") }
}
class Stop {
    void action() { Gtk.Application().shutdown()
        print("stopped;") }
}
class UI {
    Gtk.Window window
    Gtk.MenuBar menus
    int item
    void activate() {
        window = Gtk.Window("Menu bindings")
        menus = Gtk.MenuBar()
        window.setChild(menus)
        int menu = menus.addMenu("File")
        item = menus.addItem(menu, "Open", Handler().action)
        F().assertMenu(menus._widgetToken())
        F().collect()
        menus.activateItem(item)
        menus.setItemEnabled(item, false)
        menus.activateItem(item)
        menus.setItemEnabled(item, true)
        menus.activateItem(item)
        int stop = menus.addItem(menu, "Shutdown", Stop().action)
        menus.activateItem(stop)
        print(menus.disposed())
    }
}
start {
    Gtk.Application app = Gtk.Application("org.simple.BindingMenus")
    app.onActivate(UI().activate)
    app.run()
}
"""

DIALOG = """
class Relay {
    callback<void(String)> target
    Relay(callback<void(String)> action) { target = action }
    void deliver(String path) { F().collect()
        target(path) }
    void note()
    destroy { note() }
}
void Relay.note() from "binding_released"
class UI {
    Gtk.Window window
    Gtk.FileDialog dialog
    callback<void(String)> resultAction
    int phase
    void show(bool save, String path) {
        dialog = Gtk.FileDialog(window, save, path, Relay(resultAction).deliver)
        print(dialog.pending())
        F().track()
        F().collect()
    }
    void result(String path) {
        F().collect()
        print(path)
        print(";")
        print(dialog.pending())
        print(F().dialogs())
        phase = phase + 1
        if (phase == 1) {
            show(false, "")
            F().respond("", false)
            return
        }
        if (phase == 2) {
            show(true, "fresh.tweed")
            F().respond("fresh.tweed", true)
            return
        }
        if (phase == 3) {
            show(true, "existing.tweed")
            F().respond("existing.tweed", true)
            F().confirm(false)
            return
        }
        if (phase == 4) {
            show(true, "existing.tweed")
            F().respond("existing.tweed", true)
            F().confirm(true)
            return
        }
        {
            print(F().finalizedObjects() >= 5)
            print(F().finalizedConfirmations() >= 2)
            show(false, "")
            dialog.cancel()
            dialog.cancel()
            print(dialog.pending())
            print(F().dialogs())
            show(false, "")
            window.dispose()
            print(dialog.pending())
            print(F().dialogs())
            window = Gtk.Window("Shutdown")
            show(false, "")
            Gtk.Application().shutdown()
            print(dialog.pending())
            print(F().dialogs())
        }
    }
    void activate() {
        window = Gtk.Window("Dialog bindings")
        window.present()
        show(false, "existing.tweed")
        F().respond("existing.tweed", true)
    }
}
start {
    Gtk.Application app = Gtk.Application("org.simple.BindingDialogs")
    UI ui = UI()
    ui.resultAction = ui.result
    app.onActivate(ui.activate)
    app.run()
    F().collect()
    print(F().receivers())
}
"""

SOURCEVIEW = """
import sourceview
class UI {
    Gtk.Window window
    GtkSource.View view
    callback<void()> allocatedAction
    callback<void()> resizedAction
    callback<void()> changedAction
    callback<void()> cursorAction
    bool seenUndo
    bool seenRedo
    bool seenSelection
    void changed() { seenUndo = view.canUndo()
        seenRedo = view.canRedo() }
    void cursor() { seenSelection = view.hasSelection() }
    void activate() {
        window = Gtk.Window("Allocation bindings")
        window.setDefaultSize(640, 480)
        Gtk.Box box = Gtk.Box(Gtk.Box.VERTICAL, 0)
        window.setChild(box)
        Gtk.Notebook notebook = Gtk.Notebook()
        notebook.setHExpand(true)
        notebook.setVExpand(true)
        box.append(notebook)
        Gtk.ScrolledWindow scroll = Gtk.ScrolledWindow()
        scroll.setHExpand(true)
        scroll.setVExpand(true)
        notebook.appendPage(scroll, "Document")
        view = GtkSource.View()
        view.widget().setHExpand(true)
        view.widget().setVExpand(true)
        scroll.setChild(view.widget())
        view.setText("one\\ntwo\\nthree\\nfour\\nfive\\nsix\\nseven\\neight\\n")
        view.onChanged(changedAction)
        view.onCursorMoved(cursorAction)
        print(view.hasSelection())
        view.selectAll()
        print(view.hasSelection())
        print(seenSelection)
        view.copy()
        view.cut()
        print(view.text())
        print(view.canUndo())
        print(seenUndo == view.canUndo())
        view.undo()
        print(view.canRedo())
        print(seenRedo == view.canRedo())
        view.redo()
        view.paste()
        window.present()
        view.focus()
        F().waitSize(view._widgetToken(), 500, 300, allocatedAction)
    }
    void allocated() {
        print(view.text().equals("one\\ntwo\\nthree\\nfour\\nfive\\nsix\\nseven\\neight\\n"))
        print("multiline;")
        F().resize(window._widgetToken(), 900, 720)
        F().waitSize(view._widgetToken(), 760, 550, resizedAction)
    }
    void resized() {
        print("grew;")
        Gtk.Entry entry = Gtk.Entry("search")
        entry.focus()
        entry.dispose()
        window.dispose()
        Gtk.Application().quit()
    }
}
start {
    Gtk.Application app = Gtk.Application("org.simple.BindingAllocation")
    UI ui = UI()
    ui.allocatedAction = ui.allocated
    ui.resizedAction = ui.resized
    ui.changedAction = ui.changed
    ui.cursorAction = ui.cursor
    app.onActivate(ui.activate)
    app.run()
}
"""

# Shortcut callbacks that dispose their own view (closing a tab) or the whole
# window (quitting) must not release their in-flight callback registration.
SHORTCUTS = """
class Action {
    Gtk.Window window
    Gtk.Notebook notebook
    Gtk.ScrolledWindow page
    Action(Gtk.Window owner, Gtk.Notebook pages, Gtk.ScrolledWindow closing) {
        window = owner
        notebook = pages
        page = closing
    }
    void run() {
        if (page != null) {
            notebook.remove(page)
            page.dispose()
            F().collect()
            print("closed;")
        } else {
            window.dispose()
            F().collect()
            print("quit;")
            Gtk.Application().quit()
        }
    }
    void note()
    destroy { note() }
}
void Action.note() from "binding_released"
class UI {
    Gtk.Window window
    Gtk.Notebook notebook
    Gtk.ScrolledWindow first
    GtkSource.View firstView
    GtkSource.View secondView
    void activate() {
        window = Gtk.Window("Shortcut bindings")
        notebook = Gtk.Notebook()
        window.setChild(notebook)
        first = Gtk.ScrolledWindow()
        firstView = GtkSource.View()
        first.setChild(firstView.widget())
        notebook.appendPage(first, "one")
        Gtk.ScrolledWindow second = Gtk.ScrolledWindow()
        secondView = GtkSource.View()
        second.setChild(secondView.widget())
        notebook.appendPage(second, "two")
        print(firstView.bindShortcut("<Control>w", Action(window, notebook, first).run))
        print(secondView.bindShortcut("<Control>q", Action(window, notebook, null).run))
        window.present()
        int closing = firstView._widgetToken()
        print(Keys().press(closing, "<Control>w"))
        print(Keys().press(secondView._widgetToken(), "<Control>q"))
    }
}
start {
    Gtk.Application app = Gtk.Application("org.simple.BindingShortcuts")
    app.onActivate(UI().activate)
    app.run()
    F().collect()
    print(F().receivers())
}
"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--clang", default="clang")
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--xvfb", required=True)
    parser.add_argument("--sourceview", action="store_true")
    parser.add_argument("--sanitize", default="")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)
    env = environment(work)
    flags = shlex.split(subprocess.check_output(
        ["pkg-config", "--cflags", "gtksourceview-5" if args.sourceview else "gtk4"],
        text=True))
    if args.sourceview:
        languages = root / "stdlib/sourceview/0.1.0/language-specs"
        flags += ["-DSIMP_GTK_SOURCEVIEW=1",
                  f'-DSIMP_GTK_SOURCEVIEW_SOURCE_LANG_DIR="{languages}"',
                  f'-DSIMP_GTK_SOURCEVIEW_INSTALL_LANG_DIR="{languages}"']
    if args.sanitize:
        flags.append(f"-fsanitize={args.sanitize}")
    native = work / "fixture.o"
    result = invoke([args.clang, "-std=c11", "-Wall", "-Wextra", "-Werror",
                     "-Wno-deprecated-declarations", "-I", str(root / "include"),
                     "-c", str(root / "tests/gtk_bindings_fixture.c"), "-o",
                     str(native), *flags], work, env)
    assert result.returncode == 0, result.stderr
    existing = work / "existing.tweed"
    existing.write_text("unchanged")
    fresh = work / "fresh.tweed"
    fresh.unlink(missing_ok=True)
    dialog = DIALOG.replace('"existing.tweed"', f'"{existing}"').replace(
        '"fresh.tweed"', f'"{fresh}"')
    expected_dialog = (f"true{existing};false0true;false0true{fresh};false0"
                       f"true;false0true{existing};false0true"
                       "truetruefalse0truefalse0truefalse08")
    cases = [("menus", MENU, "action;action;stopped;true"),
             ("dialogs", dialog, expected_dialog)]
    if args.sourceview:
        cases.append(("allocation", SOURCEVIEW, "falsetruetruetruetruetruetruetruemultiline;grew;"))
        cases.append(("shortcuts", SHORTCUT_NATIVE + SHORTCUTS,
                      "truetrueclosed;truequit;true2"))
    with headless(args.xvfb, work, env) as env:
        for name, program, expected in cases:
            source = work / f"{name}.simp"
            source.write_text(NATIVE + program)
            output = work / name
            result = invoke([str(args.compiler.resolve()), str(source), str(native),
                             "-o", str(output)], work, env)
            assert result.returncode == 0, f"{name}: {result.stderr}"
            result = invoke([str(output)], work, env)
            assert result.returncode == 0, f"{name}: {result.stderr}\n{result.stdout}"
            assert result.stdout == expected, (name, result.stdout, expected, result.stderr)
            assert "CRITICAL" not in result.stderr and "ERROR" not in result.stderr, result.stderr
            assert existing.read_text() == "unchanged" and not fresh.exists()
            print(f"PASS {name}")


if __name__ == "__main__":
    main()
