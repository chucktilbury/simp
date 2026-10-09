"""Real GTK controls/actions plus native property checks; every run has private HOME/XDG."""

import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import tomllib

from gtk_support import headless
from program_runner import environment, invoke


DRIVER = r"""
class PreferenceFixture {
    void collect()
    bool shortcut(String trigger)
    bool visible()
    bool click(String label)
    bool close()
    bool commandVisible(String name)
    bool commandClick(String name, String label)
    bool views(int size, int tabs, bool spaces, bool numbers, bool wrap, bool highlight)
    bool record(String trigger)
    void tick()
}
void PreferenceFixture.collect() from "fixture_editor_collect"
bool PreferenceFixture.shortcut(String trigger) from "fixture_editor_shortcut"
bool PreferenceFixture.visible() from "fixture_preferences_visible"
bool PreferenceFixture.click(String label) from "fixture_preferences_click"
bool PreferenceFixture.close() from "fixture_preferences_close"
bool PreferenceFixture.commandVisible(String name) from "fixture_preferences_command_visible"
bool PreferenceFixture.commandClick(String name, String label) from "fixture_preferences_command_click"
bool PreferenceFixture.views(int size, int tabs, bool spaces, bool numbers, bool wrap, bool highlight)
    from "fixture_preferences_views"
bool PreferenceFixture.record(String trigger) from "fixture_preferences_record"
void PreferenceFixture.tick() from "fixture_preferences_tick"

class PreferencesChecks {
    CwhipEditor editor
    callback<void()> nextAction
    callback<void(String)> shutdownAction
    bool shutdownCallbackRan
    int stage
    int checks
    int waits
    String mode
    void require(bool condition, String label) {
        if (!condition) {
            String message = editor.settings.errors
            if (editor.preferencesMessage != null) { message = editor.preferencesMessage.text() }
            System.StandardIO().writeErrorLine(format("FAIL stage {}: {}: {}", stage, label, message))
            System.Process().exit(1)
        }
        checks = checks + 1
    }
    KeyboardPreference row(String command) {
        int i = 0
        while (i < editor.keyboardRows.length) {
            KeyboardPreference item = editor.keyboardRows[i] as KeyboardPreference
            if (item.binding.id.equals(command)) { return item }
            i = i + 1
        }
        return null
    }
    void activate() {
        mode = System.Process().getEnv("PREFERENCE_CASE")
        editor.activate()
        Gtk.Application().post(nextAction)
    }
    void shutdownSaved(String error) {
        shutdownCallbackRan = true
        require(error.equals(""), "native save succeeds before shutdown callback")
        Gtk.Application().shutdown()
        print(format("PASS shutdown-saving {}", checks))
    }
    void next() {
        if (editor.savePending) {
            waits = waits + 1
            require(waits < 500, "save completion")
            PreferenceFixture().tick()
            Gtk.Application().post(nextAction)
            return
        }
        PreferenceFixture().collect()
        if (mode.equals("shutdown-saving")) {
            Gtk.Config().save("shutdown.toml", "version = 1\n", "", shutdownAction)
            return
        }
        if (mode.equals("teardown-pending")) {
            Gtk.Config().save("shutdown.toml", "version = 1\n", "", shutdownAction)
            Gtk.Application().shutdown()
            require(!shutdownCallbackRan && System.FileSystem().exists("shutdown.toml"), "shutdown drains write but suppresses callback")
            print(format("PASS teardown-pending {}", checks))
            return
        }
        if (mode.equals("invalid")) {
            require(!editor.settings.writable && !editor.settings.errors.equals(""), "visible load error, saving locked")
            editor.showPreferences()
            require(editor.preferencesMessage.text().contains(System.Process().getEnv("EXPECTED_ERROR")), "specific error")
            editor.sizeEntry.setText("22")
            require(!editor.savePending && !editor.settings.writable, "malformed file is never saved")
            editor.quitAction()
            print(format("PASS invalid {}", checks))
            return
        }
        if (mode.equals("reload")) {
            if (stage == 0) {
                require(editor.settings.fontSize == 18 && editor.settings.tabWidth == 3 &&
                    !editor.settings.insertSpaces && !editor.settings.lineNumbers &&
                    editor.settings.wrap && editor.settings.highlightLine, "startup reload")
                require(PreferenceFixture().views(18, 3, false, false, true, true), "startup view properties")
                require(editor.currentDocument().view.hasShortcut("<Control><Alt>s") &&
                    !editor.currentDocument().view.hasShortcut("<Control>s"), "startup shortcut reload")
                editor.newAction()
                stage = 1
                PreferenceFixture().tick()
                Gtk.Application().post(nextAction)
                return
            }
            require(PreferenceFixture().views(18, 3, false, false, true, true), "new view reload")
            editor.quitAction()
            print(format("PASS reload {}", checks))
            return
        }
        if (mode.equals("legacy")) {
            require(editor.currentDocument().view.hasShortcut("<Control><Alt>s") &&
                !editor.currentDocument().view.hasShortcut("<Control>s") &&
                !editor.currentDocument().view.hasShortcut("<Control>w"), "legacy full-map semantics")
            editor.showPreferences()
            if (stage == 0) {
                editor.sizeEntry.setText("18")
                stage = 1
                Gtk.Application().post(nextAction)
                return
            }
            require(editor.preferencesMessage.text().equals("Settings saved"), "legacy migration save")
            editor.quitAction()
            print(format("PASS legacy {}", checks))
            return
        }
        if (mode.equals("legacy-settings")) {
            if (stage == 0) {
                require(editor.settings.fontSize == 16 &&
                    editor.settings.path.contains("tweed/settings.toml"), "legacy settings fallback")
                editor.showPreferences()
                editor.sizeEntry.setText("19")
                stage = 1
                Gtk.Application().post(nextAction)
                return
            }
            require(editor.preferencesMessage.text().equals("Settings saved"), "legacy settings save")
            editor.quitAction()
            print(format("PASS legacy-settings {}", checks))
            return
        }
        if (mode.equals("precedence")) {
            require(editor.settings.writable, "authoritative TOML ignores obsolete malformed legacy")
            require(editor.currentDocument().view.hasShortcut("<Control><Alt>s") &&
                editor.currentDocument().view.hasShortcut("<Control>w"), "TOML map overrides legacy; missing keys use defaults")
            editor.quitAction()
            print(format("PASS precedence {}", checks))
            return
        }
        if (mode.equals("quit-saving") || mode.equals("fallback")) {
            editor.showPreferences()
            require(!System.FileSystem().exists(editor.settings.path), "first run does not write until edited")
            editor.sizeEntry.setText("20")
            require(editor.savePending, "asynchronous save queued")
            editor.quitAction()
            require(!editor.window.disposed() && editor.closeAfterSave, "close waits for latest settings")
            print(format("PASS {} {}", mode, checks))
            return
        }
        if (mode.equals("coalesced-save-failure")) {
            editor.showPreferences()
            if (stage == 0) {
                editor.sizeEntry.setText("18")
                editor.sizeEntry.setText("19")
                require(editor.savePending && editor.saveAgain, "latest settings coalesced")
                editor.settings.path = "/dev/null/settings.toml"
                editor.quitAction()
                stage = 1
                Gtk.Application().post(nextAction)
                return
            }
            require(!editor.window.disposed() && !editor.closeAfterSave &&
                editor.preferencesMessage.text().contains("save failed"), "failed latest save cancels queued close")
            editor.quitAction()
            print(format("PASS coalesced-save-failure {}", checks))
            return
        }
        if (mode.equals("external-change")) {
            editor.showPreferences()
            System.File file = System.File(editor.settings.path, "w")
            require(file.isOpen(), "external edit fixture")
            file.write("version = [\n")
            file.close()
            editor.sizeEntry.setText("19")
            require(!editor.savePending && editor.preferencesMessage.text().contains("changed on disk"), "external malformed edit preserved")
            editor.quitAction()
            print(format("PASS external-change {}", checks))
            return
        }
        if (mode.equals("save-failure") || mode.equals("async-save-failure") || mode.equals("async-save-conflict")) {
            editor.showPreferences()
            if (stage == 0) {
                if (mode.equals("async-save-failure")) {
                    editor.savePending = true
                    Gtk.Config().save("/dev/null/settings.toml", "version = 1\n", "", editor.settingsSaved)
                } else {
                    if (mode.equals("async-save-conflict")) {
                        editor.savePending = true
                        Gtk.Config().save(editor.settings.path, "version = 1\n", "wrong expected contents", editor.settingsSaved)
                    } else {
                        String blocked = System.FileSystem().join(System.FileSystem().getCwd(), "blocked-config")
                        System.File blocker = System.File(blocked, "w")
                        require(blocker.isOpen(), "non-directory destination fixture")
                        blocker.write("not a directory\n")
                        blocker.close()
                        editor.settings.path = System.FileSystem().join(blocked, "settings.toml")
                        editor.sizeEntry.setText("19")
                    }
                }
                stage = 1
                Gtk.Application().post(nextAction)
                return
            }
            require(editor.preferencesMessage.text().contains("save failed"), "visible write failure")
            require(PreferenceFixture().visible(), "write failure keeps preferences available")
            editor.quitAction()
            print(format("PASS save-failure {}", checks))
            return
        }
        if (stage == 0) {
            require(PreferenceFixture().shortcut("<Control>comma"), "Preferences shortcut dispatch")
            require(PreferenceFixture().visible(), "nonmodal transient Preferences")
            require(PreferenceFixture().close() && !PreferenceFixture().visible(), "close hides")
            editor.menu.activateItem(editor.preferencesItem)
            require(PreferenceFixture().visible(), "menu reopens same window")
            require(PreferenceFixture().click("Editor"), "Editor category control")
            editor.newAction()
            editor.openFile("example.simp")
            require(editor.documents.length == 3, "old, new, opened documents")
            editor.sizeEntry.setText("18")
            editor.tabsEntry.setText("3")
            editor.spacesCheck.setActive(false)
            editor.numbersCheck.setActive(false)
            editor.wrapCheck.setActive(true)
            editor.highlightCheck.setActive(true)
        }
        if (stage == 1) {
            require(PreferenceFixture().views(18, 3, false, false, true, true), "all existing views updated, including font size")
            editor.newAction()
            PreferenceFixture().tick()
        }
        if (stage == 2) {
            require(PreferenceFixture().views(18, 3, false, false, true, true), "subsequently created view updated")
            editor.sizeEntry.setText("73")
            require(editor.settings.fontSize == 18 && editor.preferencesMessage.text().contains("6 to 72"), "numeric range rejection")
            editor.sizeEntry.setText("18")
            editor.fontEntry.setText("not-an-installed-monospace-font")
            require(editor.settings.fontFamily.equals("monospace"), "font validation")
            editor.fontEntry.setText("monospace")
            require(PreferenceFixture().click("Reset Editor to Defaults"), "Editor reset control")
            require(editor.settings.fontSize == 18, "reset waits for confirmation")
            require(PreferenceFixture().click("Cancel"), "cancel reset")
            require(editor.settings.fontSize == 18, "cancel leaves settings")
            require(PreferenceFixture().click("Reset Editor to Defaults") &&
                    PreferenceFixture().click("Confirm Reset"), "confirm reset")
            require(editor.settings.fontSize == 12 && editor.settings.tabWidth == 4, "Editor defaults")
        }
        if (stage == 3) {
            require(PreferenceFixture().views(12, 4, true, true, false, false), "reset propagated")
            editor.sizeEntry.setText("18")
            editor.tabsEntry.setText("3")
            editor.spacesCheck.setActive(false)
            editor.numbersCheck.setActive(false)
            editor.wrapCheck.setActive(true)
            editor.highlightCheck.setActive(true)
            require(PreferenceFixture().click("Keyboard"), "Keyboard category control")
            editor.commandSearch.setText("save")
            require(editor.keyboardRows.length == 22, "named command rows")
            require(PreferenceFixture().commandVisible("Save") && !PreferenceFixture().commandVisible("Quit"), "command search filters actual rows")
            editor.commandSearch.setText("")
            KeyboardPreference save = row("save")
            save.entry.setText("<Control>q")
            require(save.errorLabel.text().contains("conflicts") && save.binding.trigger.equals("<Control>s"), "conflict does not override")
            save.entry.setText("not-a-trigger")
            require(save.errorLabel.text().contains("Invalid") && save.binding.trigger.equals("<Control>s"), "invalid trigger does not override")
            require(PreferenceFixture().commandClick("Save", "Record"), "Record control")
            require(PreferenceFixture().record("<Control><Alt>s"), "real recorder controller")
            require(save.binding.normalized.equals(Gtk.Keyboard().normalize("<Control><Alt>s")), "recorded binding applied")
            require(editor.currentDocument().view.hasShortcut(save.binding.trigger) &&
                !editor.currentDocument().view.hasShortcut("<Control>s"), "rebind replaces old registration")
        }
        if (stage == 4) {
            KeyboardPreference save = row("save")
            require(PreferenceFixture().commandClick("Save", "Reset"), "individual Reset control")
            require(save.binding.trigger.equals("<Control>s"), "individual reset")
            save.setTrigger("<Control><Alt>s")
            require(PreferenceFixture().commandClick("Find", "Disable"), "Disable control")
            require(!editor.currentDocument().view.hasShortcut("<Control>f"), "disabled binding removed")
            require(PreferenceFixture().click("Reset Keyboard to Defaults"), "Keyboard reset control")
            require(!row("find").binding.trigger.equals("<Control>f"), "Keyboard reset confirmation")
            require(PreferenceFixture().click("Confirm Reset"), "confirm Keyboard reset")
            require(editor.currentDocument().view.hasShortcut("<Control>f") &&
                editor.currentDocument().view.hasShortcut("<Control>w") &&
                editor.currentDocument().view.hasShortcut("<Control>q"), "defaults restored")
            save.setTrigger("<Control><Alt>s")
            require(PreferenceFixture().commandClick("Save", "Record"), "record again")
            require(PreferenceFixture().record("Escape") && save.recording == 0 &&
                save.binding.trigger.equals("<Control><Alt>s"), "Escape cancels recording")
            save.record()
            require(PreferenceFixture().close(), "hide during recording")
            require(save.recording == 0, "hide cancels recorder")
            editor.menu.activateItem(editor.preferencesItem)
        }
        if (stage == 5) {
            require(editor.preferencesMessage.text().equals("Settings saved"), "auto-save completed")
            require(PreferenceFixture().visible(), "preferences still open")
            require(PreferenceFixture().shortcut("<Control>w"), "close tab while preferences open")
            require(!editor.window.disposed() && editor.documents.length == 3, "Ctrl+W only closes tab")
            require(PreferenceFixture().shortcut("<Control>f"), "Find default intact")
            require(PreferenceFixture().shortcut("<Control>h"), "Replace default intact")
            require(PreferenceFixture().commandClick("Save", "Record"), "active recorder during parent teardown")
            require(PreferenceFixture().shortcut("<Control>q"), "quit with preferences and search open")
            print(format("PASS ui {}", checks))
            return
        }
        stage = stage + 1
        Gtk.Application().post(nextAction)
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
    with tempfile.TemporaryDirectory(dir=args.work, prefix="preferences-") as directory:
        work = Path(directory).resolve()
        env = environment(work)
        native = work / "fixture.o"
        flags = shlex.split(subprocess.check_output(
            ["pkg-config", "--cflags", "gtk4", "gtksourceview-5"], text=True))
        if args.sanitize:
            flags += ["-fsanitize=" + args.sanitize, "-fno-omit-frame-pointer"]
        result = invoke([args.clang, "-std=c11", "-Wall", "-Wextra", "-Werror",
                         "-Wno-deprecated-declarations", "-I" + str(args.include.resolve()),
                         "-I" + str(args.native.resolve()), *flags, "-c",
                         str(Path(__file__).with_name("editor_dialog_fixture.c").resolve()),
                         "-o", str(native)], work, env)
        assert result.returncode == 0, result.stderr
        text = args.source.read_text()
        index = text.rindex("\nstart {")
        setup = text[index:].replace(
            "app.onActivate(editor.activate)",
            "PreferencesChecks checks = PreferencesChecks()\n"
            "    checks.editor = editor\n"
            "    checks.nextAction = checks.next\n"
            "    checks.shutdownAction = checks.shutdownSaved\n"
            "    app.onActivate(checks.activate)")
        source = work / "preferences.simp"
        source.write_text(text[:index] + DRIVER + setup)
        executable = work / "preferences"
        result = invoke([str(args.compiler.resolve()), str(source), str(native),
                         "-o", str(executable)], work, env)
        assert result.returncode == 0, result.stderr

        def run(name, config=None, legacy=None, expected="", legacy_settings=False):
            case = work / name
            case.mkdir()
            case_env = environment(case)
            case_env["PREFERENCE_CASE"] = name.split("-invalid")[0] if "-invalid" in name else name
            if name.startswith("invalid"):
                case_env["PREFERENCE_CASE"] = "invalid"
            case_env["EXPECTED_ERROR"] = expected
            path = Path(case_env["XDG_CONFIG_HOME"]) / "cwhip/settings.toml"
            if name == "fallback":
                case_env.pop("XDG_CONFIG_HOME")
                path = Path(case_env["HOME"]) / ".config/cwhip/settings.toml"
            if name == "legacy":
                path = Path(case_env["XDG_CONFIG_HOME"]) / "tweed/settings.toml"
                config = "version = 1\n[editor]\nfont_size = 12\n"
            if legacy_settings:
                path = Path(case_env["XDG_CONFIG_HOME"]) / "tweed/settings.toml"
            if config is not None:
                path.parent.mkdir()
                path.write_text(config)
            if legacy is not None:
                (case / "tweed-shortcuts.conf").write_text(legacy)
            (case / "example.simp").write_text("class Example {}\n")
            if name == "invalid-read":
                path.parent.chmod(0)
            try:
                with headless(args.xvfb, case, case_env) as gui_env:
                    result = invoke([str(executable)], case, gui_env, timeout=100)
            finally:
                if name == "invalid-read":
                    path.parent.chmod(0o700)
            assert result.returncode == 0, f"{name}:\n{result.stdout}\n{result.stderr}"
            assert result.stdout.splitlines()[-1].startswith("PASS "), result
            assert not result.stderr, result.stderr
            print(result.stdout.splitlines()[-1])
            if name.startswith("invalid"):
                assert path.read_text() == config, "invalid settings clobbered"
            return path

        original = 'version = 1\n[unrelated]\nkeep = ["one", "two"]\n[editor]\nextra = { nested = true }\n'
        saved = run("ui", original)
        data = tomllib.loads(saved.read_text())
        assert data["unrelated"]["keep"] == ["one", "two"]
        assert data["editor"]["extra"] == {"nested": True}
        assert data["editor"]["font_size"] == 18 and data["editor"]["tab_width"] == 3
        assert data["keyboard"]["save"] == "<Control><Alt>s"
        run("reload", saved.read_text())
        legacy = run("legacy", legacy="<Control><Alt>s=save\n")
        migrated = tomllib.loads(legacy.read_text())
        assert migrated["keyboard"]["save"] == "<Control><Alt>s"
        assert migrated["keyboard"]["close"] == ""
        legacy_settings = run(
            "legacy-settings",
            "version = 1\n[editor]\nfont_size = 16\n",
            legacy_settings=True,
        )
        assert tomllib.loads(legacy_settings.read_text())["editor"]["font_size"] == 19
        assert not (legacy_settings.parents[1] / "cwhip/settings.toml").exists()
        run("precedence", 'version = 1\n[keyboard]\nsave = "<Control><Alt>s"\n', legacy="broken legacy\n")
        run("save-failure")
        run("async-save-failure")
        run("shutdown-saving")
        run("teardown-pending")
        conflict = run("async-save-conflict", "version = 1\n")
        assert conflict.read_text() == "version = 1\n"
        coalesced = run("coalesced-save-failure")
        assert tomllib.loads(coalesced.read_text())["editor"]["font_size"] == 18
        external = run("external-change", "version = 1\n")
        assert external.read_text() == "version = [\n"
        if os.getuid() != 0:
            run("invalid-read", "version = 1\n", expected="Could not read")
        for mode in ("quit-saving", "fallback"):
            path = run(mode)
            assert tomllib.loads(path.read_text())["editor"]["font_size"] == 20
        for name, config, expected in (
            ("syntax", "version = [\n", "Malformed"),
            ("scalar", "version = 1\nunknown = not_toml\n", "Malformed"),
            ("nul", 'version = 1\n[keyboard]\nsave = "\\u0000"\n', "Malformed"),
            ("binary", "version = 1\n\0", "Malformed"),
            ("version", "version = 99\n", "version"),
            ("type", 'version = 1\n[editor]\nfont_size = "large"\n', "integer"),
            ("range", "version = 1\n[editor]\ntab_width = 17\n", "1 to 16"),
            ("table", "version = 1\neditor = []\n", "table"),
            ("trigger", 'version = 1\n[keyboard]\nsave = "invalid syntax"\n', "Invalid"),
            ("conflict", 'version = 1\n[keyboard]\nsave = "<Control>q"\n', "conflicts"),
            ("command", 'version = 1\n[keyboard]\nunknown = "<Control>u"\n', "Unknown"),
        ):
            run("invalid-" + name, config, expected=expected)


if __name__ == "__main__":
    main()
