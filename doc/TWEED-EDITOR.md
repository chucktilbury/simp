# Tweed editor

`examples/editor/tweed.simp` is a small Simple-written GTK editor. It keeps the
compiler and package names unchanged; Tweed Lang is the editor/application
branding, not a repository-wide rename. The editor uses the optional GTK
package and a separate GtkSourceView-backed `sourceview` package. It provides
File and Edit menus, native open/save-as file choosers, multiple notebook tabs,
duplicate-path focusing, open/save/save-as, session
recent files, dirty tab titles, close protection, undo/redo, find/replace,
line/column status, and configurable GTK keyboard shortcuts. GTK's text view
provides ordinary selection, cursor navigation, and clipboard bindings.

The editor's source adapter is `GtkSource.View`. It exposes managed multiline
text buffers, undo/redo, modified state, cursor coordinates, search/replace,
syntax-context queries, and shortcut registration while remaining attachable
to existing `Gtk.Widget` containers. The `tweed` GtkSourceView language
definition highlights Tweed Lang keywords/types, strings, comments, and
numbers. Documents are represented separately from notebook pages; opening an
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

Build with the explicit GtkSourceView opt-in:

```sh
cmake -S . -B build-tweed -DCMAKE_BUILD_TYPE=Debug \
  -DSIMP_GTK=ON -DSIMP_GTK_SOURCEVIEW=ON \
  -DSIMP_STAGE_PREFIX="$PWD/build-tweed/stage"
cmake --build build-tweed -j4
build-tweed/stage/bin/simp examples/editor/tweed.simp \
  -o build-tweed/tweed-editor
build-tweed/tweed-editor file1.simp file2.simp
```

The `sourceview` package is staged and installed only with
`-DSIMP_GTK_SOURCEVIEW=ON`, which requires `-DSIMP_GTK=ON`, GTK 4, and
GtkSourceView 5 development files. Headless GTK/editor tests additionally
require Xvfb and `dbus-daemon`. With both options disabled, ordinary compiler
and non-GUI applications do not link GTK or GtkSourceView.

**File > New** creates an editable untitled tab. **Open...** uses a native GTK
file chooser; selecting an already-open file focuses its existing tab.
**Save** writes the active document to its current path, or opens **Save As...**
for an untitled document. Save As asks for confirmation before replacing an
existing file, and refuses a destination belonging to another open document.
Cancelling a chooser or overwrite confirmation leaves files and document
identities unchanged. Open/read/write/flush/close failures appear in the status
line. The editor supports local filesystem paths; nonlocal chooser URIs are
explicitly rejected rather than interpreted as cancellation.
**Recent** reopens the last unique file added to the session history.
Command-line file arguments open tabs at startup.

**Edit** offers Undo, Redo, Cut, Copy, Paste, Select All, Find, and Replace.
Find and Replace focus their respective search-row fields; **Find next** and
**Replace next** execute the entered query. Undo/Redo and Cut/Copy menu items
track the active buffer's undo/redo and selection state. File dialogs are modal,
asynchronous, parented to the editor window, and cancelled on parent teardown;
they do not run nested event loops.

A modified tab is marked with `*`. **File > Close**, **Quit**, and a window-close
request offer in-app **Save**, **Discard**, and **Cancel** controls. Saving an
untitled document during close opens Save As; cancelling that chooser cancels
the pending close without discarding changes. For a window close with several
modified documents, Save handles them one at a time until all are saved.

Default shortcuts use GTK trigger notation: `<Control>s` saves,
`<Control>o` opens the file chooser, `<Control>n` creates a tab,
`<Control>f` focuses Find, `<Control>h` focuses Replace, `<Control>z` undoes,
and `<Control><Shift>z` redoes. `<Control><Shift>s` opens Save As,
`<Control>w` closes the active tab, and `<Control>q` requests Quit.
`<Control>x`, `<Control>c`, `<Control>v`, and `<Control>a` perform Cut, Copy,
Paste, and Select All. Menus and configurable shortcuts resolve to the same
named command callbacks. To change or remove defaults, put a
`tweed-shortcuts.conf` file in the editor's working directory. Each nonblank
line maps one GTK trigger to one command:

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

Commands are `save`, `save-as`, `open`, `new`, `close`, `quit`, `find`, `replace`,
`undo`, `redo`, `cut`, `copy`, `paste`, and `select-all`.
Duplicate triggers, unknown commands, and unparseable triggers are reported in
the status line rather than silently rebound.

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
extension/plugin runtime are not included yet.
Any future in-process plugins should be treated as trusted code.

Run the editor-specific headless cases and existing GTK integration checks:

```sh
ctest --test-dir build-tweed \
  -R '^(simp_example_editor_tweed\.simp|simp_editor_default_shortcuts|simp_editor_dialogs|simp_gtk|simp_gtk_bindings)$' \
  --output-on-failure
```
