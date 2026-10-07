#include "gtk_private.h"
#include <simp/RuntimeGc.h>
#include <simp/Stdlib.h>
#include <stdlib.h>
#include <string.h>

static char *chosen_path;
static int chosen_response;
static int overwrite_response;
static guint attempts;
static guint automation;
static guint selection_attempts;
static gboolean answer_chooser(gpointer unused);

static GtkWindow *editor_window(void) {
    GList *windows = gtk_application_get_windows(
        GTK_APPLICATION(g_application_get_default()));
    for (GList *item = windows; item; item = item->next) {
        if (g_strcmp0(gtk_window_get_title(item->data), "Tweed Lang Editor") == 0)
            return item->data;
    }
    abort();
}

static GtkWidget *find_view(GtkWidget *widget) {
    if (GTK_IS_TEXT_VIEW(widget) && gtk_widget_get_mapped(widget)) return widget;
    for (GtkWidget *child = gtk_widget_get_first_child(widget); child;
         child = gtk_widget_get_next_sibling(child)) {
        GtkWidget *view = find_view(child);
        if (view) return view;
    }
    return NULL;
}

static bool loading(GtkWidget *widget) {
    if (GTK_IS_SPINNER(widget) && gtk_widget_get_mapped(widget) &&
        gtk_spinner_get_spinning(GTK_SPINNER(widget)))
        return true;
    for (GtkWidget *child = gtk_widget_get_first_child(widget); child;
         child = gtk_widget_get_next_sibling(child)) {
        if (loading(child)) return true;
    }
    return false;
}

static gboolean accept_file(gpointer data) {
    GtkDialog *dialog = data;
    if (++selection_attempts > 100) {
        g_printerr("Chooser never finished selecting %s\n", chosen_path);
        abort();
    }
    if (loading(GTK_WIDGET(dialog))) return G_SOURCE_CONTINUE;
    GFile *selected = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
    GFile *expected = g_file_new_for_path(chosen_path);
    bool ready = selected && g_file_equal(selected, expected);
    g_clear_object(&selected);
    g_object_unref(expected);
    if (!ready) return G_SOURCE_CONTINUE;
    gtk_dialog_response(dialog, GTK_RESPONSE_ACCEPT);
    return G_SOURCE_REMOVE;
}

static gboolean answer_overwrite(gpointer unused) {
    (void)unused;
    if (++attempts > 200) abort();
    GListModel *windows = gtk_window_get_toplevels();
    for (guint i = 0; i < g_list_model_get_n_items(windows); ++i) {
        GtkWindow *window = g_list_model_get_item(windows, i);
        if (GTK_IS_MESSAGE_DIALOG(window) && gtk_widget_get_visible(GTK_WIDGET(window))) {
            bool native_confirmation = gtk_dialog_get_widget_for_response(GTK_DIALOG(window), 0) != NULL;
            bool rejected = overwrite_response == GTK_RESPONSE_CANCEL;
            if (native_confirmation)
                overwrite_response = overwrite_response == GTK_RESPONSE_ACCEPT ? 1 : 0;
            gtk_dialog_response(GTK_DIALOG(window), overwrite_response);
            g_object_unref(window);
            if (native_confirmation && rejected) {
                chosen_response = GTK_RESPONSE_CANCEL;
                overwrite_response = 0;
                attempts = 0;
                automation = g_timeout_add(500, answer_chooser, NULL);
            } else {
                automation = 0;
            }
            return G_SOURCE_REMOVE;
        }
        g_object_unref(window);
    }
    return G_SOURCE_CONTINUE;
}

static gboolean answer_chooser(gpointer unused) {
    (void)unused;
    if (++attempts > 200) abort();
    GListModel *windows = gtk_window_get_toplevels();
    for (guint i = 0; i < g_list_model_get_n_items(windows); ++i) {
        GtkWindow *window = g_list_model_get_item(windows, i);
        if (!GTK_IS_FILE_CHOOSER(window) || !gtk_widget_get_visible(GTK_WIDGET(window))) {
            g_object_unref(window);
            continue;
        }
        if (chosen_response == GTK_RESPONSE_CANCEL) {
            gtk_dialog_response(GTK_DIALOG(window), GTK_RESPONSE_CANCEL);
        } else {
            GtkFileChooser *chooser = GTK_FILE_CHOOSER(window);
            GError *error = NULL;
            if (gtk_file_chooser_get_action(chooser) == GTK_FILE_CHOOSER_ACTION_SAVE) {
                char *folder = g_path_get_dirname(chosen_path);
                char *name = g_path_get_basename(chosen_path);
                GFile *file = g_file_new_for_path(folder);
                GFile *current = gtk_file_chooser_get_current_folder(chooser);
                if ((!current || !g_file_equal(current, file)) &&
                    !gtk_file_chooser_set_current_folder(chooser, file, &error)) abort();
                g_clear_object(&current);
                gtk_file_chooser_set_current_name(chooser, name);
                g_object_unref(file);
                g_free(folder);
                g_free(name);
            } else {
                GFile *file = g_file_new_for_path(chosen_path);
                if (!gtk_file_chooser_set_file(chooser, file, &error)) abort();
                g_object_unref(file);
            }
            if (error) abort();
            selection_attempts = 0;
            g_timeout_add_full(G_PRIORITY_DEFAULT, 500, accept_file,
                               g_object_ref(window), g_object_unref);
            if (overwrite_response) {
                attempts = 0;
                automation = g_timeout_add(25, answer_overwrite, NULL);
                g_object_unref(window);
                return G_SOURCE_REMOVE;
            }
        }
        g_object_unref(window);
        automation = 0;
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

void fixture_editor_choose(void *self, void *path, int64_t response, int64_t overwrite) {
    (void)self;
    simp_gtk_require_owner();
    if (automation) abort();
    const char *bytes;
    uint64_t length;
    simp_string_bytes(path, &bytes, &length);
    g_free(chosen_path);
    chosen_path = g_strndup(bytes, length);
    chosen_response = response ? GTK_RESPONSE_ACCEPT : GTK_RESPONSE_CANCEL;
    overwrite_response = overwrite == 0 ? 0 :
                         overwrite > 0 ? GTK_RESPONSE_ACCEPT : GTK_RESPONSE_CANCEL;
    attempts = 0;
    automation = g_timeout_add(500, answer_chooser, NULL);
}

void fixture_editor_collect(void *self) {
    (void)self;
    simp_gc_collect();
}

void fixture_editor_edit(void *self, void *text) {
    (void)self;
    simp_gtk_require_owner();
    const char *bytes;
    uint64_t length;
    simp_string_bytes(text, &bytes, &length);
    GtkWidget *view = find_view(GTK_WIDGET(editor_window()));
    if (!view || length > G_MAXINT) abort();
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gtk_text_buffer_begin_user_action(buffer);
    gtk_text_buffer_delete(buffer, &start, &end);
    gtk_text_buffer_insert(buffer, &start, bytes, (int)length);
    gtk_text_buffer_end_user_action(buffer);
}

bool fixture_editor_shortcut(void *self, void *trigger) {
    (void)self;
    simp_gtk_require_owner();
    const char *bytes;
    uint64_t length;
    simp_string_bytes(trigger, &bytes, &length);
    char *text = g_strndup(bytes, length);
    GtkShortcutTrigger *requested = gtk_shortcut_trigger_parse_string(text);
    g_free(text);
    if (!requested) abort();
    GtkWidget *view = find_view(GTK_WIDGET(editor_window()));
    if (!view) abort();
    GListModel *controllers = gtk_widget_observe_controllers(view);
    bool activated = false;
    for (guint i = 0; i < g_list_model_get_n_items(controllers); ++i) {
        GtkEventController *controller = g_list_model_get_item(controllers, i);
        if (GTK_IS_SHORTCUT_CONTROLLER(controller)) {
            GListModel *shortcuts = G_LIST_MODEL(controller);
            for (guint j = 0; j < g_list_model_get_n_items(shortcuts); ++j) {
                GtkShortcut *shortcut = g_list_model_get_item(shortcuts, j);
                if (GTK_IS_CALLBACK_ACTION(gtk_shortcut_get_action(shortcut)) &&
                    gtk_shortcut_trigger_equal(gtk_shortcut_get_trigger(shortcut), requested))
                    activated = gtk_shortcut_action_activate(
                        gtk_shortcut_get_action(shortcut), GTK_SHORTCUT_ACTION_EXCLUSIVE,
                        view, gtk_shortcut_get_arguments(shortcut));
                g_object_unref(shortcut);
            }
        }
        g_object_unref(controller);
    }
    g_object_unref(controllers);
    g_object_unref(requested);
    return activated;
}

int64_t fixture_editor_height(void *self) {
    (void)self;
    simp_gtk_require_owner();
    GtkWidget *view = find_view(GTK_WIDGET(editor_window()));
    if (!view) abort();
    return gtk_widget_get_height(view);
}

void fixture_editor_resize(void *self) {
    (void)self;
    simp_gtk_require_owner();
    gtk_window_set_default_size(editor_window(), 1000, 750);
}

bool fixture_editor_entry_focus(void *self, void *text) {
    (void)self;
    simp_gtk_require_owner();
    const char *bytes;
    uint64_t length;
    simp_string_bytes(text, &bytes, &length);
    GtkWidget *focus = gtk_root_get_focus(GTK_ROOT(editor_window()));
    for (GtkWidget *widget = focus; widget; widget = gtk_widget_get_parent(widget)) {
        if (GTK_IS_ENTRY(widget)) {
            const char *actual = gtk_editable_get_text(GTK_EDITABLE(widget));
            return strlen(actual) == length && memcmp(actual, bytes, length) == 0;
        }
    }
    return false;
}

void fixture_editor_cleanup(void *self) {
    (void)self;
    if (automation) abort();
    g_clear_pointer(&chosen_path, g_free);
}
