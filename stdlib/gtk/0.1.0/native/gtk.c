#include "gtk_private.h"
#include <simp/Callbacks.h>
#include <simp/RuntimeGc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Connection {
    struct Connection *next;
    int64_t token;
    GObject *object;
    gulong signal;
    SimpCallbackContext *context;
    unsigned active;
    bool closed;
    bool detached;
} Connection;

typedef struct Post {
    struct Post *next;
    int64_t token;
    GSource *source;
    SimpCallbackTransfer *transfer;
} Post;

/* Lists and lifecycle state are protected by the runtime lock. Only the
 * immutable GUI identity is read at an unlocked signal boundary. */
static pthread_t owner;
static bool initialized;
static bool stopped;
static bool quitting;
static unsigned running;
static int64_t next_token;
static GMainContext *main_context;
static Connection *connections;
static Post *posts;

static _Noreturn void fatal(const char *message) {
    fprintf(stderr, "Simple GTK error: %s\n", message);
    abort();
}

static void require_managed(void) {
    if (simp_runtime_managed_enter()) fatal("operation requires the managed runtime lock");
}

void simp_gtk_require_owner(void) {
    if (!initialized || !pthread_equal(owner, pthread_self()))
        fatal("operation requires the GUI owner thread");
}

static void require_live(void) {
    require_managed();
    simp_gtk_require_owner();
    if (stopped) fatal("application has shut down");
}

static int64_t token_new(void) {
    if (next_token == INT64_MAX) fatal("token space exhausted");
    return ++next_token;
}

static void connection_collect(Connection *connection) {
    if (!connection->detached || connection->active) return;
    Connection **link = &connections;
    while (*link != connection) link = &(*link)->next;
    *link = connection->next;
    simp_callback_release(connection->context);
    simp_callback_dispose(connection->context);
    free(connection);
}

static void connection_destroy(gpointer data, GClosure *closure) {
    (void)closure;
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    Connection *connection = data;
    connection->object = NULL;
    connection->closed = true;
    connection->detached = true;
    connection_collect(connection);
    simp_runtime_managed_leave(acquired);
}

static int signal_enter(Connection *connection) {
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    ++connection->active;
    return acquired;
}

static void signal_leave(Connection *connection, int acquired) {
    --connection->active;
    connection_collect(connection);
    simp_runtime_managed_leave(acquired);
}

static void clicked(GtkButton *button, gpointer data) {
    (void)button;
    Connection *connection = data;
    int acquired = signal_enter(connection);
    typedef void (*Adapter)(SimpCallbackContext *);
    ((Adapter)simp_callback_adapter(connection->context))(connection->context);
    signal_leave(connection, acquired);
}

static void changed(GtkEditable *editable, gpointer data) {
    (void)editable;
    Connection *connection = data;
    int acquired = signal_enter(connection);
    typedef void (*Adapter)(SimpCallbackContext *);
    ((Adapter)simp_callback_adapter(connection->context))(connection->context);
    signal_leave(connection, acquired);
}

static gboolean close_request(GtkWindow *window, gpointer data) {
    (void)window;
    Connection *connection = data;
    int acquired = signal_enter(connection);
    typedef bool (*Adapter)(SimpCallbackContext *);
    bool result = ((Adapter)simp_callback_adapter(connection->context))(connection->context);
    signal_leave(connection, acquired);
    return result ? TRUE : FALSE;
}

static int64_t connect_signal(GObject *object, const char *signal,
                               GCallback forwarder, void *callback, const char *signature) {
    require_live();
    if (!object) fatal("null signal object");
    Connection *connection = calloc(1, sizeof(*connection));
    if (!connection) fatal("allocation failed");
    connection->token = token_new();
    connection->object = object;
    connection->context = simp_callback_acquire(callback, signature);
    connection->next = connections;
    connections = connection;
    connection->signal = g_signal_connect_data(object, signal, forwarder, connection,
                                               connection_destroy, 0);
    if (!connection->signal) fatal("signal connection failed");
    return connection->token;
}

int64_t simp_gtk_connect_clicked(GtkButton *button, void *callback) {
    require_live();
    if (!GTK_IS_BUTTON(button)) fatal("clicked requires GtkButton");
    return connect_signal(G_OBJECT(button), "clicked", G_CALLBACK(clicked),
                          callback, "callback<void()>");
}

int64_t simp_gtk_connect_changed(GtkEditable *editable, void *callback) {
    require_live();
    if (!GTK_IS_EDITABLE(editable)) fatal("changed requires GtkEditable");
    return connect_signal(G_OBJECT(editable), "changed", G_CALLBACK(changed),
                          callback, "callback<void()>");
}

int64_t simp_gtk_connect_close_request(GtkWindow *window, void *callback) {
    require_live();
    if (!GTK_IS_WINDOW(window)) fatal("close-request requires GtkWindow");
    return connect_signal(G_OBJECT(window), "close-request", G_CALLBACK(close_request),
                          callback, "callback<bool()>");
}

bool simp_gtk_disconnect(int64_t token) {
    require_managed();
    simp_gtk_require_owner();
    for (Connection *connection = connections; connection; connection = connection->next) {
        if (connection->token != token) continue;
        if (connection->closed) return false;
        /* GClosure defers its notify until emission unwinds. The active
         * count also covers synchronous/nested callbacks explicitly. */
        connection->closed = true;
        g_signal_handler_disconnect(connection->object, connection->signal);
        return true;
    }
    return false;
}

void simp_gtk_initialize(void *self) {
    (void)self;
    require_managed();
    if (initialized) fatal("application initialization is single-use");
    if (!gtk_init_check()) fatal("GTK initialization failed (display unavailable)");
    owner = pthread_self();
    initialized = true;
    main_context = g_main_context_ref(g_main_context_default());
}

void simp_gtk_run(void *self) {
    (void)self;
    require_live();
    if (running) fatal("nested application run is not supported");
    quitting = false;
    ++running;
    while (!quitting && !stopped) {
        simp_runtime_gil_release();
        g_main_context_iteration(main_context, TRUE);
        simp_runtime_gil_acquire();
    }
    --running;
}

void simp_gtk_quit(void *self) {
    (void)self;
    require_live();
    quitting = true;
    g_main_context_wakeup(main_context);
}

static void post_remove(Post *post) {
    Post **link = &posts;
    while (*link != post) link = &(*link)->next;
    *link = post->next;
}

static gboolean dispatch(gpointer data) {
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    Post *post = data;
    post_remove(post);
    SimpCallbackContext *context = simp_callback_transfer_accept(post->transfer);
    g_source_unref(post->source);
    free(post);
    typedef void (*Adapter)(SimpCallbackContext *);
    ((Adapter)simp_callback_adapter(context))(context);
    simp_callback_release(context);
    simp_callback_dispose(context);
    simp_runtime_managed_leave(acquired);
    return G_SOURCE_REMOVE;
}

int64_t simp_gtk_post(void *self, void *callback) {
    (void)self;
    require_managed();
    if (!initialized || stopped) fatal("post requires a live application");
    Post *post = calloc(1, sizeof(*post));
    if (!post) fatal("allocation failed");
    post->token = token_new();
    post->transfer = simp_callback_transfer_prepare(callback, "callback<void()>");
    post->source = g_idle_source_new();
    post->next = posts;
    posts = post;
    g_source_set_callback(post->source, dispatch, post, NULL);
    if (!g_source_attach(post->source, main_context)) fatal("post attachment failed");
    return post->token;
}

static void post_cancel(Post *post) {
    post_remove(post);
    g_source_destroy(post->source);
    g_source_unref(post->source);
    simp_callback_transfer_cancel(post->transfer);
    free(post);
}

bool simp_gtk_cancel(void *self, int64_t token) {
    (void)self;
    require_live();
    for (Post *post = posts; post; post = post->next) {
        if (post->token == token) {
            post_cancel(post);
            return true;
        }
    }
    return false;
}

void simp_gtk_shutdown(void *self) {
    (void)self;
    require_managed();
    simp_gtk_require_owner();
    if (stopped) return;
    stopped = true;
    quitting = true;
    while (posts) post_cancel(posts);
    for (Connection *connection = connections; connection;) {
        Connection *next = connection->next;
        if (!connection->closed) simp_gtk_disconnect(connection->token);
        connection = next;
    }
    g_main_context_wakeup(main_context);
    g_main_context_unref(main_context);
    /* The default context itself outlives this application and a currently
     * unwinding iteration. All package-owned sources are already destroyed. */
    main_context = NULL;
}
