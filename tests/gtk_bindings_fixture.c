/* Test-only access to private records; all chooser responses use real GTK
 * objects. No alternate chooser or test entrypoints enter the package. */
#include "../stdlib/gtk/0.1.0/native/gtk.c"
#include <assert.h>

static int64_t finalized_dialogs;
static int64_t released_receivers;

void binding_released(void *self) {
    (void)self;
    ++released_receivers;
}

int64_t binding_receivers(void *self) {
    (void)self;
    return released_receivers;
}

static void fixture_finalized(gpointer data, GObject *object) {
    (void)data;
    (void)object;
    ++finalized_dialogs;
}

void binding_collect(void *self) {
    (void)self;
    simp_gc_collect();
}

int64_t binding_dialogs(void *self) {
    (void)self;
    int64_t count = 0;
    for (FileDialog *dialog = dialogs; dialog; dialog = dialog->next) ++count;
    return count;
}

int64_t binding_finalized(void *self) {
    (void)self;
    return finalized_dialogs;
}

void binding_track(void *self) {
    (void)self;
    assert(dialogs && dialogs->chooser);
    g_object_weak_ref(G_OBJECT(dialogs->chooser), fixture_finalized, NULL);
}

typedef struct Response {
    char *path;
    int response;
    unsigned tries;
} Response;

static GtkDialog *fallback_chooser(void) {
    GListModel *windows = gtk_window_get_toplevels();
    for (guint i = 0; i < g_list_model_get_n_items(windows); ++i) {
        GtkWindow *window = g_list_model_get_item(windows, i);
        if (GTK_IS_FILE_CHOOSER_DIALOG(window) && gtk_widget_get_visible(GTK_WIDGET(window)))
            return GTK_DIALOG(window);
        g_object_unref(window);
    }
    assert(!"native chooser fallback was not presented");
    return NULL;
}

static gboolean respond_when_selected(gpointer data) {
    Response *request = data;
    int acquired = simp_runtime_managed_enter();
    assert(dialogs && dialogs->chooser);
    GtkDialog *chooser = fallback_chooser();
    if (request->tries == 20 && request->response == GTK_RESPONSE_ACCEPT) {
        GFile *selected = g_file_new_for_path(request->path);
        if (gtk_file_chooser_get_action(GTK_FILE_CHOOSER(chooser)) == GTK_FILE_CHOOSER_ACTION_SAVE) {
            GFile *parent = g_file_get_parent(selected);
            assert(gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), parent, NULL));
            char *name = g_file_get_basename(selected);
            gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser), name);
            g_free(name);
            g_object_unref(parent);
        } else assert(gtk_file_chooser_set_file(GTK_FILE_CHOOSER(chooser), selected, NULL));
        g_object_unref(selected);
    }
    GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(chooser));
    char *path = file ? g_file_get_path(file) : NULL;
    bool ready = request->response == GTK_RESPONSE_CANCEL ||
                 g_strcmp0(path, request->path) == 0;
    /* Let GTK's mapped fallback finish its asynchronous directory/pathbar
     * loading before synthesizing a user click. */
    ready = ready && request->tries >= 40;
    if (request->tries == 98)
        fprintf(stderr, "chooser selected %s, expected %s (save %d)\n",
                path ? path : "(none)", request->path,
                gtk_file_chooser_get_action(GTK_FILE_CHOOSER(chooser)) == GTK_FILE_CHOOSER_ACTION_SAVE);
    g_clear_object(&file);
    g_free(path);
    assert(++request->tries < 100);
    if (ready) {
        gtk_dialog_response(chooser, request->response);
        g_free(request->path);
        free(request);
    }
    g_object_unref(chooser);
    simp_runtime_managed_leave(acquired);
    return ready ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
}

void binding_respond(void *self, void *path_text, bool accept) {
    (void)self;
    assert(dialogs && dialogs->chooser);
    Response *request = calloc(1, sizeof(*request));
    request->path = text_copy(path_text);
    request->response = accept ? GTK_RESPONSE_ACCEPT : GTK_RESPONSE_CANCEL;
    g_timeout_add(25, respond_when_selected, request);
}

typedef struct Confirm {
    bool accept;
    unsigned tries;
} Confirm;

static gboolean confirm_when_ready(gpointer data) {
    Confirm *request = data;
    int acquired = simp_runtime_managed_enter();
    assert(++request->tries < 100);
    bool ready = false;
    GListModel *windows = gtk_window_get_toplevels();
    for (guint i = 0; i < g_list_model_get_n_items(windows); ++i) {
        GtkWindow *window = g_list_model_get_item(windows, i);
        if (GTK_IS_MESSAGE_DIALOG(window) && gtk_widget_get_visible(GTK_WIDGET(window))) {
            ready = true;
            g_object_weak_ref(G_OBJECT(window), fixture_finalized, NULL);
            bool alert = gtk_dialog_get_widget_for_response(GTK_DIALOG(window), 1) != NULL;
            gtk_dialog_response(GTK_DIALOG(window),
                                alert ? (request->accept ? 1 : 0) :
                                (request->accept ? GTK_RESPONSE_ACCEPT : GTK_RESPONSE_CANCEL));
            if (!request->accept) {
                Response *cancel = calloc(1, sizeof(*cancel));
                cancel->path = g_strdup("");
                cancel->response = GTK_RESPONSE_CANCEL;
                g_timeout_add(25, respond_when_selected, cancel);
            }
        }
        g_object_unref(window);
        if (ready) break;
    }
    if (ready) free(request);
    simp_runtime_managed_leave(acquired);
    return ready ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
}

void binding_confirm(void *self, bool accept) {
    (void)self;
    Confirm *request = calloc(1, sizeof(*request));
    request->accept = accept;
    g_timeout_add(25, confirm_when_ready, request);
}

typedef struct SizeRequest {
    int64_t token;
    int width;
    int height;
    unsigned tries;
    SimpCallbackContext *context;
} SizeRequest;

static gboolean allocation_ready(gpointer data) {
    SizeRequest *request = data;
    int acquired = simp_runtime_managed_enter();
    GtkWidget *widget = widget_live(request->token);
    bool ready = gtk_widget_get_width(widget) >= request->width &&
                 gtk_widget_get_height(widget) >= request->height;
    assert(++request->tries < 200);
    if (ready) {
        typedef void (*Adapter)(SimpCallbackContext *);
        ((Adapter)simp_callback_adapter(request->context))(request->context);
        simp_callback_release(request->context);
        simp_callback_dispose(request->context);
        free(request);
    }
    simp_runtime_managed_leave(acquired);
    return ready ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
}

void binding_wait_size(void *self, int64_t token, int64_t width, int64_t height,
                       void *callback) {
    (void)self;
    SizeRequest *request = calloc(1, sizeof(*request));
    request->token = token;
    request->width = (int)width;
    request->height = (int)height;
    request->context = simp_callback_acquire(callback, "callback<void()>");
    g_timeout_add(25, allocation_ready, request);
}

void binding_assert_menu(void *self, int64_t token) {
    (void)self;
    WidgetRecord *record = menu_record(token);
    assert(GTK_IS_POPOVER_MENU_BAR(record->widget));
    assert(G_IS_MENU(record->menu) && G_IS_SIMPLE_ACTION_GROUP(record->actions));
    assert(g_menu_model_get_n_items(G_MENU_MODEL(record->menu)) == 1);
}

void binding_resize(void *self, int64_t token, int64_t width, int64_t height) {
    (void)self;
    gtk_window_set_default_size(GTK_WINDOW(widget_live(token)), (int)width, (int)height);
}
