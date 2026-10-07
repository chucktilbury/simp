# GTK 4 integration foundation

The optional `gtk` package, version `0.1.0`, implements lifecycle, GUI scheduling,
and package-private typed signal forwarding. It is **not** the main window/widget
interface. `import gtk` exposes namespace `Gtk`; adapters and lock management
are hidden from Simple applications.

## Build and package linkage

Configure with `-DSIMP_GTK=ON`. CMake requires `pkg-config`, GTK 4 development
files (`gtk4.pc`), and Xvfb for the configured integration test. With this
option off (the default), the GTK package is absent from the staged/installed
standard modules. With it on, `lib/libsimp_gtk.a` lives inside
`share/simp/modules/gtk/0.1.0/`, beside its source and manifest.

The package's `[link]` metadata links importing applications with `simp_gtk`,
`gtk-4`, `gobject-2.0`, and `glib-2.0`. Neither the compiler executable nor a
non-import application acquires a GTK dependency. Generated compile-only
objects use the existing package link sidecar mechanism. Consumers need GTK 4
runtime libraries and a usable GTK display; installation does not bundle GTK.
The current native target remains POSIX, not a new cross-platform GUI layer.

## Minimal Simple API

`Gtk.Application` is a stateless facade over one process-wide, single-use
application lifecycle:

```text
void initialize()
void run()
void quit()
void shutdown()
int post(callback<void()> action)
bool cancel(int token)
```

Call `initialize()` on the registered Simple thread that will own GTK, then
`run()` to wait for events. `quit()` exits the current run without shutting
down; a subsequent `run()` is allowed. `shutdown()` is terminal and idempotent.
It quits the current run, cancels pending package work, and disconnects all
package signal registrations. It is safe inside a handler. Reinitialization
and nested `run()` are deliberately unsupported.

Only `post()` may be called by another registered Simple worker. It returns a
positive, process-unique token, never a native callback context. Posts execute
once on the GUI thread using an idle source on GLib's default main context.
`cancel(token)` is GUI-only: true means pending work was removed, false means
the token is no longer pending (including already running/completed work).
Tokens do not expose freed native memory and are never reused.

Initialization failures, null or signature-mismatched callbacks, operations
on the wrong thread, posts before initialization/after shutdown, and other
native-contract violations print an explicit diagnostic and abort. No work is
silently dropped or executed on the submitting worker. Applications should
coordinate worker shutdown before calling `shutdown()`; workers still trying
to post afterward fail explicitly. Native allocation failure is also fatal.

## Thread, lock, and rooting rules

GTK creation, mutation, signals, and teardown must stay on the designated GUI
owner thread, including native reference releases. The package owns iteration
of the default GLib context; do not iterate it from a worker or install external
sources that directly run Simple code without a supported boundary.

`run()` releases the managed-code lock around each potentially blocking
`g_main_context_iteration()` and reacquires it afterward. Simple stack roots
remain registered and frozen while suspended, so workers can execute and
collect while the loop is idle. Signal dispatch and closure destruction use
the supported `<simp/RuntimeGc.h>` `simp_runtime_managed_enter()` /
`simp_runtime_managed_leave(acquired)` pair. It acquires the lock on an already
registered, suspended GUI thread, but does not recursively lock synchronous
reentry from Simple. Nested emissions preserve the prior lock state. It does
not register foreign threads or permit foreign-owner callback invocation.

Posting prepares a rooted, **non-invocable** `SimpCallbackTransfer` under the
submitting worker's managed lock. Dispatch accepts it on the GUI thread,
atomically replacing its root with a fresh GUI-owned callback registration,
invokes that registration, then releases/disposes it. Cancellation consumes
the transfer without invocation. The package lists are protected by the same
managed lock; GLib sources only access them after acquiring it. Receiver and
callback therefore survive GC even after the submitting worker exits.

All Simple handlers use the existing bridge exception policy: an escaping
exception is reported and aborts the process **inside** the bridge. It never
unwinds across GTK/GLib frames and never fabricates a native return value.

## Package-private signal and object ownership

`native/gtk_private.h` is an implementation contract for future widget
wrappers, not an installed `<simp/...>` API. There are three static,
signature-correct forwarding functions:

| GTK signal | Native forwarding ABI | Simple callback |
| --- | --- | --- |
| `GtkButton::clicked` | `void(GtkButton *, gpointer)` | `callback<void()>` |
| `GtkEditable::changed` | `void(GtkEditable *, gpointer)` | `callback<void()>` |
| `GtkWindow::close-request` | `gboolean(GtkWindow *, gpointer)` | `callback<bool()>` |

Each forwarder obtains only the exact generated context-first adapter.
GTK's emitted instance argument is not exposed yet; a bound receiver owns
the application's state. `close-request` converts C `_Bool` to GTK
`gboolean`; true suppresses the default close action, false allows it.
No disparate signal signatures are coerced through a generic forwarding cast,
and no private Simple method symbols are called.

Connections have positive integer tokens and root callbacks until disconnected
and all active invocations/GLib closure users finish. Disconnect during a
handler or nested emission defers release/disposal until both the invocation
count is zero and GLib's closure destroy notification has run. Finalizing the
native object releases its signal registrations by the same path. Repeated
disconnect of an expired token returns false.

Connections borrow GTK objects: **GC callback roots are not GObject refs**.
Future widget wrappers must explicitly own/sink/ref/unref their objects and
perform disposal on the GUI thread. `gtk_window_destroy()` is not necessarily
finalization when an external reference remains; either disconnect explicitly
or release all owned refs. Shutdown disconnects callbacks but does not destroy
application-owned widgets. GC finalizers must not perform GTK teardown:
collection can happen on a worker and existing GC finalizers cannot allocate
or block. Explicit GUI-side close/disposal will be required.

## Evidence and scope before the widget interface

`simp_gtk` compiles Simple against the staged native archive, exercises real
button/entry/window signals under a fresh Xvfb display, then installs the prefix
and runs an installed consumer. It checks reentry, GC inside handlers,
disconnect/destruction during handlers, boolean close responses, cancellation,
shutdown while dispatching, rooted worker transfers, and worker progress while
the loop is idle. Misuse subprocesses must abort with the intended diagnostic,
not sanitizer output. It also checks the compiler and a non-import executable
with `ldd` for absence of GTK linkage. See [TESTING.md](TESTING.md).

Before implementing the main GTK interface, define the public widget hierarchy
and explicit GUI-side reference/disposal contract; decide whether the eventual
lifecycle should use `GtkApplication`/application IDs and desktop activation;
and specify callback parameters for additional signal families. The current
`Application` is a scheduler, not a `GtkApplication` desktop object. Strings,
properties, layout, events carrying native data, application/window ownership,
and an expanded widget API are intentionally deferred. Package test fixtures
create widgets only for verification and are not a public widget API.
