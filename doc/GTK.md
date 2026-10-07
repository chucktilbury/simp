# GTK 4 interface

The optional `gtk` package (`0.1.0`, `import gtk`, namespace `Gtk`) provides a
compact GTK-aligned interface: `Application`, `Widget`, `Window`, `Box`, `Label`,
`Button`, `Entry`, `CheckButton`, `ScrolledWindow`, `Notebook`, `MenuBar`,
`FileDialog`, and `SignalConnection`, plus the optional `sourceview` package.
It is not a complete GTK binding or a generic GUI framework.

## Build and package linkage

Fresh configurations enable `SIMP_GTK` when GTK 4 development files and
`pkg-config` are available, and enable `SIMP_GTK_SOURCEVIEW` when GtkSourceView 5
is also available. Explicit `ON` options require the corresponding dependencies.
With `BUILD_TESTING=ON`, headless tests default to enabled when Xvfb and
`dbus-daemon` are found; `SIMP_GTK_TESTS=ON` requires them explicitly.
With `SIMP_GTK=OFF` and `SIMP_GTK_SOURCEVIEW=OFF`, the GTK package is absent from staged/installed
standard modules. With it on, `lib/libsimp_gtk.a` resides inside
`share/simp/modules/gtk/0.1.0/`, beside the source and manifest.

Package `[link]` metadata links importing applications with `simp_gtk`, `gtk-4`,
`gio-2.0`, `gobject-2.0`, and `glib-2.0`. The compiler and non-import applications
remain unlinked to GTK. Compile-only objects retain package link sidecars.
Installed consumers use the normal `simpkg init` / `simpkg install` lock flow.
GTK runtime libraries and a usable display are external requirements; they
are not bundled. The native platform remains POSIX.

## Application and activation

Construct exactly one `Application(String id)` on the registered Simple thread
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
call does not consume Simple's program arguments.

Closing the last application window normally ends the run. `quit()` requests
exit and requires an active run; it does not pretend that closing was approved.
`run()` is single-use and **terminal**: on return it performs `shutdown()`.
`shutdown()` is GUI-only, terminal and idempotent, and is safe inside a handler
or before running. It cancels pending posts, disconnects every signal, disposes
all widgets (including detached widgets), and releases the native application.
Native application release is kept safe while a run or emission is unwinding.
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
`org.simple.Scheduler` and an explicit hold until `quit`/`shutdown`, so
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
| `Window` | `Window(String title)`, `setTitle(String)`, `String title()`, `setDefaultSize(int width, int height)`, `setTransientFor(Window parent)`, `setChild(Widget)`, `remove(Widget)`, `present()`, `close()`, `onCloseRequest(callback<bool()>)` |
| `TransientWindow` | `TransientWindow(String title)`, `setTitle(String)`, `String title()`, `setDefaultSize(int width, int height)`, `setTransientFor(Window parent)`, `setChild(Widget)`, `remove(Widget)`, `present()`, `close()`, `onCloseRequest(callback<bool()>)` |
| `Box` | `Box(int orientation, int spacing)`, `setLayout(int orientation, int spacing)`, `append(Widget)`, `remove(Widget)` |
| `Label` | `Label(String text)`, `setText(String)`, `String text()` |
| `Button` | `Button(String label)`, `setLabel(String)`, `String label()`, `onClicked(callback<void()>)` |
| `Entry` | `Entry(String text)`, `setText(String)`, `String text()`, `focus()`, `onChanged(callback<void(String)>)` |
| `CheckButton` | `CheckButton(String label)`, `setLabel(String)`, `String label()`, `setActive(bool)`, `bool active()`, `onToggled(callback<void(bool)>)` |
| `ScrolledWindow` | `ScrolledWindow()`, `setChild(Widget)`, `remove(Widget)` |
| `Notebook` | `Notebook()`, `appendPage(Widget, String)`, `int currentPage()`, `setCurrentPage(int)`, `int pageCount()`, `setPageTitle(int, String)`, `remove(Widget)`, `onPageChanged(callback<void()>)` |
| `MenuBar` | `MenuBar()`, `int addMenu(String)`, `int addItem(int menu, String label, callback<void()>)`, `setItemEnabled(int item, bool)`, `activateItem(int item)` |

Methods without a listed result return `void`, except signal registration,
which returns `SignalConnection`. Orientation is `Gtk.Box.HORIZONTAL` (`0`) or
`Gtk.Box.VERTICAL` (`1`); spacing is nonnegative and must fit GTK's integer range.
Default window dimensions must be positive and fit that range.
Text accepts managed UTF-8 strings, including empty strings, but not null,
embedded NULs, or invalid UTF-8. Getters return managed copies.

`Window`, `TransientWindow` and `ScrolledWindow` have one child; remove it before
attaching a replacement. `Window` is associated with the `Gtk.Application`;
`TransientWindow` is a standalone top-level. `setTransientFor` establishes the
native transient relationship, not managed ownership, so dispose transient
windows explicitly. `Box` accepts multiple children. A child must be live and unparented;
windows cannot be children. Self-parenting, cycles, already-parented children,
occupied single-child containers, and removing from the wrong parent are errors.
GTK may internally insert a viewport in a scrolled window; the Simple ownership
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

Construction shows a modal, parented `GtkFileChooserNative` asynchronously:
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
parent, or shut down. Native response cleanup completes before the Simple
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

## Source editing

Configure `-DSIMP_GTK_SOURCEVIEW=ON` together with GTK to enable `import sourceview`
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
```

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
3. `tweed.lang` is the only shipped definition (`*.simp;*.tweed`). Supporting
   another language means adding its `.lang` file; no code changes are needed.

`isText` returns false for data containing NUL bytes. Simple strings are always
valid UTF-8, so `File.readAll` raises on undecodable input; applications should
treat that as a binary/non-text file.

## Ownership, aliases, and disposal

The package sinks floating GObject references and owns an independent native
reference for every concrete wrapper; GTK's parent/application references remain
GTK's responsibility. Its private records and retained, non-invoked receiver
callbacks keep wrappers alive until disposal or shutdown. Dropping a Simple
variable does **not** dispose its native object. Detached widgets remain
application-owned resources until explicitly disposed or shut down.

`dispose()` is explicit, GUI-thread-only and idempotent. It disconnects the
source's signals, recursively disposes its **currently attached Simple children**,
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
Use `dispose()`, not Simple's language-level `destroy()`, for GUI lifetime.
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

Handlers receive Simple values, never raw GTK instance pointers:
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
The native text pointer is copied before entering Simple; callbacks can allocate,
collect, retain the string, and mutate/dispose the source without invalidating
the payload. GTK may coalesce or suppress recursive editable changes; no extra
synthetic notifications are promised.

## Runnable sample

This headless-tested program builds its hierarchy during activation and uses a
typed changed signal to update a label. The posted finish action makes the
documentation program terminate automatically. For an interactive editor with
a checkbox, scrolled content, and a close button, see `examples/gtk.simp`
(run without `--test`).

```simp
// test: {"stdout": "Hello GTK", "requires": "gtk"}
import gtk

class Demo {
    Gtk.Window window
    Gtk.Entry entry
    Gtk.Label label
    callback<void(String)> changedAction
    callback<void()> finishAction

    void activate() {
        window = Gtk.Window("Simple")
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
    Gtk.Application app = Gtk.Application("org.simple.Demo")
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
while GTK waits. Every native-to-Simple signal acquires the lock with
`simp_runtime_managed_enter()` and restores it with `managed_leave(acquired)`.
Synchronous/nested reentry preserves lock ownership; foreign threads are never
registered implicitly. One-shot worker posts prepare a rooted, non-invocable
`SimpCallbackTransfer`; GUI dispatch accepts it into a fresh GUI-owned context.
Cancellation consumes the transfer without invocation.

Native contract violations print a diagnostic and abort; null managed receiver
access follows the existing Simple runtime diagnostic. Exceptions escaping any
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
