#include "gtk_private.h"
#include <simp/Callbacks.h>
#include <simp/RuntimeGc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const SimpClassMeta simp_string_class_meta;

typedef struct WidgetRecord {
    struct WidgetRecord *next;
    int64_t token;
    GtkWidget *widget;
    struct WidgetRecord *parent;
    void *receiver;
    SimpCallbackContext *root;
    bool native_destroying;
} WidgetRecord;

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
static unsigned running;
static int64_t next_token;
static GMainContext *main_context;
static Connection *connections;
static Post *posts;
static WidgetRecord *widgets;
static GtkApplication *application;
static bool activated;
static bool ran;
static bool scheduler_hold;
static void widget_dispose(WidgetRecord *record);
static void window_removed(GtkApplication *app, GtkWindow *window, gpointer data);

static _Noreturn void fatal(const char *message) {
    fprintf(stderr, "Simple GTK error: %s\n", message);
    abort();
}

/* Private typed receiver bindings compensate for Simple's lack of a this expression. */
void *simp_gtk_self(void *self) { return self; }

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

static void text_changed(GtkEditable *editable, gpointer data) {
    Connection *connection = data;
    int acquired = signal_enter(connection);
    const char *bytes = gtk_editable_get_text(editable);
    void *text = simp_string_new(&simp_string_class_meta, bytes, strlen(bytes));
    void *slots[] = { &text };
    SimpRootFrame frame = {0};
    simp_gc_push_or_abort(&frame, slots, 1);
    typedef void (*Adapter)(SimpCallbackContext *, void *);
    ((Adapter)simp_callback_adapter(connection->context))(connection->context, text);
    simp_gc_pop_or_abort(&frame);
    signal_leave(connection, acquired);
}

static void toggled(GtkCheckButton *button, gpointer data) {
    Connection *connection = data;
    int acquired = signal_enter(connection);
    typedef void (*Adapter)(SimpCallbackContext *, bool);
    ((Adapter)simp_callback_adapter(connection->context))(
        connection->context, gtk_check_button_get_active(button) != FALSE);
    signal_leave(connection, acquired);
}

static void activate(GApplication *app, gpointer data) {
    (void)app;
    clicked(NULL, data);
}

static void activation_started(GApplication *app, gpointer data) {
    (void)app;
    (void)data;
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    activated = true;
    simp_runtime_managed_leave(acquired);
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

static void application_initialize(const char *id, bool scheduler) {
    require_managed();
    if (initialized) fatal("application initialization is single-use");
    if (!gtk_init_check()) fatal("GTK initialization failed (display unavailable)");
    owner = pthread_self();
    initialized = true;
    main_context = g_main_context_ref(g_main_context_default());
    application = gtk_application_new(id, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(application, "activate", G_CALLBACK(activation_started), NULL);
    g_signal_connect(application, "window-removed", G_CALLBACK(window_removed), NULL);
    if (scheduler) g_application_hold(G_APPLICATION(application));
    scheduler_hold = scheduler;
}

void simp_gtk_initialize(void *self) {
    (void)self;
    application_initialize("org.simple.Scheduler", true);
}

void simp_gtk_run(void *self) {
    (void)self;
    require_live();
    if (running) fatal("nested application run is not supported");
    if (ran) fatal("application run is single-use");
    ran = true;
    ++running;
    GtkApplication *run_application = g_object_ref(application);
    simp_runtime_gil_release();
    int status = g_application_run(G_APPLICATION(run_application), 0, NULL);
    simp_runtime_gil_acquire();
    --running;
    g_object_unref(run_application);
    if (status != 0) fatal("GtkApplication run failed");
    simp_gtk_shutdown(self);
}

void simp_gtk_quit(void *self) {
    (void)self;
    require_live();
    if (!running) fatal("quit requires a running application");
    g_application_quit(G_APPLICATION(application));
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
    g_application_quit(G_APPLICATION(application));
    while (posts) post_cancel(posts);
    for (Connection *connection = connections; connection;) {
        Connection *next = connection->next;
        if (!connection->closed) simp_gtk_disconnect(connection->token);
        connection = next;
    }
    for (WidgetRecord *record = widgets; record; record = record->next)
        widget_dispose(record);
    while (widgets) {
        WidgetRecord *next = widgets->next;
        free(widgets);
        widgets = next;
    }
    if (scheduler_hold) {
        g_application_release(G_APPLICATION(application));
        scheduler_hold = false;
    }
    g_object_unref(application);
    application = NULL;
    g_main_context_wakeup(main_context);
    g_main_context_unref(main_context);
    /* The default context itself outlives this application and a currently
     * unwinding iteration. All package-owned sources are already destroyed. */
    main_context = NULL;
}

static char *text_copy(void *text) {
    if (!text) fatal("null text");
    const char *bytes;
    uint64_t length;
    simp_string_bytes(text, &bytes, &length);
    if (!length) bytes = "";
    if (length > G_MAXSSIZE || memchr(bytes, 0, length) ||
        !g_utf8_validate(bytes, (gssize)length, NULL))
        fatal("text must be UTF-8 without embedded NUL");
    return g_strndup(bytes, length);
}

void simp_gtk_application_create(void *self, void *id) {
    (void)self;
    require_managed();
    if (initialized) fatal("application initialization is single-use");
    char *name = text_copy(id);
    if (!g_application_id_is_valid(name)) fatal("invalid application ID");
    application_initialize(name, false);
    g_free(name);
}

int64_t simp_gtk_application_activate(void *self, void *callback) {
    (void)self;
    require_live();
    if (ran) fatal("activation handler must be installed before run");
    return connect_signal(G_OBJECT(application), "activate", G_CALLBACK(activate),
                          callback, "callback<void()>");
}

int64_t simp_gtk_activation_signal(void *self, void *source, void *callback) {
    if (!source) fatal("null signal source");
    return simp_gtk_application_activate(self, callback);
}

void *simp_gtk_application_id(void *self) {
    (void)self;
    require_live();
    const char *id = g_application_get_application_id(G_APPLICATION(application));
    return simp_string_new(&simp_string_class_meta, id, strlen(id));
}

bool simp_gtk_connected(void *self, int64_t token) {
    (void)self;
    require_managed();
    simp_gtk_require_owner();
    for (Connection *c = connections; c; c = c->next)
        if (c->token == token) return !c->closed;
    return false;
}

bool simp_gtk_connection_disconnect(void *self, int64_t token) {
    (void)self;
    return simp_gtk_disconnect(token);
}

static WidgetRecord *record_find(int64_t token) {
    require_managed();
    simp_gtk_require_owner();
    for (WidgetRecord *r = widgets; r; r = r->next)
        if (r->token == token) return r;
    fatal("invalid or disposed widget");
}

static GtkWidget *widget_live(int64_t token) {
    require_live();
    GtkWidget *widget = record_find(token)->widget;
    if (!widget) fatal("widget has been disposed");
    return widget;
}

static void disconnect_object(GObject *object) {
    for (Connection *c = connections; c;) {
        Connection *next = c->next;
        if (c->object == object && !c->closed) simp_gtk_disconnect(c->token);
        c = next;
    }
}

static void detach(WidgetRecord *r) {
    if (!r->parent) return;
    GtkWidget *parent = r->parent->widget;
    r->parent = NULL;
    if (GTK_IS_BOX(parent)) gtk_box_remove(GTK_BOX(parent), r->widget);
    else if (GTK_IS_WINDOW(parent)) gtk_window_set_child(GTK_WINDOW(parent), NULL);
    else if (GTK_IS_SCROLLED_WINDOW(parent))
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(parent), NULL);
    else fatal("invalid widget parent");
}

static void widget_dispose(WidgetRecord *r) {
    if (!r->widget) return;
    GtkWidget *widget = r->widget;
    disconnect_object(G_OBJECT(widget));
    for (WidgetRecord *child = widgets; child; child = child->next)
        if (child->parent == r) widget_dispose(child);
    detach(r);
    r->widget = NULL;
    g_signal_handlers_disconnect_by_data(widget, r);
    if (GTK_IS_WINDOW(widget) && !r->native_destroying)
        gtk_window_destroy(GTK_WINDOW(widget));
    g_object_unref(widget);
    simp_callback_release(r->root);
    simp_callback_dispose(r->root);
    r->root = NULL;
    r->receiver = NULL;
}

static void widget_destroyed(GtkWidget *widget, gpointer data) {
    (void)widget;
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    WidgetRecord *r = data;
    r->native_destroying = true;
    widget_dispose(r);
    simp_runtime_managed_leave(acquired);
}

static void window_removed(GtkApplication *app, GtkWindow *window, gpointer data) {
    (void)app;
    (void)data;
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    for (WidgetRecord *r = widgets; r; r = r->next) {
        if (r->widget != GTK_WIDGET(window)) continue;
        g_object_ref(window);
        r->native_destroying = true;
        widget_dispose(r);
        g_object_unref(window);
        break;
    }
    simp_runtime_managed_leave(acquired);
}

int64_t simp_gtk_widget_create(void *self, int64_t kind, void *text,
                               int64_t orientation, int64_t spacing, void *keep_alive) {
    require_live();
    if (!activated || g_application_get_is_remote(G_APPLICATION(application)))
        fatal("widgets must be created during or after local activation");
    void *receiver = simp_gc_root(self);
    const SimpClassMeta *metadata = *(const SimpClassMeta *const *)receiver;
    unsigned widget_bases = 0;
    for (uint64_t i = 0; i < metadata->base_class_count; ++i) {
        const SimpClassName *base = &metadata->base_classes[i];
        if (base->name_length == sizeof("Gtk.Widget") - 1 &&
            memcmp(base->name, "Gtk.Widget", sizeof("Gtk.Widget") - 1) == 0 &&
            ++widget_bases > 1)
            fatal("multiple Widget bases in one managed object are not supported");
    }
    if (orientation < 0 || orientation > 1 || spacing < 0 || spacing > G_MAXINT)
        fatal("invalid layout orientation or spacing");
    char *label = text_copy(text);
    GtkWidget *widget = NULL;
    switch (kind) {
    case 1: widget = gtk_application_window_new(application);
        gtk_window_set_title(GTK_WINDOW(widget), label); break;
    case 2: widget = gtk_box_new((GtkOrientation)orientation, (int)spacing); break;
    case 3: widget = gtk_label_new(label); break;
    case 4: widget = gtk_button_new_with_label(label); break;
    case 5: widget = gtk_entry_new();
        gtk_editable_set_text(GTK_EDITABLE(widget), label); break;
    case 6: widget = gtk_check_button_new_with_label(label); break;
    case 7: widget = gtk_scrolled_window_new(); break;
    default: fatal("invalid widget kind");
    }
    g_free(label);
    WidgetRecord *r = calloc(1, sizeof(*r));
    if (!r) fatal("allocation failed");
    r->token = token_new();
    /* Windows are already non-floating and GTK owns a reference. */
    r->widget = g_object_ref_sink(widget);
    r->receiver = receiver;
    r->root = simp_callback_acquire(keep_alive, "callback<void()>");
    r->next = widgets;
    widgets = r;
    g_signal_connect(widget, "destroy", G_CALLBACK(widget_destroyed), r);
    return r->token;
}

void simp_gtk_widget_dispose(void *self, int64_t token) {
    (void)self;
    require_managed();
    simp_gtk_require_owner();
    if (stopped) return;
    widget_dispose(record_find(token));
}

bool simp_gtk_widget_disposed(void *self, int64_t token) {
    (void)self;
    require_managed();
    simp_gtk_require_owner();
    return stopped || !record_find(token)->widget;
}

void simp_gtk_widget_visible(void *self, int64_t token, bool value) {
    (void)self;
    gtk_widget_set_visible(widget_live(token), value);
}

void simp_gtk_widget_sensitive(void *self, int64_t token, bool value) {
    (void)self;
    gtk_widget_set_sensitive(widget_live(token), value);
}

void simp_gtk_widget_text_set(void *self, int64_t token, void *text) {
    (void)self;
    GtkWidget *w = g_object_ref(widget_live(token));
    char *label = text_copy(text);
    if (GTK_IS_WINDOW(w)) gtk_window_set_title(GTK_WINDOW(w), label);
    else if (GTK_IS_LABEL(w)) gtk_label_set_text(GTK_LABEL(w), label);
    else if (GTK_IS_BUTTON(w)) gtk_button_set_label(GTK_BUTTON(w), label);
    else if (GTK_IS_EDITABLE(w)) gtk_editable_set_text(GTK_EDITABLE(w), label);
    else if (GTK_IS_CHECK_BUTTON(w)) gtk_check_button_set_label(GTK_CHECK_BUTTON(w), label);
    else fatal("widget has no text property");
    g_free(label);
    g_object_unref(w);
}

void *simp_gtk_widget_text_get(void *self, int64_t token) {
    (void)self;
    GtkWidget *w = widget_live(token);
    const char *text;
    if (GTK_IS_WINDOW(w)) text = gtk_window_get_title(GTK_WINDOW(w));
    else if (GTK_IS_LABEL(w)) text = gtk_label_get_text(GTK_LABEL(w));
    else if (GTK_IS_BUTTON(w)) text = gtk_button_get_label(GTK_BUTTON(w));
    else if (GTK_IS_EDITABLE(w)) text = gtk_editable_get_text(GTK_EDITABLE(w));
    else if (GTK_IS_CHECK_BUTTON(w)) text = gtk_check_button_get_label(GTK_CHECK_BUTTON(w));
    else fatal("widget has no text property");
    if (!text) text = "";
    return simp_string_new(&simp_string_class_meta, text, strlen(text));
}

void simp_gtk_widget_attach(void *self, int64_t parent, int64_t child) {
    (void)self;
    GtkWidget *p = widget_live(parent);
    GtkWidget *c = widget_live(child);
    WidgetRecord *pr = record_find(parent), *cr = record_find(child);
    if (GTK_IS_WINDOW(c) || pr == cr || cr->parent || gtk_widget_get_parent(c))
        fatal("child is a window, already parented, or self");
    for (WidgetRecord *r = pr; r; r = r->parent)
        if (r == cr) fatal("parenting cycle");
    if (GTK_IS_BOX(p)) gtk_box_append(GTK_BOX(p), c);
    else if (GTK_IS_WINDOW(p)) {
        if (gtk_window_get_child(GTK_WINDOW(p))) fatal("container already has a child");
        gtk_window_set_child(GTK_WINDOW(p), c);
    } else if (GTK_IS_SCROLLED_WINDOW(p)) {
        if (gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(p)))
            fatal("container already has a child");
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(p), c);
    } else fatal("widget is not a container");
    cr->parent = pr;
}

void simp_gtk_widget_remove(void *self, int64_t parent, int64_t child) {
    (void)self;
    widget_live(parent);
    widget_live(child);
    WidgetRecord *cr = record_find(child);
    if (cr->parent != record_find(parent)) fatal("widget is not a child of this container");
    detach(cr);
}

void simp_gtk_window_action(void *self, int64_t token, int64_t action) {
    (void)self;
    GtkWidget *w = g_object_ref(widget_live(token));
    if (!GTK_IS_WINDOW(w)) fatal("operation requires Window");
    if (action == 0) gtk_window_present(GTK_WINDOW(w));
    else if (action == 1) {
        if (!gtk_widget_get_realized(w))
            fatal("close requires a presented window; use dispose before presentation");
        gtk_window_close(GTK_WINDOW(w));
    }
    else fatal("invalid window action");
    g_object_unref(w);
}

void simp_gtk_window_size(void *self, int64_t token, int64_t width, int64_t height) {
    (void)self;
    GtkWidget *w = widget_live(token);
    if (!GTK_IS_WINDOW(w) || width < 1 || height < 1 || width > G_MAXINT || height > G_MAXINT)
        fatal("invalid window size");
    gtk_window_set_default_size(GTK_WINDOW(w), (int)width, (int)height);
}

void simp_gtk_box_layout(void *self, int64_t token, int64_t orientation, int64_t spacing) {
    (void)self;
    GtkWidget *w = widget_live(token);
    if (!GTK_IS_BOX(w) || orientation < 0 || orientation > 1 || spacing < 0 || spacing > G_MAXINT)
        fatal("invalid box layout");
    gtk_orientable_set_orientation(GTK_ORIENTABLE(w), (GtkOrientation)orientation);
    gtk_box_set_spacing(GTK_BOX(w), (int)spacing);
}

void simp_gtk_checkbox_set(void *self, int64_t token, bool active) {
    (void)self;
    GtkWidget *w = g_object_ref(widget_live(token));
    if (!GTK_IS_CHECK_BUTTON(w)) fatal("operation requires CheckButton");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(w), active);
    g_object_unref(w);
}

bool simp_gtk_checkbox_get(void *self, int64_t token) {
    (void)self;
    GtkWidget *w = widget_live(token);
    if (!GTK_IS_CHECK_BUTTON(w)) fatal("operation requires CheckButton");
    return gtk_check_button_get_active(GTK_CHECK_BUTTON(w)) != FALSE;
}

int64_t simp_gtk_widget_signal(void *self, int64_t token, int64_t kind, void *callback) {
    (void)self;
    GtkWidget *w = widget_live(token);
    if (kind == 0) return simp_gtk_connect_clicked(GTK_BUTTON(w), callback);
    if (kind == 1) {
        if (!GTK_IS_EDITABLE(w)) fatal("changed requires Entry");
        return connect_signal(G_OBJECT(w), "changed", G_CALLBACK(text_changed),
                              callback, "callback<void(String)>");
    }
    if (kind == 2) return simp_gtk_connect_close_request(GTK_WINDOW(w), callback);
    if (kind == 3) {
        if (!GTK_IS_CHECK_BUTTON(w)) fatal("toggled requires CheckButton");
        return connect_signal(G_OBJECT(w), "toggled", G_CALLBACK(toggled),
                              callback, "callback<void(bool)>");
    }
    fatal("invalid signal kind");
}

static int64_t source_token(void *source) {
    require_live();
    if (!source) fatal("null signal source");
    void *receiver = simp_gc_root(source);
    for (WidgetRecord *r = widgets; r; r = r->next)
        if (r->receiver == receiver && r->widget) return r->token;
    fatal("widget has been disposed or is not initialized");
}

int64_t simp_gtk_button_signal(void *self, void *source, void *callback) {
    return simp_gtk_widget_signal(self, source_token(source), 0, callback);
}
int64_t simp_gtk_entry_signal(void *self, void *source, void *callback) {
    return simp_gtk_widget_signal(self, source_token(source), 1, callback);
}
int64_t simp_gtk_window_signal(void *self, void *source, void *callback) {
    return simp_gtk_widget_signal(self, source_token(source), 2, callback);
}
int64_t simp_gtk_checkbox_signal(void *self, void *source, void *callback) {
    return simp_gtk_widget_signal(self, source_token(source), 3, callback);
}
