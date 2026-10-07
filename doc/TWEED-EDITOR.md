# Tweed editor

`examples/editor/tweed.simp` is a small Simple-written GTK editor. It keeps the
compiler and package names unchanged; Tweed Lang is the editor/application
branding, not a repository-wide rename. The editor uses the optional GTK
package and a separate GtkSourceView-backed `sourceview` package. It provides
File and Edit menus, native open/save-as file choosers, multiple notebook tabs,
duplicate-path focusing, open/save/save-as, session
recent files, dirty tab titles, close protection, undo/redo, find/replace,
line/column status, a non-modal Preferences window, and configurable GTK keyboard shortcuts. GTK's text view
provides ordinary selection, cursor navigation, and clipboard bindings.

The editor's source adapter is `GtkSource.View`. It exposes managed multiline
text buffers, undo/redo, modified state, cursor coordinates, search/replace,
syntax-context queries, and shortcut registration while remaining attachable
to existing `Gtk.Widget` containers. The document editor uses GtkSourceView's
monospace setting; the rest of the application keeps the platform's normal
font. The `tweed` GtkSourceView language
definition highlights Tweed Lang keywords/types, strings, comments, and
numbers. Highlighting is enabled only for recognized source files: a document's
language is chosen from its file name (`*.simp` and `*.tweed` select `tweed`),
and untitled documents, `.txt` files, and other unrecognized names are plain
text. Save As re-evaluates the language for the new name. Additional languages
are added by installing a GtkSourceView `<id>.lang` definition in the
sourceview package (see [GTK source editing](GTK.md#language-definitions));
only Tweed is shipped. Documents are represented separately from notebook pages; opening an
already-open path selects its existing tab. `openDocument(path)` returns the
existing or newly opened document independently of selection, and
`activateDocument(document)` selects its tab; a future Project Explorer can use
these same APIs. The notebook, scrolled viewport, and source view explicitly
expand horizontally and vertically, so the multiline editor fills the remaining
window space and grows when the window is resized; the menu, search controls,
close prompt, and status line retain their natural height.

## Build and run

On Debian/Ubuntu, install the compiler/build tools and optional GUI development
and headless-test dependencies:

```sh
sudo apt install build-essential cmake clang python3 pkg-config \
  libgtk-4-dev libgtksourceview-5-dev xvfb dbus-daemon
```

Installed GUI development dependencies are detected on fresh configurations.
Build the editor directly:

```sh
cmake -S . -B build-tweed -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
make -C build-tweed -j4 tweed
./bin/tweed file1.simp file2.simp
```

Inside the configured Makefiles build directory, use `make tweed`; with any
generator, use `cmake --build build-tweed --target tweed -j4`. The target builds
the compiler and required support on a fresh build, generates a private package
lock from staged modules, and compiles the editor only when its inputs change.
An ordinary compiler build prepares the enabled peripheral support but does
not compile the editor. The executable is `<source>/bin/tweed` by default;
with `-DSIMP_STAGE_PREFIX=/absolute/prefix` it is `/absolute/prefix/bin/tweed`.
No manual compiler invocation or package activation is needed. Tweed is not
currently an install target.

Headless tests require Xvfb and `dbus-daemon`; they default to enabled when
those tools are present, otherwise configuration explicitly reports them
disabled without blocking Tweed. Set `-DSIMP_GTK_TESTS=ON` to require them, or
`-DBUILD_TESTING=OFF` to omit all tests. Compiler-only builds
can set `-DSIMP_GTK=OFF -DSIMP_GTK_SOURCEVIEW=OFF`; their `tweed` target reports
the missing support explicitly. To enable GUI support in an already configured
compiler-only build, reconfigure with both options `ON`. Even with GUI packages
enabled, the compiler and non-import applications do not link GTK or
GtkSourceView. See [build and installation](INSTALLATION.md).

**File > New** creates an editable untitled tab. **Open...** uses a native GTK
file chooser that allows selecting several files; each selected file is opened
(an already-open file focuses its existing tab) and the last one becomes active.
A failure for one file does not stop the others: the status line reports the
failure (or `Open failed for N of M files`), and one error dialog lists every
file that could not be opened. Binary files (containing NUL bytes) and files
that are not valid UTF-8 are refused with a "not a text file" error dialog;
they are never lossily decoded, and existing documents are unchanged.
**Save** writes the active document to its current path, or opens **Save As...**
for an untitled document. Save As asks for confirmation before replacing an
existing file, and refuses a destination belonging to another open document.
Cancelling a chooser or overwrite confirmation leaves files and document
identities unchanged. Open/read/write/flush/close failures appear in the status
line; Open failures from the chooser or binary files also show a modal error
dialog parented to the editor window. The editor supports local filesystem paths; nonlocal chooser URIs are
explicitly rejected rather than interpreted as cancellation.
**Recent** reopens the last unique file added to the session history.
Command-line file arguments open tabs at startup.

**Edit** offers Undo, Redo, Cut, Copy, Paste, Select All, Find, Replace, and Preferences.
Find and Replace open a separate, non-modal window transient for the editor.
The Find window contains the query and Find next; Replace adds the replacement
field and Replace next / Replace all. Closing the window hides it and preserves
the query/options for next time. The dialog follows the active document: its
search uses that tab's buffer, and in-selection bounds are stored separately
per document. While it is open, switching to a tab with selected text captures
that selection as the tab's search scope; tabs without a selection retain any
existing scope. If text is selected on a single line when Find or Replace
is opened, it replaces the Find query; otherwise the previous query is kept.
**Find next** selects the next match after the current selection,
wrapping around once; **Replace next** replaces the selected match (or the next
one), and **Replace all** replaces every match in one undoable step and reports
the count. The **Whole words**, **Case sensitive**, and **In selection** check
boxes apply to all three. Searches are case-insensitive substring matches by
default. The **In selection** scope is the selection active when Find or
Replace (menu or shortcut) is invoked, and its bounds follow replacements; a
selection that is just the last Find match keeps the existing scope, and
invoking Find or Replace with no selection clears it. With **In selection**
checked but no scope, the status line asks you to select text first. Undo/Redo and Cut/Copy menu items
track the active buffer's undo/redo and selection state. File dialogs are modal,
asynchronous, parented to the editor window, and cancelled on parent teardown;
they do not run nested event loops.

A modified tab is marked with `*`. **File > Close** (`<Control>w`) closes only
the active tab; closing the last tab leaves the window open with a fresh
untitled document. Only **Quit** (`<Control>q`) and a window-close request exit
the editor. Close, Quit, and a window-close request offer in-app **Save**,
**Discard**, and **Cancel** controls for modified documents. Saving an
untitled document during close opens Save As; cancelling that chooser cancels
the pending close without discarding changes. For a window close with several
modified documents, Save handles them one at a time until all are saved.

Default shortcuts use GTK trigger notation: `<Control>s` saves,
`<Control>o` opens the file chooser, `<Control>n` creates a tab,
`<Control>f` focuses Find, `<Control>h` focuses Replace, `<Control>z` undoes,
and `<Control><Shift>z` redoes. `<Control><Shift>s` opens Save As,
`<Control>w` closes the active tab, and `<Control>q` requests Quit.
`<Control>x`, `<Control>c`, `<Control>v`, and `<Control>a` perform Cut, Copy,
Paste, and Select All. `<Control>comma` opens Preferences. Menus and configurable
shortcuts resolve to the same named command callbacks.

## Preferences and settings

**Edit > Preferences...** or **Ctrl+,** opens a non-modal window with Editor
and Keyboard navigation on the left. Closing it hides it; reopening preserves
the selected page. Settings are global: changes apply immediately to all open
documents and to subsequently created/opened documents, without changing file
contents, modified state, language detection, or search options.

The **Editor** page controls an installed monospace font family (or the generic
`monospace`), font size, tab width, insertion of spaces vs tabs, line numbers,
wrapping, and current-line highlighting. Font names must be 1-128 UTF-8 bytes
without control characters; proportional/unknown families are rejected.
Font size is an integer **6-72 points**, and tab width is an integer **1-16
columns**. Defaults are `monospace`, 12 pt, 4 columns, spaces, line numbers on,
wrapping off, and current-line highlighting off. Changing tab width/insertion
does not rewrite existing indentation.

The **Keyboard** page searches named commands, displays their current GTK
trigger strings, and permits editing or recording one key combination. Click
**Record**, then press the combination; modifiers alone keep recording and
Escape cancels. Starting another recording, switching categories, or hiding
Preferences cancels the previous recorder. Invalid syntax and conflicts
(compared using canonical GTK triggers, not raw spelling) show an error without
replacing either command. **Disable** removes a binding; **Reset** restores one
binding unless its default would conflict. Each section's **Reset to Defaults**
requires **Confirm Reset**; **Cancel** changes nothing. Only document command
shortcuts are configurable; GTK's ordinary text navigation remains native.

Settings save automatically to
`$XDG_CONFIG_HOME/tweed/settings.toml`, or
`$HOME/.config/tweed/settings.toml` when XDG_CONFIG_HOME is unset/empty.
The config root must be absolute. The settings file is user-editable UTF-8 TOML,
with a 64 KiB limit and this version-1 schema:

```toml
version = 1

[editor]
font_family = "monospace"
font_size = 12
tab_width = 4
insert_spaces = true
line_numbers = true
wrap = false
highlight_current_line = false

[keyboard]
save = "<Control>s"
close = "<Control>w"
quit = "<Control>q"
find = "<Control>f"
replace = "<Control>h"
preferences = "<Control>comma"
```

Editor keys are optional and use defaults when absent. Keyboard keys are
command IDs (`new`, `open`, `save`, `save-as`, `close`, `quit`, `find`, `replace`,
`undo`, `redo`, `cut`, `copy`, `paste`, `select-all`, `preferences`); omitted IDs
use defaults and `""` explicitly disables one. Unknown keyboard commands are
errors. A missing file is normal first-run behavior: no file is created until
a valid setting changes.

Malformed TOML, wrong types/ranges, invalid/conflicting shortcuts, read errors,
unsupported/missing versions, and strings with encoded NULs are reported in a
startup alert and Preferences. Valid entries still apply, invalid editor entries
use defaults, and invalid/conflicting bindings are not installed. Automatic
saving is disabled for that session, so the original file is not clobbered;
repair it and restart. Preference edits can still apply for the current session.
Write failures are displayed explicitly and never reported as saved.

Saving uses asynchronous GIO atomic replacement on the local filesystem,
with private file permissions, an expected-content check and an ETag check
against concurrent edits. Changes arriving during a save are coalesced into
the next save. Quit waits for the latest queued save; a write failure keeps
Preferences available rather than claiming success. Unknown valid keys/tables
outside the keyboard command map are retained. Serialization normalizes
formatting and does not retain comments; nested unrelated tables/arrays may
be emitted inline. External edits/removal require restarting before saving.

### Legacy shortcut compatibility

`tweed-shortcuts.conf` in the working directory is still supported when the
TOML file is absent or has no `[keyboard]` table. Each nonblank line maps one
GTK trigger to one command:

```text
<Control>s=save
<Control>o=open
<Control>n=new
<Control>f=find
<Control>h=replace
<Control>z=undo
<Control><Shift>z=redo
<Control><Shift>s=save-as
<Control>w=close
<Control>q=quit
<Control>x=cut
<Control>c=copy
<Control>v=paste
<Control>a=select-all
```

As before, a legacy file **replaces** the default shortcut map: commands omitted
from it remain disabled. Preferences adds its new Ctrl+, binding only if the
legacy map does not already use that trigger and does not configure Preferences
itself. The menu always remains available. An existing TOML `[keyboard]` table is authoritative
and the legacy file is ignored, even if malformed; missing TOML command keys
then use built-in defaults, not legacy bindings.

The first successful preferences edit saves the complete effective map
(including disabled commands) into TOML, migrating without deleting or
rewriting the legacy file. Subsequent starts use TOML. Duplicate triggers,
duplicate command lines (multiple bindings for one command cannot be represented
by this increment), unknown commands, empty legacy triggers, and invalid triggers
are reported explicitly, and automatic migration/saving is disabled until
repaired. A conflicting later legacy line never overrides the earlier command.

## Deliberately deferred

There is no language server in this iteration. The repository does not provide
a real Tweed LSP server, and the current process APIs do not yet constitute a
complete JSON-RPC client with correct Content-Length framing, split-message
handling, ordered writes, request correlation, cancellation, notifications,
and shutdown. The editor does not mislabel compiler checking as LSP. A future
LSP client should be a separate configurable component attached to the document
and buffer APIs.

The requested directory-tree Project Explorer is also deferred. The current
document-open/focus and notebook APIs are the intended integration points for
that panel. Persistent recent history, native per-tab close glyphs, and an
extension/plugin runtime, and a configurable toolbar are not included yet. The
dialog controls and menu/shortcut commands use the same editor actions, so a
future toolbar can reuse those actions.
Any future in-process plugins should be treated as trusted code.

Appearance, Files/session, Projects, Build, and LSP preference categories and
project/file-type overrides are future work; no placeholder pages are installed.

Run the editor-specific headless cases and existing GTK integration checks:

```sh
ctest --test-dir build-tweed \
  -R '^(simp_tweed_binary|simp_tweed_shortcuts|simp_tweed_preferences|simp_editor_dialogs|simp_example_editor_tweed\\.simp|simp_editor_default_shortcuts|simp_gtk|simp_gtk_bindings)$' \
  --output-on-failure
```
