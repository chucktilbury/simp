#include "gtk_private.h"
#include <simp/Callbacks.h>
#include <simp/RuntimeGc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef SIMP_GTK_SOURCEVIEW
#include <gtksourceview/gtksource.h>
#endif

extern const SimpClassMeta simp_string_class_meta;

typedef struct ShortcutBinding ShortcutBinding;
typedef struct MenuItem {
    struct MenuItem *next;
    int64_t token;
    GSimpleAction *action;
} MenuItem;

typedef struct FileDialog {
    struct FileDialog *next;
    int64_t token;
    int64_t parent;
    GtkFileChooserNative *chooser;
    SimpCallbackContext *context;
    GSource *completion;
    GSource *presentation;
    GPtrArray *paths;
    bool multiple;
} FileDialog;

typedef struct WidgetRecord {
    struct WidgetRecord *next;
    int64_t token;
    GtkWidget *widget;
    struct WidgetRecord *parent;
    void *receiver;
    SimpCallbackContext *root;
    ShortcutBinding *shortcuts;
    GMenu *menu;
    GPtrArray *submenus;
    GSimpleActionGroup *actions;
    MenuItem *items;
    bool native_destroying;
} WidgetRecord;

#ifdef SIMP_GTK_SOURCEVIEW
/* A shortcut callback may dispose its own view (closing a tab or the window).
 * While `active` is nonzero, disposal only marks the binding: the callback
 * registration is released, and an orphaned binding freed, when the outermost
 * invocation unwinds. */
struct ShortcutBinding {
    ShortcutBinding *next;
    WidgetRecord *owner;
    SimpCallbackContext *context;
    char *trigger;
    GtkEventController *controller;
    unsigned active;
    bool release_pending;
    bool orphaned;
};

static void shortcut_release(ShortcutBinding *binding) {
    if (!binding->context) return;
    if (binding->active) {
        binding->release_pending = true;
        return;
    }
    simp_callback_release(binding->context);
    simp_callback_dispose(binding->context);
    binding->context = NULL;
    binding->release_pending = false;
}
#endif

typedef struct Connection {
    struct Connection *next;
    int64_t token;
    GObject *object;
    gulong signal;
    gulong additional[3];
    unsigned additional_count;
    unsigned closures;
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
static FileDialog *dialogs;
static GtkApplication *application;
static bool activated;
static bool ran;
static bool scheduler_hold;
#ifdef SIMP_GTK_SOURCEVIEW
static GtkSourceLanguageManager *source_language_manager;
/* Language ids whose <id>.lang file is installed by the sourceview package. */
static GHashTable *registered_languages;
#endif
static void widget_dispose(WidgetRecord *record);
static void window_removed(GtkApplication *app, GtkWindow *window, gpointer data);
static void dialog_finish(FileDialog *dialog, bool invoke);

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
    if (--connection->closures == 0) {
        connection->object = NULL;
        connection->closed = true;
        connection->detached = true;
    }
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

static void page_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num,
                          gpointer data) {
    (void)notebook;
    (void)page;
    (void)page_num;
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

#ifdef SIMP_GTK_SOURCEVIEW
static void changed_no_args(GObject *object, gpointer data) {
    (void)object;
    Connection *connection = data;
    int acquired = signal_enter(connection);
    typedef void (*Adapter)(SimpCallbackContext *);
    ((Adapter)simp_callback_adapter(connection->context))(connection->context);
    signal_leave(connection, acquired);
}

static void state_notified(GObject *object, GParamSpec *property, gpointer data) {
    (void)property;
    changed_no_args(object, data);
}

static void cursor_moved(GtkTextBuffer *buffer, GtkTextIter *location,
                         GtkTextMark *mark, gpointer data) {
    (void)location;
    if (mark != gtk_text_buffer_get_insert(buffer) &&
        mark != gtk_text_buffer_get_selection_bound(buffer)) return;
    Connection *connection = data;
    int acquired = signal_enter(connection);
    typedef void (*Adapter)(SimpCallbackContext *);
    ((Adapter)simp_callback_adapter(connection->context))(connection->context);
    signal_leave(connection, acquired);
}
#endif

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
    connection->closures = 1;
    connection->next = connections;
    connections = connection;
    connection->signal = g_signal_connect_data(object, signal, forwarder, connection,
                                               connection_destroy, 0);
    if (!connection->signal) fatal("signal connection failed");
    return connection->token;
}

#ifdef SIMP_GTK_SOURCEVIEW
static void connect_state(int64_t token, const char *signal) {
    Connection *connection = connections;
    while (connection && connection->token != token) connection = connection->next;
    if (!connection || connection->additional_count == G_N_ELEMENTS(connection->additional))
        fatal("invalid grouped signal connection");
    ++connection->closures;
    gulong id = g_signal_connect_data(connection->object, signal, G_CALLBACK(state_notified),
                                     connection, connection_destroy, 0);
    if (!id) fatal("signal connection failed");
    connection->additional[connection->additional_count++] = id;
}
#endif

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
        GObject *object = connection->object;
        gulong ids[4] = { connection->signal };
        unsigned count = connection->additional_count + 1;
        memcpy(ids + 1, connection->additional, connection->additional_count * sizeof(gulong));
        for (unsigned i = 0; i < count; ++i) g_signal_handler_disconnect(object, ids[i]);
        return true;
    }
    return false;
}

#ifdef SIMP_GTK_SOURCEVIEW
static void register_languages(const char *directory) {
    GDir *dir = g_dir_open(directory, 0, NULL);
    if (!dir) return;
    for (const char *name; (name = g_dir_read_name(dir));) {
        if (g_str_has_suffix(name, ".lang") && strlen(name) > 5)
            g_hash_table_add(registered_languages, g_strndup(name, strlen(name) - 5));
    }
    g_dir_close(dir);
}
#endif

static void application_initialize(const char *id, bool scheduler) {
    require_managed();
    if (initialized) fatal("application initialization is single-use");
    if (!gtk_init_check()) fatal("GTK initialization failed (display unavailable)");
#ifdef SIMP_GTK_SOURCEVIEW
    GtkSourceLanguageManager *defaults = gtk_source_language_manager_get_default();
    const char * const *existing_paths =
        gtk_source_language_manager_get_search_path(defaults);
    GPtrArray *paths = g_ptr_array_new();
    for (size_t i = 0; existing_paths && existing_paths[i]; ++i)
        g_ptr_array_add(paths, (gpointer)existing_paths[i]);
    if (g_file_test(SIMP_GTK_SOURCEVIEW_SOURCE_LANG_DIR, G_FILE_TEST_IS_DIR))
        g_ptr_array_add(paths, (gpointer)SIMP_GTK_SOURCEVIEW_SOURCE_LANG_DIR);
    if (g_file_test(SIMP_GTK_SOURCEVIEW_INSTALL_LANG_DIR, G_FILE_TEST_IS_DIR))
        g_ptr_array_add(paths, (gpointer)SIMP_GTK_SOURCEVIEW_INSTALL_LANG_DIR);
    g_ptr_array_add(paths, NULL);
    registered_languages = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    register_languages(SIMP_GTK_SOURCEVIEW_SOURCE_LANG_DIR);
    register_languages(SIMP_GTK_SOURCEVIEW_INSTALL_LANG_DIR);
    source_language_manager = gtk_source_language_manager_new();
    gtk_source_language_manager_set_search_path(source_language_manager,
                                                 (const char **)paths->pdata);
    g_ptr_array_unref(paths);
#endif
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
    while (dialogs) dialog_finish(dialogs, false);
    for (Connection *connection = connections; connection;) {
        Connection *next = connection->next;
        if (!connection->closed) simp_gtk_disconnect(connection->token);
        connection = next;
    }
    for (WidgetRecord *record = widgets; record; record = record->next) {
        if (record->widget && GTK_IS_WINDOW(record->widget))
            gtk_window_set_focus(GTK_WINDOW(record->widget), NULL);
    }
    for (WidgetRecord *record = widgets; record; record = record->next)
        widget_dispose(record);
    config_finish_shutdown();
#ifdef SIMP_GTK_SOURCEVIEW
    g_clear_object(&source_language_manager);
    g_clear_pointer(&registered_languages, g_hash_table_unref);
#endif
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

#include "explorer.c"

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
    else if (GTK_IS_PANED(parent)) {
        if (gtk_paned_get_start_child(GTK_PANED(parent)) == r->widget)
            gtk_paned_set_start_child(GTK_PANED(parent), NULL);
        else gtk_paned_set_end_child(GTK_PANED(parent), NULL);
    }
    else if (GTK_IS_NOTEBOOK(parent)) {
        int page = gtk_notebook_page_num(GTK_NOTEBOOK(parent), r->widget);
        if (page >= 0) gtk_notebook_remove_page(GTK_NOTEBOOK(parent), page);
    }
    else fatal("invalid widget parent");
}

static void widget_dispose(WidgetRecord *r) {
    if (!r->widget) return;
    GtkWidget *widget = r->widget;
    if (GTK_IS_WINDOW(widget)) gtk_window_set_focus(GTK_WINDOW(widget), NULL);
    directory_cancel_owner(r->token);
    tree_stop(widget);
    for (FileDialog *dialog = dialogs; dialog;) {
        FileDialog *next = dialog->next;
        if (dialog->parent == r->token) dialog_finish(dialog, false);
        dialog = next;
    }
    while (r->items) {
        MenuItem *item = r->items;
        r->items = item->next;
        simp_gtk_disconnect(item->token);
        g_object_unref(item->action);
        free(item);
    }
    if (r->actions) {
        gtk_widget_insert_action_group(widget, "menu", NULL);
        g_clear_object(&r->actions);
        g_clear_pointer(&r->submenus, g_ptr_array_unref);
        g_clear_object(&r->menu);
    }
    disconnect_object(G_OBJECT(widget));
#ifdef SIMP_GTK_SOURCEVIEW
    if (GTK_SOURCE_IS_VIEW(widget))
        disconnect_object(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget))));
    for (ShortcutBinding *binding = r->shortcuts; binding;) {
        ShortcutBinding *next = binding->next;
        shortcut_release(binding);
        binding->owner = NULL;
        binding = next;
    }
    r->shortcuts = NULL;
#endif
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
    case 11: widget = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(widget), label); break;
    case 2: widget = gtk_box_new((GtkOrientation)orientation, (int)spacing); break;
    case 3: widget = gtk_label_new(label); break;
    case 4: widget = gtk_button_new_with_label(label); break;
    case 5: widget = gtk_entry_new();
        gtk_editable_set_text(GTK_EDITABLE(widget), label); break;
    case 6: widget = gtk_check_button_new_with_label(label); break;
    case 7: widget = gtk_scrolled_window_new(); break;
    case 12: widget = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_paned_set_resize_start_child(GTK_PANED(widget), FALSE);
        gtk_paned_set_shrink_start_child(GTK_PANED(widget), TRUE);
        gtk_paned_set_shrink_end_child(GTK_PANED(widget), FALSE);
        break;
    case 13: widget = gtk_list_view_new(NULL, NULL);
        gtk_list_view_set_single_click_activate(GTK_LIST_VIEW(widget), FALSE);
        break;
    case 9: widget = gtk_notebook_new(); break;
    case 10: {
        GMenu *menu = g_menu_new();
        widget = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(menu));
        g_object_unref(menu);
        break;
    }
#ifdef SIMP_GTK_SOURCEVIEW
    case 8:
        widget = GTK_WIDGET(gtk_source_view_new());
        gtk_text_buffer_set_enable_undo(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)), TRUE);
        break;
#endif
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
    if (kind == 10) {
        r->menu = G_MENU(g_object_ref(gtk_popover_menu_bar_get_menu_model(GTK_POPOVER_MENU_BAR(widget))));
        r->submenus = g_ptr_array_new_with_free_func(g_object_unref);
        r->actions = g_simple_action_group_new();
        gtk_widget_insert_action_group(widget, "menu", G_ACTION_GROUP(r->actions));
    }
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

void simp_gtk_widget_expand(void *self, int64_t token, bool horizontal, bool value) {
    (void)self;
    GtkWidget *widget = widget_live(token);
    if (horizontal) gtk_widget_set_hexpand(widget, value);
    else gtk_widget_set_vexpand(widget, value);
}

void simp_gtk_widget_focus(void *self, int64_t token) {
    (void)self;
    gtk_widget_grab_focus(widget_live(token));
}

static WidgetRecord *menu_record(int64_t token) {
    if (!GTK_IS_POPOVER_MENU_BAR(widget_live(token))) fatal("operation requires MenuBar");
    return record_find(token);
}

int64_t simp_gtk_menu_add(void *self, int64_t token, void *label) {
    (void)self;
    WidgetRecord *record = menu_record(token);
    char *text = text_copy(label);
    GMenu *menu = g_menu_new();
    int64_t index = record->submenus->len;
    g_ptr_array_add(record->submenus, menu);
    g_menu_append_submenu(record->menu, text, G_MENU_MODEL(menu));
    g_free(text);
    return index;
}

static void menu_activated(GSimpleAction *action, GVariant *parameter, gpointer data) {
    (void)action;
    (void)parameter;
    clicked(NULL, data);
}

int64_t simp_gtk_menu_item_add(void *self, int64_t token, int64_t menu,
                               void *label, void *callback) {
    (void)self;
    WidgetRecord *record = menu_record(token);
    if (menu < 0 || menu >= record->submenus->len) fatal("invalid menu index");
    char *text = text_copy(label);
    MenuItem *item = calloc(1, sizeof(*item));
    if (!item) fatal("allocation failed");
    char *name = g_strdup_printf("item%" G_GINT64_FORMAT, token_new());
    item->action = g_simple_action_new(name, NULL);
    item->token = connect_signal(G_OBJECT(item->action), "activate",
                                G_CALLBACK(menu_activated), callback, "callback<void()>");
    g_action_map_add_action(G_ACTION_MAP(record->actions), G_ACTION(item->action));
    char *detailed = g_strconcat("menu.", name, NULL);
    g_menu_append(g_ptr_array_index(record->submenus, (guint)menu), text, detailed);
    g_free(detailed);
    g_free(name);
    g_free(text);
    item->next = record->items;
    record->items = item;
    return item->token;
}

static GSimpleAction *menu_item(int64_t token, int64_t item) {
    for (MenuItem *i = menu_record(token)->items; i; i = i->next)
        if (i->token == item) return i->action;
    fatal("invalid menu item");
}

void simp_gtk_menu_item_enabled(void *self, int64_t token, int64_t item, bool enabled) {
    (void)self;
    g_simple_action_set_enabled(menu_item(token, item), enabled);
}

void simp_gtk_menu_item_activate(void *self, int64_t token, int64_t item) {
    (void)self;
    GSimpleAction *action = g_object_ref(menu_item(token, item));
    g_action_activate(G_ACTION(action), NULL);
    g_object_unref(action);
}

static void *path_string(const char *path) {
    return simp_string_new(&simp_string_class_meta, path, strlen(path));
}

/* Remove all native ownership before invoking Simple: a result handler may
 * immediately dispose its parent, open another chooser, or shut down. A
 * single-file chooser delivers one path ("" when cancelled); a multi-select
 * chooser delivers each selected path and then "" to end the batch. */
static void dialog_finish(FileDialog *dialog, bool invoke) {
    GPtrArray *paths = dialog->paths;
    bool multiple = dialog->multiple;
    FileDialog **link = &dialogs;
    while (*link != dialog) link = &(*link)->next;
    *link = dialog->next;
    if (dialog->completion) {
        g_source_destroy(dialog->completion);
        g_source_unref(dialog->completion);
    }
    if (dialog->chooser) {
        g_signal_handlers_disconnect_by_data(dialog->chooser, dialog);
        gtk_native_dialog_hide(GTK_NATIVE_DIALOG(dialog->chooser));
        g_object_unref(dialog->chooser);
    }
    SimpCallbackContext *context = dialog->context;
    free(dialog);
    if (invoke) {
        typedef void (*Adapter)(SimpCallbackContext *, void *);
        Adapter adapter = (Adapter)simp_callback_adapter(context);
        guint deliveries = multiple ? paths->len + 1 : 1;
        for (guint i = 0; i < deliveries; ++i) {
            void *text = path_string(i < paths->len ? g_ptr_array_index(paths, i) : "");
            void *slots[] = { &text };
            SimpRootFrame frame = {0};
            simp_gc_push_or_abort(&frame, slots, 1);
            adapter(context, text);
            simp_gc_pop_or_abort(&frame);
        }
    }
    g_ptr_array_unref(paths);
    simp_callback_release(context);
    simp_callback_dispose(context);
}

static gboolean dialog_complete(gpointer data) {
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    FileDialog *dialog = data;
    g_source_unref(dialog->completion);
    dialog->completion = NULL;
    dialog_finish(dialog, true);
    simp_runtime_managed_leave(acquired);
    return G_SOURCE_REMOVE;
}

static void dialog_schedule(FileDialog *dialog) {
    if (dialog->completion) return;
    dialog->completion = g_idle_source_new();
    g_source_set_callback(dialog->completion, dialog_complete, dialog, NULL);
    g_source_attach(dialog->completion, main_context);
}

static void chooser_response(GtkNativeDialog *chooser, int response, gpointer data) {
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    FileDialog *dialog = data;
    if (!dialog->completion && response == GTK_RESPONSE_ACCEPT) {
        GListModel *files = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(chooser));
        guint count = files ? g_list_model_get_n_items(files) : 0;
        if (!dialog->multiple) count = MIN(count, 1u);
        for (guint i = 0; i < count; ++i) {
            GFile *file = g_list_model_get_item(files, i);
            char *path = g_file_get_path(file);
            /* Simple strings are UTF-8; fall back to the (ASCII) URI otherwise. */
            if (!path || !g_utf8_validate(path, -1, NULL)) {
                g_free(path);
                path = g_file_get_uri(file);
            }
            g_ptr_array_add(dialog->paths, path);
            g_object_unref(file);
        }
        g_clear_object(&files);
    }
    /* GTK's fallback chooser still has response cleanup to perform. Release
     * native dialogs and enter user code only after that signal unwinds. */
    dialog_schedule(dialog);
    simp_runtime_managed_leave(acquired);
}

static int64_t file_dialog_create(int64_t parent, bool save, bool multiple,
                                  void *initial_path, void *callback) {
    GtkWidget *window = widget_live(parent);
    if (!GTK_IS_WINDOW(window)) fatal("FileDialog requires Window");
    char *path = text_copy(initial_path);
    FileDialog *dialog = calloc(1, sizeof(*dialog));
    if (!dialog) fatal("allocation failed");
    dialog->token = token_new();
    dialog->parent = parent;
    dialog->multiple = multiple;
    dialog->paths = g_ptr_array_new_with_free_func(g_free);
    dialog->context = simp_callback_acquire(callback, "callback<void(String)>");
    dialog->chooser = gtk_file_chooser_native_new(
        save ? "Save file" : "Open file", GTK_WINDOW(window),
        save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
        save ? "_Save" : "_Open", "_Cancel");
    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(dialog->chooser), TRUE);
    if (multiple)
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog->chooser), TRUE);
    if (*path) {
        GFile *file = g_file_new_for_path(path);
        if (g_file_test(path, G_FILE_TEST_IS_DIR))
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog->chooser), file, NULL);
        else if (save) {
            GFile *folder = g_file_get_parent(file);
            if (folder) {
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog->chooser), folder, NULL);
                g_object_unref(folder);
            }
            char *name = g_file_get_basename(file);
            gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog->chooser), name);
            g_free(name);
        } else if (multiple) {
            GFile *folder = g_file_get_parent(file);
            if (folder) {
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog->chooser), folder, NULL);
                g_object_unref(folder);
            }
        } else gtk_file_chooser_set_file(GTK_FILE_CHOOSER(dialog->chooser), file, NULL);
        g_object_unref(file);
    }
    g_free(path);
    dialog->next = dialogs;
    dialogs = dialog;
    int64_t token = dialog->token;
    g_signal_connect(dialog->chooser, "response", G_CALLBACK(chooser_response), dialog);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog->chooser));
    return token;
}

int64_t simp_gtk_file_dialog_create(void *self, int64_t parent, bool save,
                                    void *initial_path, void *callback) {
    (void)self;
    return file_dialog_create(parent, save, false, false, initial_path, callback);
}

int64_t simp_gtk_file_dialog_create_multiple(void *self, int64_t parent, void *initial_path,
                                             void *callback) {
    (void)self;
    return file_dialog_create(parent, false, true, false, initial_path, callback);
}

int64_t simp_gtk_file_dialog_create_folder(void *self, int64_t parent, void *initial_path,
                                          void *callback) {
    (void)self;
    return file_dialog_create(parent, false, false, true, initial_path, callback);
}

/* GtkAlertDialog owns its window: it is modal, transient for the parent, and
 * destroys itself when dismissed or when the parent is destroyed. */
void simp_gtk_alert_show(void *self, int64_t parent, void *message, void *detail) {
    (void)self;
    require_managed();
    simp_gtk_require_owner();
    GtkWidget *window = widget_live(parent);
    if (!GTK_IS_WINDOW(window)) fatal("AlertDialog requires Window");
    char *heading = text_copy(message);
    char *body = text_copy(detail);
    GtkAlertDialog *alert = gtk_alert_dialog_new("%s", heading);
    if (*body) gtk_alert_dialog_set_detail(alert, body);
    gtk_alert_dialog_set_modal(alert, TRUE);
    gtk_alert_dialog_show(alert, GTK_WINDOW(window));
    g_object_unref(alert);
    g_free(body);
    g_free(heading);
}

void simp_gtk_file_dialog_cancel(void *self, int64_t token) {
    (void)self;
    require_managed();
    simp_gtk_require_owner();
    for (FileDialog *dialog = dialogs; dialog; dialog = dialog->next)
        if (dialog->token == token) {
            dialog_finish(dialog, false);
            return;
        }
}

bool simp_gtk_file_dialog_pending(void *self, int64_t token) {
    (void)self;
    require_managed();
    simp_gtk_require_owner();
    for (FileDialog *dialog = dialogs; dialog; dialog = dialog->next)
        if (dialog->token == token) return true;
    return false;
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
    } else if (GTK_IS_NOTEBOOK(p)) {
        fatal("use Notebook.appendPage to attach a notebook page");
    } else if (GTK_IS_PANED(p)) {
        if (!gtk_paned_get_start_child(GTK_PANED(p)))
            gtk_paned_set_start_child(GTK_PANED(p), c);
        else if (!gtk_paned_get_end_child(GTK_PANED(p)))
            gtk_paned_set_end_child(GTK_PANED(p), c);
        else fatal("Paned already has two children");
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

void simp_gtk_window_transient_for(void *self, int64_t token, int64_t parent) {
    (void)self;
    require_managed();
    simp_gtk_require_owner();
    GtkWidget *window = widget_live(token);
    GtkWidget *parent_window = widget_live(parent);
    if (!GTK_IS_WINDOW(window) || !GTK_IS_WINDOW(parent_window) || window == parent_window)
        fatal("transient parent and child must be distinct Windows");
    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(parent_window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);
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

void simp_gtk_notebook_append(void *self, int64_t notebook, int64_t child, void *title) {
    (void)self;
    GtkWidget *parent = widget_live(notebook);
    GtkWidget *page = widget_live(child);
    WidgetRecord *pr = record_find(notebook), *cr = record_find(child);
    if (!GTK_IS_NOTEBOOK(parent) || GTK_IS_WINDOW(page) || pr == cr ||
        cr->parent || gtk_widget_get_parent(page))
        fatal("invalid notebook page");
    for (WidgetRecord *r = pr; r; r = r->parent)
        if (r == cr) fatal("parenting cycle");
    char *label = text_copy(title);
    gtk_notebook_append_page(GTK_NOTEBOOK(parent), page, gtk_label_new(label));
    g_free(label);
    cr->parent = pr;
}

int64_t simp_gtk_notebook_current(void *self, int64_t token) {
    (void)self;
    GtkWidget *widget = widget_live(token);
    if (!GTK_IS_NOTEBOOK(widget)) fatal("operation requires Notebook");
    return gtk_notebook_get_current_page(GTK_NOTEBOOK(widget));
}

void simp_gtk_notebook_set_current(void *self, int64_t token, int64_t page) {
    (void)self;
    GtkWidget *widget = widget_live(token);
    if (!GTK_IS_NOTEBOOK(widget) || page < 0 ||
        page >= gtk_notebook_get_n_pages(GTK_NOTEBOOK(widget)))
        fatal("invalid notebook page");
    gtk_notebook_set_current_page(GTK_NOTEBOOK(widget), (int)page);
}

int64_t simp_gtk_notebook_count(void *self, int64_t token) {
    (void)self;
    GtkWidget *widget = widget_live(token);
    if (!GTK_IS_NOTEBOOK(widget)) fatal("operation requires Notebook");
    return gtk_notebook_get_n_pages(GTK_NOTEBOOK(widget));
}

void simp_gtk_notebook_title(void *self, int64_t token, int64_t page, void *title) {
    (void)self;
    GtkWidget *widget = widget_live(token);
    if (!GTK_IS_NOTEBOOK(widget) || page < 0 ||
        page >= gtk_notebook_get_n_pages(GTK_NOTEBOOK(widget)))
        fatal("invalid notebook page");
    GtkWidget *tab = gtk_notebook_get_tab_label(GTK_NOTEBOOK(widget),
                                                gtk_notebook_get_nth_page(GTK_NOTEBOOK(widget), (int)page));
    if (!GTK_IS_LABEL(tab)) fatal("notebook tab label is not a Label");
    char *label = text_copy(title);
    gtk_label_set_text(GTK_LABEL(tab), label);
    g_free(label);
}

#ifdef SIMP_GTK_SOURCEVIEW
static GtkSourceBuffer *source_buffer(int64_t token) {
    GtkWidget *widget = widget_live(token);
    if (!GTK_SOURCE_IS_VIEW(widget)) fatal("operation requires GtkSourceView");
    return GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)));
}

static void shortcut_free(ShortcutBinding *binding) {
    g_free(binding->trigger);
    free(binding);
}

static gboolean shortcut_invoke(GtkWidget *widget, GVariant *args, gpointer data) {
    (void)widget;
    (void)args;
    simp_gtk_require_owner();
    ShortcutBinding *binding = data;
    if (!binding->context || binding->release_pending) return FALSE;
    int acquired = simp_runtime_managed_enter();
    ++binding->active;
    typedef void (*Adapter)(SimpCallbackContext *);
    ((Adapter)simp_callback_adapter(binding->context))(binding->context);
    if (--binding->active == 0) {
        if (binding->release_pending) shortcut_release(binding);
        if (binding->orphaned) shortcut_free(binding);
    }
    simp_runtime_managed_leave(acquired);
    return TRUE;
}

/* GtkCallbackAction destroy notify. GTK may finalize the action while its
 * callback is still running (the view or window was disposed from inside the
 * shortcut); shortcut_invoke then completes the release on unwind. */
static void shortcut_context_dispose(gpointer data) {
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    ShortcutBinding *binding = data;
    if (binding->owner) {
        ShortcutBinding **link = &binding->owner->shortcuts;
        while (*link && *link != binding) link = &(*link)->next;
        if (*link == binding) *link = binding->next;
        binding->owner = NULL;
    }
    shortcut_release(binding);
    if (binding->active) binding->orphaned = true;
    else shortcut_free(binding);
    simp_runtime_managed_leave(acquired);
}

int64_t simp_gtk_source_view_changed(void *self, int64_t token, void *callback) {
    (void)self;
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    int64_t connection = connect_signal(G_OBJECT(buffer), "changed", G_CALLBACK(changed_no_args),
                                        callback, "callback<void()>");
    connect_state(connection, "notify::can-undo");
    connect_state(connection, "notify::can-redo");
    return connection;
}

int64_t simp_gtk_source_view_cursor_moved(void *self, int64_t token, void *callback) {
    (void)self;
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    int64_t connection = connect_signal(G_OBJECT(buffer), "mark-set", G_CALLBACK(cursor_moved),
                                        callback, "callback<void()>");
    connect_state(connection, "notify::has-selection");
    return connection;
}

bool simp_gtk_source_view_bind_shortcut(void *self, int64_t token, void *trigger_text,
                                        void *callback) {
    (void)self;
    GtkWidget *widget = widget_live(token);
    if (!GTK_SOURCE_IS_VIEW(widget)) fatal("operation requires GtkSourceView");
    char *trigger_name = text_copy(trigger_text);
    GtkShortcutTrigger *trigger = gtk_shortcut_trigger_parse_string(trigger_name);
    if (!trigger) {
        g_free(trigger_name);
        return false;
    }
    ShortcutBinding *binding = calloc(1, sizeof(*binding));
    if (!binding) fatal("allocation failed");
    binding->owner = record_find(token);
    binding->context = simp_callback_acquire(callback, "callback<void()>");
    binding->trigger = trigger_name;
    binding->next = binding->owner->shortcuts;
    binding->owner->shortcuts = binding;
    GtkShortcutAction *action = gtk_callback_action_new(
        shortcut_invoke, binding, shortcut_context_dispose);
    if (!action) fatal("could not create GTK shortcut action");
    GtkShortcut *shortcut = gtk_shortcut_new(trigger, action);
    GtkEventController *controller = gtk_shortcut_controller_new();
    binding->controller = controller;
    gtk_shortcut_controller_set_scope(GTK_SHORTCUT_CONTROLLER(controller),
                                      GTK_SHORTCUT_SCOPE_LOCAL);
    gtk_shortcut_controller_add_shortcut(GTK_SHORTCUT_CONTROLLER(controller), shortcut);
    gtk_widget_add_controller(widget, controller);
    return true;
}

bool simp_gtk_source_view_has_shortcut(void *self, int64_t token, void *trigger_text) {
    (void)self;
    char *trigger = text_copy(trigger_text);
    WidgetRecord *record = record_find(token);
    if (!GTK_SOURCE_IS_VIEW(record->widget)) fatal("operation requires GtkSourceView");
    bool found = false;
    for (ShortcutBinding *binding = record->shortcuts; binding; binding = binding->next)
        if (g_str_equal(binding->trigger, trigger)) {
            found = true;
            break;
        }
    g_free(trigger);
    return found;
}

void simp_gtk_source_view_set_text(void *self, int64_t token, void *text) {
    (void)self;
    char *bytes = text_copy(text);
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(source_buffer(token)), bytes, -1);
    g_free(bytes);
}

void *simp_gtk_source_view_get_text(void *self, int64_t token) {
    (void)self;
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
    void *result = simp_string_new(&simp_string_class_meta, text, strlen(text));
    g_free(text);
    return result;
}

bool simp_gtk_source_view_modified(void *self, int64_t token) {
    (void)self;
    return gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(source_buffer(token))) != FALSE;
}

void simp_gtk_source_view_mark_saved(void *self, int64_t token) {
    (void)self;
    gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(source_buffer(token)), FALSE);
}

void simp_gtk_source_view_undo(void *self, int64_t token) {
    (void)self;
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    if (gtk_text_buffer_get_can_undo(buffer)) gtk_text_buffer_undo(buffer);
}

void simp_gtk_source_view_redo(void *self, int64_t token) {
    (void)self;
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    if (gtk_text_buffer_get_can_redo(buffer)) gtk_text_buffer_redo(buffer);
}

bool simp_gtk_source_view_can_undo(void *self, int64_t token) {
    (void)self;
    return gtk_text_buffer_get_can_undo(GTK_TEXT_BUFFER(source_buffer(token))) != FALSE;
}

bool simp_gtk_source_view_can_redo(void *self, int64_t token) {
    (void)self;
    return gtk_text_buffer_get_can_redo(GTK_TEXT_BUFFER(source_buffer(token))) != FALSE;
}

bool simp_gtk_source_view_has_selection(void *self, int64_t token) {
    (void)self;
    return gtk_text_buffer_get_has_selection(GTK_TEXT_BUFFER(source_buffer(token))) != FALSE;
}

void simp_gtk_source_view_edit(void *self, int64_t token, int64_t operation) {
    (void)self;
    GtkTextBuffer *buffer = g_object_ref(GTK_TEXT_BUFFER(source_buffer(token)));
    GtkWidget *widget = g_object_ref(widget_live(token));
    GdkClipboard *clipboard = gtk_widget_get_clipboard(widget);
    bool editable = gtk_text_view_get_editable(GTK_TEXT_VIEW(widget));
    if (operation == 0) gtk_text_buffer_cut_clipboard(buffer, clipboard, editable);
    else if (operation == 1) gtk_text_buffer_copy_clipboard(buffer, clipboard);
    else if (operation == 2) gtk_text_buffer_paste_clipboard(buffer, clipboard, NULL, editable);
    else if (operation == 3) {
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        gtk_text_buffer_select_range(buffer, &start, &end);
    } else fatal("invalid source editing operation");
    g_object_unref(widget);
    g_object_unref(buffer);
}

void simp_gtk_source_view_monospace(void *self, int64_t token, bool monospace) {
    (void)self;
    GtkWidget *widget = widget_live(token);
    if (!GTK_SOURCE_IS_VIEW(widget)) fatal("operation requires GtkSourceView");
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(widget), monospace);
}

void simp_gtk_source_view_clear_shortcuts(void *self, int64_t token) {
    (void)self;
    GtkWidget *widget = widget_live(token);
    WidgetRecord *record = record_find(token);
    while (record->shortcuts) {
        ShortcutBinding *binding = record->shortcuts;
        record->shortcuts = binding->next;
        binding->owner = NULL;
        shortcut_release(binding);
        gtk_widget_remove_controller(widget, binding->controller);
    }
}

void simp_gtk_source_view_configure(void *self, int64_t token, void *font_text,
                                   int64_t size, int64_t tabs, bool spaces,
                                   bool numbers, bool wrap, bool highlight) {
    (void)self;
    GtkWidget *widget = widget_live(token);
    if (!GTK_SOURCE_IS_VIEW(widget)) fatal("operation requires GtkSourceView");
    if (size < 6 || size > 72 || tabs < 1 || tabs > 16) fatal("invalid source view settings");
    char *font = text_copy(font_text);
    /* CSS quoting prevents a user-editable font name from injecting rules. */
    GString *quoted = g_string_new("\"");
    for (const char *p = font; *p; ++p) {
        if (*p == '\\' || *p == '"') g_string_append_c(quoted, '\\');
        g_string_append_c(quoted, *p);
    }
    g_string_append_c(quoted, '"');
    char *css = g_strdup_printf("textview { font-family: %s; font-size: %" G_GINT64_FORMAT
                               "pt; }", quoted->str, size);
    const char *previous_css = g_object_get_data(G_OBJECT(widget), "simp-font-css");
    if (g_strcmp0(previous_css, css) != 0) {
        GtkCssProvider *provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(provider, css, -1);
        GtkStyleContext *context = gtk_widget_get_style_context(widget);
        GtkCssProvider *previous = g_object_get_data(G_OBJECT(widget), "simp-font-provider");
        if (previous) gtk_style_context_remove_provider(context, GTK_STYLE_PROVIDER(previous));
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider),
                                       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_set_data_full(G_OBJECT(widget), "simp-font-provider", provider, g_object_unref);
        g_object_set_data_full(G_OBJECT(widget), "simp-font-css", g_strdup(css), g_free);
    }
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(widget), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(widget), wrap ? GTK_WRAP_WORD_CHAR : GTK_WRAP_NONE);
    gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(widget), (guint)tabs);
    gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(widget), spaces);
    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(widget), numbers);
    gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(widget), highlight);
    g_free(font);
    g_free(css);
    g_string_free(quoted, TRUE);
}

/* Search option bits shared with GtkSource.View in sourceview.simp. */
enum {
    SEARCH_CASE_SENSITIVE = 1,
    SEARCH_WHOLE_WORDS = 2,
    SEARCH_IN_SELECTION = 4,
};

/* The "in selection" scope and the last match are buffer marks so they track
 * edits: the scope start has left gravity and its end right gravity, so text
 * replaced at either boundary stays inside the scope. */
#define SCOPE_START "simp-search-scope-start"
#define SCOPE_END "simp-search-scope-end"
#define MATCH_START "simp-search-match-start"
#define MATCH_END "simp-search-match-end"

static void mark_set(GtkTextBuffer *buffer, const char *name, const GtkTextIter *where,
                     gboolean left_gravity) {
    GtkTextMark *mark = gtk_text_buffer_get_mark(buffer, name);
    if (mark) gtk_text_buffer_move_mark(buffer, mark, where);
    else gtk_text_buffer_create_mark(buffer, name, where, left_gravity);
}

static bool mark_iter(GtkTextBuffer *buffer, const char *name, GtkTextIter *iter) {
    GtkTextMark *mark = gtk_text_buffer_get_mark(buffer, name);
    if (!mark) return false;
    gtk_text_buffer_get_iter_at_mark(buffer, iter, mark);
    return true;
}

static void mark_clear(GtkTextBuffer *buffer, const char *name) {
    GtkTextMark *mark = gtk_text_buffer_get_mark(buffer, name);
    if (mark) gtk_text_buffer_delete_mark(buffer, mark);
}

static bool word_char(gunichar c) { return g_unichar_isalnum(c) || c == '_'; }

static bool at_word_bounds(const GtkTextIter *start, const GtkTextIter *end) {
    GtkTextIter before = *start;
    if (gtk_text_iter_backward_char(&before) && word_char(gtk_text_iter_get_char(&before)))
        return false;
    return gtk_text_iter_is_end(end) || !word_char(gtk_text_iter_get_char(end));
}

static bool search_bounds(GtkTextBuffer *buffer, int64_t flags, GtkTextIter *low,
                          GtkTextIter *high) {
    if (!(flags & SEARCH_IN_SELECTION)) {
        gtk_text_buffer_get_bounds(buffer, low, high);
        return true;
    }
    return mark_iter(buffer, SCOPE_START, low) && mark_iter(buffer, SCOPE_END, high) &&
           gtk_text_iter_compare(low, high) < 0;
}

static bool search_range(GtkTextIter from, const GtkTextIter *high, const char *query,
                         int64_t flags, GtkTextIter *match_start, GtkTextIter *match_end) {
    GtkTextSearchFlags mode = flags & SEARCH_CASE_SENSITIVE ? 0 : GTK_TEXT_SEARCH_CASE_INSENSITIVE;
    while (gtk_text_iter_compare(&from, high) < 0) {
        if (!gtk_text_iter_forward_search(&from, query, mode, match_start, match_end, high))
            return false;
        if (!(flags & SEARCH_WHOLE_WORDS) || at_word_bounds(match_start, match_end)) return true;
        from = *match_start;
        if (!gtk_text_iter_forward_char(&from)) return false;
    }
    return false;
}

static bool selection_matches(GtkTextBuffer *buffer, const char *query, int64_t flags,
                              GtkTextIter *start, GtkTextIter *end) {
    GtkTextIter low, high, match_start, match_end;
    if (!gtk_text_buffer_get_selection_bounds(buffer, start, end)) return false;
    if (!search_bounds(buffer, flags, &low, &high)) return false;
    if (gtk_text_iter_compare(start, &low) < 0 || gtk_text_iter_compare(end, &high) > 0)
        return false;
    return search_range(*start, end, query, flags, &match_start, &match_end) &&
           gtk_text_iter_equal(&match_start, start) && gtk_text_iter_equal(&match_end, end);
}

static bool source_find(int64_t token, const char *query, int64_t flags) {
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    GtkTextIter low, high, from, selection_end, match_start, match_end;
    if (!*query || !search_bounds(buffer, flags, &low, &high)) return false;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &from, &selection_end))
        gtk_text_buffer_get_iter_at_mark(buffer, &from, gtk_text_buffer_get_insert(buffer));
    else from = selection_end;
    if (gtk_text_iter_compare(&from, &low) < 0 || gtk_text_iter_compare(&from, &high) > 0)
        from = low;
    bool found = search_range(from, &high, query, flags, &match_start, &match_end);
    if (!found && !gtk_text_iter_equal(&from, &low))
        found = search_range(low, &high, query, flags, &match_start, &match_end);
    if (found) {
        gtk_text_buffer_select_range(buffer, &match_start, &match_end);
        mark_set(buffer, MATCH_START, &match_start, TRUE);
        mark_set(buffer, MATCH_END, &match_end, FALSE);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(widget_live(token)), &match_start, 0.1,
                                     FALSE, 0, 0);
    }
    return found;
}

bool simp_gtk_source_view_search(void *self, int64_t token, void *needle, int64_t flags) {
    (void)self;
    char *query = text_copy(needle);
    bool found = source_find(token, query, flags);
    g_free(query);
    return found;
}

bool simp_gtk_source_view_find(void *self, int64_t token, void *needle) {
    return simp_gtk_source_view_search(self, token, needle, 0);
}

bool simp_gtk_source_view_replace(void *self, int64_t token, void *needle, void *replacement,
                                  int64_t flags) {
    (void)self;
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    char *query = text_copy(needle);
    char *text = text_copy(replacement);
    GtkTextIter start, end;
    bool replaced = false;
    if (*query && (selection_matches(buffer, query, flags, &start, &end) ||
                   (source_find(token, query, flags) &&
                    gtk_text_buffer_get_selection_bounds(buffer, &start, &end)))) {
        gtk_text_buffer_begin_user_action(buffer);
        gtk_text_buffer_delete(buffer, &start, &end);
        gtk_text_buffer_insert(buffer, &start, text, -1);
        gtk_text_buffer_end_user_action(buffer);
        replaced = true;
    }
    g_free(text);
    g_free(query);
    return replaced;
}

bool simp_gtk_source_view_replace_next(void *self, int64_t token, void *needle, void *replacement) {
    return simp_gtk_source_view_replace(self, token, needle, replacement, 0);
}

/* Replaces every match in the search range as one undoable action. Matches
 * are found after the previous replacement, so replacement text containing
 * the query is never matched again. */
int64_t simp_gtk_source_view_replace_all(void *self, int64_t token, void *needle,
                                         void *replacement, int64_t flags) {
    (void)self;
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    char *query = text_copy(needle);
    char *text = text_copy(replacement);
    GtkTextIter low, high, match_start, match_end;
    int64_t count = 0;
    if (*query && search_bounds(buffer, flags, &low, &high)) {
        GtkTextMark *position = gtk_text_buffer_create_mark(buffer, NULL, &low, FALSE);
        gtk_text_buffer_begin_user_action(buffer);
        for (;;) {
            GtkTextIter from;
            gtk_text_buffer_get_iter_at_mark(buffer, &from, position);
            if (!search_bounds(buffer, flags, &low, &high) ||
                !search_range(from, &high, query, flags, &match_start, &match_end)) break;
            gtk_text_buffer_delete(buffer, &match_start, &match_end);
            gtk_text_buffer_insert(buffer, &match_start, text, -1);
            gtk_text_buffer_move_mark(buffer, position, &match_start);
            ++count;
        }
        gtk_text_buffer_end_user_action(buffer);
        gtk_text_buffer_delete_mark(buffer, position);
    }
    g_free(text);
    g_free(query);
    return count;
}

/* Captures the current selection as the "in selection" scope. Selecting a
 * Find result does not replace the scope, so Find/Replace can be reopened while
 * a match is selected; with no selection the scope is cleared. */
bool simp_gtk_source_view_capture_scope(void *self, int64_t token) {
    (void)self;
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    GtkTextIter start, end, match_start, match_end, scope_start;
    if (!gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        mark_clear(buffer, SCOPE_START);
        mark_clear(buffer, SCOPE_END);
        return false;
    }
    if (mark_iter(buffer, MATCH_START, &match_start) && mark_iter(buffer, MATCH_END, &match_end) &&
        gtk_text_iter_equal(&start, &match_start) && gtk_text_iter_equal(&end, &match_end))
        return mark_iter(buffer, SCOPE_START, &scope_start);
    mark_set(buffer, SCOPE_START, &start, TRUE);
    mark_set(buffer, SCOPE_END, &end, FALSE);
    return true;
}

bool simp_gtk_source_view_has_scope(void *self, int64_t token) {
    (void)self;
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    GtkTextIter low, high;
    return search_bounds(buffer, SEARCH_IN_SELECTION, &low, &high);
}

void *simp_gtk_source_view_selected_text(void *self, int64_t token) {
    (void)self;
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    GtkTextIter start, end;
    char *text = gtk_text_buffer_get_selection_bounds(buffer, &start, &end)
                     ? gtk_text_buffer_get_text(buffer, &start, &end, TRUE) : g_strdup("");
    void *result = simp_string_new(&simp_string_class_meta, text, strlen(text));
    g_free(text);
    return result;
}

int64_t simp_gtk_source_view_line(void *self, int64_t token) {
    (void)self;
    GtkTextIter cursor;
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
    return gtk_text_iter_get_line(&cursor) + 1;
}

int64_t simp_gtk_source_view_column(void *self, int64_t token) {
    (void)self;
    GtkTextIter cursor;
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(source_buffer(token));
    gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
    return gtk_text_iter_get_line_offset(&cursor) + 1;
}

/* "" selects plain text (no highlighting). Other ids must name a language
 * definition shipped by the sourceview package; see doc/GTK.md. */
bool simp_gtk_source_view_language(void *self, int64_t token, void *language_id) {
    (void)self;
    char *id = text_copy(language_id);
    GtkSourceLanguage *language = NULL;
    if (*id && g_hash_table_contains(registered_languages, id))
        language = gtk_source_language_manager_get_language(source_language_manager, id);
    bool accepted = !*id || language;
    g_free(id);
    if (!accepted) return false;
    GtkSourceBuffer *buffer = source_buffer(token);
    GtkSourceView *view = GTK_SOURCE_VIEW(widget_live(token));
    gtk_source_buffer_set_language(buffer, language);
    gtk_source_buffer_set_highlight_syntax(buffer, language != NULL);
    gtk_source_view_set_show_line_numbers(view, TRUE);
    gtk_source_view_set_auto_indent(view, TRUE);
    gtk_source_view_set_tab_width(view, 4);
    gtk_source_view_set_insert_spaces_instead_of_tabs(view, TRUE);
    return true;
}

void *simp_gtk_source_view_language_id(void *self, int64_t token) {
    (void)self;
    GtkSourceLanguage *language = gtk_source_buffer_get_language(source_buffer(token));
    const char *id = language ? gtk_source_language_get_id(language) : "";
    return simp_string_new(&simp_string_class_meta, id, strlen(id));
}

/* Maps a file name to a registered language id using the globs/mimetypes
 * metadata in the package's .lang files, or "" for plain text. */
void *simp_gtk_source_language_for(void *self, void *path) {
    (void)self;
    require_live();
    char *name = text_copy(path);
    GtkSourceLanguage *language = *name
        ? gtk_source_language_manager_guess_language(source_language_manager, name, NULL) : NULL;
    const char *id = language ? gtk_source_language_get_id(language) : "";
    if (!g_hash_table_contains(registered_languages, id)) id = "";
    void *result = simp_string_new(&simp_string_class_meta, id, strlen(id));
    g_free(name);
    return result;
}

bool simp_gtk_source_is_text(void *self, void *data) {
    (void)self;
    const char *bytes;
    uint64_t length;
    simp_string_bytes(data, &bytes, &length);
    return length == 0 || !memchr(bytes, 0, length);
}

bool simp_gtk_source_view_has_context(void *self, int64_t token, void *context_name,
                                     int64_t offset) {
    (void)self;
    GtkSourceBuffer *buffer = source_buffer(token);
    GtkTextIter start, end, position;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(buffer), &start, &end);
    gtk_source_buffer_ensure_highlight(buffer, &start, &end);
    if (offset < 0 || offset > gtk_text_iter_get_offset(&end))
        fatal("source context offset is out of range");
    gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(buffer), &position, (int)offset);
    char *name = text_copy(context_name);
    gboolean has_context = gtk_source_buffer_iter_has_context_class(buffer, &position, name);
    g_free(name);
    return has_context != FALSE;
}
#endif

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
    if (kind == 4) {
        if (!GTK_IS_NOTEBOOK(w)) fatal("page change requires GtkNotebook");
        return connect_signal(G_OBJECT(w), "switch-page", G_CALLBACK(page_switched),
                              callback, "callback<void()>");
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
int64_t simp_gtk_notebook_signal(void *self, void *source, void *callback) {
    return simp_gtk_widget_signal(self, source_token(source), 4, callback);
}

#include "gtk_config.c"
