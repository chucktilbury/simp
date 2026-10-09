# GTK 4 interface

The optional `gtk` package (`0.1.0`, `import gtk`, namespace `Gtk`) provides a
compact GTK-aligned interface: `Application`, `Widget`, `Window`, `Box`, `Label`,
`Button`, `Entry`, `CheckButton`, `ScrolledWindow`, `Notebook`, `MenuBar`,
`FileDialog`, `Paned`, `Tree`, `DirectoryScan`, and `SignalConnection`, plus the optional `sourceview` package.
It is not a complete GTK binding or a generic GUI framework.

## Build and package linkage

Fresh configurations enable `CWHIP_GTK` when GTK 4.10 or newer development files and
`pkg-config` are available, and enable `CWHIP_GTK_SOURCEVIEW` when GtkSourceView 5
is also available. Explicit `ON` options require the corresponding dependencies.
With `BUILD_TESTING=ON`, headless tests default to enabled when Xvfb and
`dbus-daemon` are found; `CWHIP_GTK_TESTS=ON` requires them explicitly.
With `CWHIP_GTK=OFF` and `CWHIP_GTK_SOURCEVIEW=OFF`, the GTK package is absent from staged/installed
standard modules. With it on, `lib/libcwhip_gtk.a` resides inside
`share/cwhip/modules/gtk/0.1.0/`, beside the source and manifest.

Package `[link]` metadata links importing applications with `cwhip_gtk`, `gtk-4`,
`gio-2.0`, `gobject-2.0`, `glib-2.0`, `pango-1.0`, and `pangocairo-1.0`. The compiler and non-import applications
remain unlinked to GTK. Compile-only objects retain package link sidecars.
Installed consumers use the normal `cwhip-pkg init` / `cwhip-pkg install` lock flow.
GTK runtime libraries and a usable display are external requirements; they
are not bundled. The native platform remains POSIX.

## Application and activation

Construct exactly one `Application(String id)` on the registered Cwhip thread
that will own GTK. The ID must be a valid GApplication ID, such as
`"org.example.Editor"`. `onActivate(callback<void()> action)` returns a connection;
install activation handlers **before** `run()`. Create windows and widgets
during local activation or afterward on the same GUI thread, not before `run()`.
Each `Window` automatically associates with this `GtkApplication`.

`run()` calls the real `g_application_run()`: GTK handles desktop activation,
application registration, and normal single-instance behavior on the session
bus. A second process with the same ID forwards activation to the primary
process; its local activation handlers do not run. Activation can occur more
than once in the primary process, so handlers should present existing windows
when appropriate. No command-line/open-file API is provided yet; the GTK run
call does not consume Cwhip's program arguments.

Closing the last application window normally ends the run. `quit()` requests
exit and requires an active run; it does not pretend that closing was approved.
`run()` is single-use and **terminal**: on return it performs `shutdown()`.
`shutdown()` is GUI-only, terminal and idempotent, and is safe inside a handler
or before running. It cancels pending posts, disconnects every signal, disposes
all widgets (including detached widgets), and releases the native application.
Native application release is kept safe while a run or emission is unwinding.
Outstanding `Config.save` operations are drained before shutdown; their
completion callbacks are suppressed during teardown and failures go to stderr.
Reinitialization and nested runs are rejected.

`Window.close()` sends GTK's close request and requires a realized/presented
window; use `dispose()` to tear down a window that has not been presented.
Calling close before presentation is rejected rather than silently doing nothing.

`String id()` returns a managed copy of the application ID while live.
`int post(callback<void()> action)` is the only worker-callable GUI operation;
it queues one invocation on the GUI thread and returns a positive, process-unique
token. `bool cancel(int token)` is GUI-only: true means pending work was removed,
false means it was already cancelled/running/completed or was never pending.
Tokens are never reused. Posts before initialization or after shutdown abort
with an explicit diagnostic. Coordinate worker completion before requesting
exit if workers might still submit work. Posts do not independently hold a GUI
application open: keep an application window alive until needed work finishes.

For foundation compatibility, `Application()` is a stateless reference to this
same process-wide lifecycle (useful for `post`, `cancel`, `quit`, and `shutdown`
inside handlers). It creates no second application. Legacy
`Application().initialize()` initializes a real GtkApplication with ID
`org.cwhip.Scheduler` and an explicit hold until `quit`/`shutdown`, so
scheduler-only foundation programs still work. Prefer the ID constructor for
new GUI programs. The old custom context-iteration loop is gone; repeated
`run()` and connecting/posting after a run intentionally no longer work.

## Widgets and properties

`Widget` is a protected construction base, not a raw-handle wrapper users
instantiate directly. Every concrete widget inherits:

```text
void dispose()
bool disposed()
void setVisible(bool value)
void setSensitive(bool value)
void setHExpand(bool value)
void setVExpand(bool value)
```

The first-release additions are:

| Class | Constructor and operations |
| --- | --- |
| `Window` | `Window(String title)`, `setTitle(String)`, `String title()`, `setDefaultSize(int width, int height)`, `int defaultWidth()`, `int defaultHeight()`, `setTransientFor(Window parent)`, `setChild(Widget)`, `remove(Widget)`, `present()`, `close()`, `onCloseRequest(callback<bool()>)` |
| `TransientWindow` | `TransientWindow(String title)`, `setTitle(String)`, `String title()`, `setDefaultSize(int width, int height)`, `int defaultWidth()`, `int defaultHeight()`, `setTransientFor(Window parent)`, `setChild(Widget)`, `remove(Widget)`, `present()`, `close()`, `onCloseRequest(callback<bool()>)` |
| `Box` | `Box(int orientation, int spacing)`, `setLayout(int orientation, int spacing)`, `append(Widget)`, `remove(Widget)` |
| `Label` | `Label(String text)`, `setText(String)`, `String text()` |
| `Button` | `Button(String label)`, `setLabel(String)`, `String label()`, `onClicked(callback<void()>)` |
| `Entry` | `Entry(String text)`, `setText(String)`, `String text()`, `focus()`, `onChanged(callback<void(String)>)` |
| `CheckButton` | `CheckButton(String label)`, `setLabel(String)`, `String label()`, `setActive(bool)`, `bool active()`, `onToggled(callback<void(bool)>)` |
| `ScrolledWindow` | `ScrolledWindow()`, `setChild(Widget)`, `remove(Widget)` |
| `Notebook` | `Notebook()`, `appendPage(Widget, String)`, `int currentPage()`, `setCurrentPage(int)`, `int pageCount()`, `setPageTitle(int, String)`, `remove(Widget)`, `onPageChanged(callback<void()>)` |
| `Paned` | `Paned()`, `append(Widget)`, `setPosition(int)`, `int position()` |
| `MenuBar` | `MenuBar()`, `int addMenu(String)`, `int addItem(int menu, String label, callback<void()>)`, `setItemEnabled(int item, bool)`, `activateItem(int item)` |

Methods without a listed result return `void`, except signal registration,
which returns `SignalConnection`. Orientation is `Gtk.Box.HORIZONTAL` (`0`) or
`Gtk.Box.VERTICAL` (`1`); spacing is nonnegative and must fit GTK's integer range.
Default window dimensions must be positive and fit that range.
`defaultWidth`/`defaultHeight` return GTK's current default size, which GTK 4
updates when the user resizes an unmaximized window (`0` means unset); use them
to persist window size.
Text accepts managed UTF-8 strings, including empty strings, but not null,
embedded NULs, or invalid UTF-8. Getters return managed copies.

`Window`, `TransientWindow` and `ScrolledWindow` have one child; remove it before
attaching a replacement. `Window` is associated with the `Gtk.Application`;
`TransientWindow` is a standalone top-level. `setTransientFor` establishes the
native transient relationship, with GTK destroy-with-parent enabled. Destroying
the parent destroys the transient and releases its registrations; hiding it
does not. `Box` accepts multiple children. A child must be live and unparented;
windows cannot be children. Self-parenting, cycles, already-parented children,
occupied single-child containers, and removing from the wrong parent are errors.
GTK may internally insert a viewport in a scrolled window; the Cwhip ownership
relationship still refers to the child supplied by the caller.

Expansion uses GTK's actual `hexpand`/`vexpand` layout properties. For an editor,
expand the notebook, scrolled window and source widget vertically and horizontally
so they receive the window's available viewport, rather than only one line's
natural height. `focus()` requests keyboard focus; the target must be in a
presented window to receive it.

## Menus and asynchronous file dialogs

`MenuBar` is a real `GtkPopoverMenuBar` backed by `GMenu` and `GSimpleAction`.
`addMenu(label)` returns a zero-based submenu index. `addItem(menu, label, action)`
returns a positive, unique item token scoped to that menu bar. Actions retain
their typed callback receivers until menu disposal or application shutdown.
`setItemEnabled` updates the actual GTK action state. `activateItem` invokes
that same action, including GTK's disabled-action check; disabled items do not
invoke callbacks. An action may dispose its own menu/window or shut down.

```text
Gtk.FileDialog(Window parent, bool save, String initialPath,
               callback<void(String)> result)
void cancel()
bool pending()
```

Construction shows a modal, parented `GtkFileDialog` asynchronously:
`save=false` opens an existing file; `save=true` chooses a save destination.
An empty initial path uses GTK's default location. A directory selects the
starting folder; a filename selects the open file or initializes the save
folder/name. Local selections return filesystem paths. Nonlocal selections
return their URI, allowing applications to explicitly report unsupported
locations rather than silently treating acceptance as cancellation.

User acceptance invokes the rooted typed callback once with a copied managed
path or URI. User cancellation invokes it once with `""`. Save choosers
use GTK 4's built-in asynchronous overwrite confirmation (including native
platform/portal equivalents). A destination is delivered only after replacement
has been accepted. Declining replacement leaves the chooser open, allowing
another name or cancellation; the binding does not add a second confirmation.

`cancel()` is idempotent programmatic cancellation and **does not invoke** the
result callback. Parent destruction/disposal and application shutdown likewise
cancel pending choosers and release their callback roots without invocation.
`pending()` becomes false before the result handler runs and remains usable
after cancellation or shutdown. Dropping a dialog variable does not cancel it.
Callbacks may collect, retain their result, open another chooser, dispose the
parent, or shut down. GTK's asynchronous request completes before the Cwhip
result handler runs, using a one-shot GUI idle dispatch; no nested main loop
or blocking wait is used.

```text
Gtk.FileDialog(Window parent, String initialPath, callback<void(list)> results)
```

This open-only constructor enables multiple selection. Acceptance invokes
`results` once with a list of every selected path or URI (in chooser order);
cancellation invokes it once with an empty list. A file `initialPath` starts in
that file's folder. Cancellation, disposal and shutdown rules are the same as
for the single-result constructor.

```text
Gtk.AlertDialog().show(Window parent, String message, String detail)
```

`show` presents a modal `GtkAlertDialog`, transient for `parent`, with a primary
message, secondary detail text and a single Close button. It returns
immediately and needs no callback; the binding keeps no reference to the alert.

The folder-selection overload is:

```text
Gtk.FileDialog(Window parent, String initialPath, callback<void(String)> folder)
```

It uses `gtk_file_dialog_select_folder`, with the same parent,
asynchronous delivery, accept/cancel, URI and lifetime contracts as the existing
file constructors. The overload preserves existing open/save/multiple APIs.
All constructors use cancellable GTK asynchronous requests. Programmatic
cancellation unlinks the pending token and releases the callback root immediately;
native request storage remains alive until GTK delivers its completion. A late
completion after cancellation, parent teardown or shutdown never invokes Cwhip
code. Folder selection is not treated as an open-file request.

## Trees and asynchronous directory enumeration

`Paned` is a horizontal, draggable two-child splitter. `append` attaches the
start child first, then the end child. A third child is an error.
`setPosition` accepts a nonnegative GTK integer pixel position, and `position()`
returns the current divider position. The start side
keeps its requested width on window resize; the end side grows.

```text
Gtk.Tree(callback<void(int)> request, callback<void(int)> activate)
void add(int parent, int id, String label, String icon, bool expandable,
         String value, int data)
void clear(int parent)
void setExpanded(int id, bool expanded)
String value(int id)
int data(int id)
int findValue(String value)
int childCount(int parent)
bool expanded(int id)
```

`Tree` is a virtualized `GtkListView`/`GtkTreeListModel` with `GtkTreeExpander`
rows and symbolic icon names. IDs must be positive and unique among live rows;
parent `0` denotes its roots. Values and integer data are generic, native-owned
row metadata, copied from the caller, not filesystem policy. `request(id)`
runs when an expandable row expands (possibly again on rebinding);
applications should guard already-started loads. Collapsed rows do not request
population merely to render their expansion arrow. `activate(id)` runs for
leaves on double-click/Enter; directory activation toggles expansion.
`clear(parent)` removes its descendants, invalidating their IDs.
`setExpanded` requires a visible, expandable row; expand its ancestors first.
`expanded(id)` reports whether a live row is currently expanded (false
when it is not visible). `findValue` returns the smallest matching live ID or `0`; `childCount` counts
immediate children, including any loading/error placeholders. All other row
methods require a live ID. Roots/callbacks are released on widget disposal and
shutdown, including disposal from within a callback.

```text
Gtk.DirectoryScan(Gtk.Tree owner, String path, bool hidden,
                  callback<void(int, String)> result)
Gtk.DirectoryScan(Gtk.Tree owner, String path, bool hidden, String root,
                  String excludes, callback<void(int, String)> result)
void cancel()
void append(Gtk.Tree owner, int parent, int firstId, int count, String icons)
```

Enumeration and sorting run on a native worker without managed-runtime or GTK
access. On the GUI owner thread, `result(count, "")` delivers a batch of up to
64 entries, at 5ms intervals. During that callback, `append` copies the current
batch into its owning Tree, assigning consecutive IDs starting at `firstId`.
The application chooses the parent, IDs and icons, without allocating a
managed object/path for every file. Ignoring a batch skips those rows.
`append` is valid only for that active batch, with a positive count no larger
than the delivered count; any remaining entries in that batch are skipped.

Rows have local UTF-8 paths as their `value` and kind as their integer `data`:
`0` (regular file), `1` (directory), `2` (symlink), or `3` (other).
Directories are expandable; other kinds are leaves. `icons` is a newline-separated
set of `key=icon-name` rules, with required defaults `0` through `3` and
optional filename suffixes beginning with `.`; the first matching suffix
overrides the regular-file default. These rules are supplied by the application,
not hardcoded editor policy in the native binding.

Dot-prefixed entries are omitted when `hidden=false`; directories sort first,
then UTF-8 names in byte order. Child symlinks are not followed. An explicitly
selected root may itself be a symlink. A final `result(-1, error)` reports
completion; empty error means success. Read failures or invalid-UTF-8 filenames
report a nonempty error and no batches.

The filtered constructor additionally omits entries matching `excludes`, a
newline-separated list of GLib `g_pattern_match_simple` globs (`*`, `?`).
A pattern without `/` is matched against the entry name; a pattern with `/`
is matched against the entry's path relative to `root` (which should be an
ancestor of `path`). Empty `excludes` filters nothing. Excluded directories are
never enumerated. Pattern validation is the application's responsibility.

Dropping the scan variable does not cancel it. Idempotent `cancel`, owner Tree
disposal and shutdown release the callback root without further results.
An active callback can cancel itself or shut down; registration release waits
until it unwinds. The worker retains only native memory and a cancellable,
so cancellation does not block the GUI waiting for filesystem I/O. Applications
remain responsible for generation checks and for their loading/error rows.

## Source editing

Configure `-DCWHIP_GTK_SOURCEVIEW=ON` together with GTK to enable `import sourceview`
and `GtkSource.View`. `widget()` supplies its `Gtk.SourceViewWidget` for normal
parenting and expansion. `setMonospace(bool)` selects GTK's text-view
monospaced-font mode without changing the font of other widgets. Besides text,
modified state, search/replace, language,
cursor and shortcut operations, the editor exposes:

```text
bool canUndo()
bool canRedo()
bool hasSelection()
void undo()
void redo()
void cut()
void copy()
void paste()
void selectAll()
void focus()
void setCursor(int line, int column)
```

`setCursor` places the insertion cursor at a 1-based line and character column
and scrolls it into view. Values are clamped to the buffer: lines below 1 or past
the end select the first/last line, and columns past the end of the line select
the line end. It matches the 1-based `line()`/`column()` getters.

Undo/redo state and selection query the actual text buffer. Clipboard operations
use GTK's display clipboard; paste is asynchronous and obeys text-view editability.
`setText` follows GTK's whole-buffer replacement semantics: it is irreversible
and clears prior undo/redo history. It is appropriate for loading/resetting a
document; typing, clipboard edits and `replaceNext` create undoable user edits.
Selection and edits emit the existing buffer/cursor signals, so menus can refresh
their action state. `onChanged` also forwards `can-undo`/`can-redo` property
notifications, which occur after GTK has updated its undo manager.
`onCursorMoved` forwards insertion/selection-bound mark changes and
`has-selection` notifications. Handlers may therefore run more than once for
one edit; query current state rather than counting emissions. Each returned
connection token disconnects the whole grouped subscription. These methods
share the normal GUI-thread lifetime rules.

`configure(String font, int size, int tabs, bool spaces, bool numbers, bool wrap,
bool highlight)` applies a per-view font family/point size, tab width,
space insertion, line numbers, word/character wrapping, and current-line
highlighting without replacing the buffer or language. It keeps monospace mode
enabled. Size must be 6-72 and tabs 1-16; validate user input before calling it.
`clearShortcuts()` removes all shortcuts installed through `bindShortcut`,
including their callback roots, without changing GTK's native editing bindings.
Removal/disposal during an active shortcut callback defers releasing that
callback until invocation unwinds.

Shortcut callbacks registered on a view may dispose that view, its window or
the application, or shut down (for example a Close or Quit command). A binding
released while its own callback is running is marked released and its receiver
root is dropped only after the callback returns; later activations are ignored.

Search and replace take an option sum of `GtkSource.View.CASE_SENSITIVE`,
`WHOLE_WORDS` and `IN_SELECTION` (0 is case-insensitive, substring, whole
buffer; `find`/`replaceNext` are the zero-option forms):

```text
bool findWith(String query, int options)
bool replaceWith(String query, String replacement, int options)
int replaceAll(String query, String replacement, int options)
bool captureSearchScope()
bool hasSearchScope()
String selectedText()
```

`findWith` selects the next match after the current selection, wrapping once.
`replaceWith` replaces the selection when it is a match, otherwise finds and
replaces the next one. `replaceAll` is a single undoable user action, never
rescans inserted text, and returns the replacement count. Whole words require
non-word characters (or buffer bounds) on both sides of a match. `IN_SELECTION`
limits the search to the scope recorded by `captureSearchScope()`: the current
selection is stored as text marks whose gravities keep the range covering
replaced text as the buffer changes. Capturing with no selection clears the
scope; a selection equal to the last match keeps the existing scope, so repeated
Find does not shrink it. With `IN_SELECTION` but no scope, searches fail.

### Language definitions

Views start as plain text: no syntax highlighting. Line numbers, auto-indent
and four-space indentation are independent of language.

```text
bool setLanguage(String id)          // "" selects plain text
String language()                    // current id, "" when plain
String GtkSource.Content().languageFor(String path)
bool GtkSource.Content().isText(String data)
```

The language protocol uses standard GtkSourceView 5 `.lang` files:

1. A language is registered by installing `<id>.lang` in the sourceview
   package's `language-specs` directory (staged or installed). The file name
   must equal the `id` attribute of its `<language>` element. Only ids found
   in those directories are accepted; `setLanguage` returns false for others
   and leaves the view unchanged.
2. File association comes from the definition's `globs` and `mimetypes`
   metadata, through GtkSourceView's language manager. `languageFor(path)`
   matches the file name only (no content sniffing) and returns `""` for
   untitled paths, unknown extensions, or unregistered languages.
3. `cwhip.lang` is the only shipped definition (`*.cw;*.cw;*.cw`). Supporting
   another language means adding its `.lang` file; no code changes are needed.

`isText` returns false for data containing NUL bytes. Cwhip strings are always
valid UTF-8, so `File.readAll` raises on undecodable input; applications should
treat that as a binary/non-text file.

## Preference support primitives

`Gtk.Font().isMonospace(String family)` accepts the generic `monospace` and
installed Pango monospace families, rejecting control characters and other
families. It does not select an application-wide font.

`Gtk.Keyboard().normalize(String trigger)` returns GTK's canonical spelling for
one key-value trigger, or `""` for invalid/unsupported syntax (including compound
alternative triggers). Use this result to compare conflicts.
`int record(Gtk.Entry entry, callback<void(String)> result)` captures the next
non-modifier key combination at the entry; Escape delivers `""`. The result
does not change the entry itself. `bool cancel(int token)` disconnects and
removes the recorder, and is safe inside its callback. It remains registered
until cancelled, its entry is destroyed, or application shutdown; callers must
cancel after receiving a result and coordinate which entry is recording.

`Gtk.Config` exposes the small TOML/persistence primitives used by Cwhip Editor's
Cwhip-written settings model, not a settings framework:

| Operation | Result/contract |
| --- | --- |
| `String validate(String text)` | `""` for valid TOML; otherwise a parse/scalar error. Encoded NUL strings are unsupported. |
| `String fileStatus(String path)` | `""` for an existing path, `missing` only for file-not-found, or an explicit stat/broken-symlink error. |
| `String value(String text, String section, String key)` | After validation: `""` for absent, `s` plus decoded string, `i` plus decimal integer, `btrue`/`bfalse`, `t` for a table, `x` for another type. Empty section addresses the root. |
| `String key(String text, String section, int index)` | Zero-based key enumeration; `""` at the end or for an absent table. |
| `String quote(String text)` | A TOML-escaped basic string literal. |
| `String item(String text, String section, String key, int index, String field)` | After validation: a tagged value (same tags as `value`) for zero-based element `index` of array `section.key`, or with nonempty `field`, that key of a table element (`""` if absent, `x` if the element is not a table). `""` past the end or for a missing array. |
| `String remove(String text, String section, String key)` | Validated TOML with one key removed (empty section addresses the root); other values are kept and serialized as by `merge`. |
| `String merge(String original, String updates)` | Both inputs must be validated TOML; recursively overlays updates and serializes, preserving unrelated values but not comments/formatting. |
| `void save(String path, String text, String expected, callback<void(String)> result)` | Asynchronously loads the local file, checks expected previous contents, then atomically replaces using its ETag where supported. Expected `""` allows a missing first-run file. Result is `""` on success, or an explicit I/O/concurrent-edit error. The caller creates the parent directory and bounds/validates the data. |

Save callback roots and copied bytes survive asynchronous I/O. Shutdown drains
pending operations without invoking their callbacks after teardown. These
operations, except pure TOML conversion, share GTK's GUI-owner rules. The TOML
parser is vendored MIT-licensed
[tomlc99](https://github.com/cktan/tomlc99/tree/29076dfd095bbbbd50a3c1b2760d29f4b83e74ac)
(`toml.c`/`toml.h`, with license headers retained); no runtime parser was
available in the repository. The compiler's package-manifest-specific parser
is not used as a general user-settings parser.

## Ownership, aliases, and disposal

The package sinks floating GObject references and owns an independent native
reference for every concrete wrapper; GTK's parent/application references remain
GTK's responsibility. Its private records and retained, non-invoked receiver
callbacks keep wrappers alive until disposal or shutdown. Dropping a Cwhip
variable does **not** dispose its native object. Detached widgets remain
application-owned resources until explicitly disposed or shut down.

`dispose()` is explicit, GUI-thread-only and idempotent. It disconnects the
source's signals, recursively disposes its **currently attached Cwhip children**,
detaches from its parent, destroys windows, and releases owned references.
`remove(child)` detaches without disposing: the child remains usable and can
be attached elsewhere. A child disposed directly is removed from its parent.
Closing/destroying a native application window also invalidates its wrapper and
recursively disposes attached children, even when an external GObject reference
keeps native storage alive. A prevented close leaves the entire tree live.

Assignments and base casts are aliases to the same wrapper/record, not independent
owners. Disposing through any alias invalidates all aliases. There is no public
API for manufacturing another owner from a raw native pointer. All operations
after disposal fail clearly, except repeat `dispose()` and the GUI-only
`disposed()` query. After application shutdown, every widget reports disposed.
Disposed records are tombstones until shutdown; private integer identities are
never reused and are never dereferenced as native pointers.

The native ownership contract allows only one `Widget` base per managed object.
Multiple widget base subobjects are rejected during construction, so signal
registration cannot silently select another widget base. The shipped hierarchy
does not provide a general custom/native-widget subclass framework.

No widget, connection, or application GC finalizer performs GTK teardown.
Use `dispose()`, not Cwhip's language-level `destroy()`, for GUI lifetime.
Custom subclass finalizers must not call GTK: GC can run on a worker, and native
teardown is exclusively a GUI-thread operation.

## Typed signals

`connection.disconnect()` returns true on the first live disconnection and false
thereafter. `connection.connected()` reports current state; both are GUI-only
and remain usable after source disposal or application shutdown.
Ignoring/dropping the returned connection does not disconnect the handler.
Typed `SignalConnection(source, action)` constructors are also available for
`Application`, `Button`, `Entry`, `Window`, and `CheckButton`, with the same
callback signatures as their registration methods. They expose no native
handle or connection-token constructor.

Handlers receive Cwhip values, never raw GTK instance pointers:
`Button` invokes `callback<void()>`; `Entry` invokes `callback<void(String)>`
with a copied, rooted managed string; `CheckButton` invokes
`callback<void(bool)>` with its current active state. A
`Window` close-request handler returns true to prevent closing and false to
allow GTK's default close action. GTK's native signal ordering/accumulators
still apply when multiple handlers are connected.

Connections retain callback receivers until disconnection/source disposal/
shutdown and the last in-flight invocation **and** GLib closure user have
finished. Disconnecting or disposing inside a handler, including nested
emissions, is supported. A disconnected handler does not receive later emissions.
The native text pointer is copied before entering Cwhip; callbacks can allocate,
collect, retain the string, and mutate/dispose the source without invalidating
the payload. GTK may coalesce or suppress recursive editable changes; no extra
synthetic notifications are promised.

## Runnable sample

This headless-tested program builds its hierarchy during activation and uses a
typed changed signal to update a label. The posted finish action makes the
documentation program terminate automatically. For an interactive editor with
a checkbox, scrolled content, and a close button, see `examples/gtk.cw`
(run without `--test`).

```cwhip
// test: {"stdout": "Hello GTK", "requires": "gtk"}
import gtk

class Demo {
    Gtk.Window window
    Gtk.Entry entry
    Gtk.Label label
    callback<void(String)> changedAction
    callback<void()> finishAction

    void activate() {
        window = Gtk.Window("Cwhip")
        Gtk.Box layout = Gtk.Box(Gtk.Box.VERTICAL, 8)
        window.setChild(layout)
        entry = Gtk.Entry("")
        label = Gtk.Label("")
        layout.append(entry)
        layout.append(label)
        entry.onChanged(changedAction)
        window.present()
        Gtk.Application().post(finishAction)
    }
    void changed(String text) { label.setText(text) }
    void finish() {
        entry.setText("Hello GTK")
        print(label.text())
        window.close()
    }
}

start {
    Gtk.Application app = Gtk.Application("org.cwhip.Demo")
    Demo demo = Demo()
    demo.changedAction = demo.changed
    demo.finishAction = demo.finish
    app.onActivate(demo.activate)
    app.run()
    app.shutdown()
}
```

## Threading and native boundaries

All creation, properties, parenting, signals, queries and disposal require the
designated registered GUI thread and its managed lock. Worker threads cannot
call widget methods, disconnect signals or query GUI state directly; use `post`.
Callbacks use the existing `callback<R(P...)>` bridge and exact context-first
adapters. Package-native forwarding, callback transfers, and thread/lock checks
are private implementation machinery, not public raw-pointer APIs.

`run()` releases the managed lock across `g_application_run()` so workers progress
while GTK waits. Every native-to-Cwhip signal acquires the lock with
`cwhip_runtime_managed_enter()` and restores it with `managed_leave(acquired)`.
Synchronous/nested reentry preserves lock ownership; foreign threads are never
registered implicitly. One-shot worker posts prepare a rooted, non-invocable
`CwhipCallbackTransfer`; GUI dispatch accepts it into a fresh GUI-owned context.
Cancellation consumes the transfer without invocation.

Native contract violations print a diagnostic and abort; null managed receiver
access follows the existing Cwhip runtime diagnostic. Exceptions escaping any
native callback abort **inside** the bridge, never unwind through GTK or fabricate
a return value. Catch recoverable exceptions inside handlers. Bridge ownership
checks are unchanged.

The suite uses real GTK/Xvfb and isolated session buses, compiled and installed
consumers, GC/reentry/misuse cases, optional documentation/examples, and ASan/UBSan.
`tests/test_gtk_bindings.py` additionally drives real native chooser responses,
overwrite acceptance/decline, callback GC and cancellation/teardown, menu actions,
and actual multiline source-widget allocations before and after window resize.
Its private fixture is not shipped in the package.
See [TESTING.md](TESTING.md). Styling, broad dialog/model coverage, drawing, input events,
arbitrary native-widget adoption, file-open/command-line activation, and broad GTK
property coverage are not implemented in this release.
