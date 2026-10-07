# Tweed editor (first iteration)

`examples/editor/tweed.simp` is a small Simple-written GTK editor. It keeps the
compiler and package names unchanged; Tweed Lang is the editor/application
branding, not a repository-wide rename. The editor uses the optional GTK
package and a separate GtkSourceView-backed `sourceview` package. It provides
multiple notebook tabs, duplicate-path focusing, open/save/save-as, session
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
these same APIs.

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

Use the path field with **Open** or **Save As**; **Save** writes the active
document to its current path. The **Recent** button reopens the last file opened
or saved during this editor session. Command-line file arguments open tabs at
startup. A modified tab is marked with `*`. **Close Tab** and a window-close
request offer in-app **Save**, **Discard**, and **Cancel** controls; saving an
untitled document uses the path field. For a window close with several modified
documents, Save handles them one at a time until all are saved.

Default shortcuts use GTK trigger notation: `<Control>s` saves,
`<Control>o` opens the path in the field, `<Control>n` creates a tab,
`<Control>f` finds, `<Control>h` replaces, `<Control>z` undoes, and
`<Control><Shift>z` redoes. To change or remove defaults, put a
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
```

Commands are `save`, `open`, `new`, `find`, `replace`, `undo`, and `redo`.
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
  -R '^(simp_example_editor_tweed\\.simp|simp_editor_default_shortcuts|simp_gtk)$' \
  --output-on-failure
```
