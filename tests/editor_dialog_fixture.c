#include "gtk_private.h"
#include <simp/RuntimeGc.h>
#include <simp/Stdlib.h>
#include <gtksourceview/gtksource.h>
#include <stdlib.h>
#include <string.h>

static char *chosen_path;
static int chosen_response;
static int overwrite_response;
static guint attempts;
static guint automation;
static guint selection_attempts;
static char *many_folder;
static guint many_count;
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

static GtkWidget *find_type(GtkWidget *widget, GType type) {
    if (G_TYPE_CHECK_INSTANCE_TYPE(widget, type)) return widget;
    for (GtkWidget *child = gtk_widget_get_first_child(widget); child;
         child = gtk_widget_get_next_sibling(child)) {
        GtkWidget *found = find_type(child, type);
        if (found) return found;
    }
    return NULL;
}

static GtkWindow *search_window(void) {
    GListModel *windows = gtk_window_get_toplevels();
    GtkWindow *found = NULL;
    for (guint i = 0; i < g_list_model_get_n_items(windows); ++i) {
        GtkWindow *window = g_list_model_get_item(windows, i);
        const char *title = gtk_window_get_title(window);
        if ((g_strcmp0(title, "Find") == 0 ||
             g_strcmp0(title, "Find and Replace") == 0) &&
            gtk_window_get_transient_for(window) == editor_window()) {
            found = window;
            break;
        }
        g_object_unref(window);
    }
    return found;
}

static GtkWidget *find_button(GtkWidget *widget, const char *label) {
    if (GTK_IS_BUTTON(widget) &&
        g_strcmp0(gtk_button_get_label(GTK_BUTTON(widget)), label) == 0)
        return widget;
    for (GtkWidget *child = gtk_widget_get_first_child(widget); child;
         child = gtk_widget_get_next_sibling(child)) {
        GtkWidget *found = find_button(child, label);
        if (found) return found;
    }
    return NULL;
}

/* Multi-select: select every entry of the chooser's file list, then accept. */
static gboolean accept_many(gpointer data) {
    GtkDialog *dialog = data;
    if (++selection_attempts > 100) {
        g_printerr("Chooser never listed %u files in %s\n", many_count, many_folder);
        abort();
    }
    if (loading(GTK_WIDGET(dialog))) return G_SOURCE_CONTINUE;
    GtkWidget *list = find_type(GTK_WIDGET(dialog), GTK_TYPE_COLUMN_VIEW);
    if (!list) return G_SOURCE_CONTINUE;
    GtkSelectionModel *model = gtk_column_view_get_model(GTK_COLUMN_VIEW(list));
    if (!model || g_list_model_get_n_items(G_LIST_MODEL(model)) < many_count)
        return G_SOURCE_CONTINUE;
    gtk_selection_model_select_all(model);
    GListModel *files = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
    guint selected = g_list_model_get_n_items(files);
    g_object_unref(files);
    if (selected != many_count) return G_SOURCE_CONTINUE;
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
        } else if (many_folder) {
            GtkFileChooser *chooser = GTK_FILE_CHOOSER(window);
            if (!gtk_file_chooser_get_select_multiple(chooser)) abort();
            GFile *folder = g_file_new_for_path(many_folder);
            if (!gtk_file_chooser_set_current_folder(chooser, folder, NULL)) abort();
            g_object_unref(folder);
            selection_attempts = 0;
            g_timeout_add_full(G_PRIORITY_DEFAULT, 500, accept_many,
                               g_object_ref(window), g_object_unref);
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
    g_clear_pointer(&many_folder, g_free);
    chosen_response = response ? GTK_RESPONSE_ACCEPT : GTK_RESPONSE_CANCEL;
    overwrite_response = overwrite == 0 ? 0 :
                         overwrite > 0 ? GTK_RESPONSE_ACCEPT : GTK_RESPONSE_CANCEL;
    attempts = 0;
    automation = g_timeout_add(500, answer_chooser, NULL);
}

void fixture_editor_choose_many(void *self, void *folder, int64_t count) {
    (void)self;
    simp_gtk_require_owner();
    if (automation) abort();
    const char *bytes;
    uint64_t length;
    simp_string_bytes(folder, &bytes, &length);
    g_free(many_folder);
    many_folder = g_strndup(bytes, length);
    many_count = (guint)count;
    chosen_response = GTK_RESPONSE_ACCEPT;
    overwrite_response = 0;
    attempts = 0;
    automation = g_timeout_add(500, answer_chooser, NULL);
}

static void label_text(GtkWidget *widget, GString *text) {
    if (GTK_IS_LABEL(widget)) {
        g_string_append(text, gtk_label_get_text(GTK_LABEL(widget)));
        g_string_append_c(text, '\n');
    }
    for (GtkWidget *child = gtk_widget_get_first_child(widget); child;
         child = gtk_widget_get_next_sibling(child))
        label_text(child, text);
}

/* Finds a visible alert transient for the editor whose text contains every
 * expected fragment, dismisses it with its button, and reports whether it was
 * found. */
bool fixture_editor_alert(void *self, void *expected) {
    (void)self;
    simp_gtk_require_owner();
    const char *bytes;
    uint64_t length;
    simp_string_bytes(expected, &bytes, &length);
    char *fragments_text = g_strndup(bytes, length);
    char **fragments = g_strsplit(fragments_text, "|", -1);
    GtkWindow *editor = editor_window();
    GListModel *windows = gtk_window_get_toplevels();
    bool found = false;
    for (guint i = 0; i < g_list_model_get_n_items(windows) && !found; ++i) {
        GtkWindow *window = g_list_model_get_item(windows, i);
        if (window != editor && !GTK_IS_FILE_CHOOSER(window) &&
            gtk_window_get_transient_for(window) == editor &&
            gtk_widget_get_visible(GTK_WIDGET(window)) && gtk_window_get_modal(window)) {
            GString *text = g_string_new(NULL);
            label_text(GTK_WIDGET(window), text);
            found = true;
            for (char **fragment = fragments; *fragment; ++fragment)
                if (!strstr(text->str, *fragment)) found = false;
            if (!found) g_printerr("alert text: %s\n", text->str);
            g_string_free(text, TRUE);
            GtkWidget *button = find_type(GTK_WIDGET(window), GTK_TYPE_BUTTON);
            if (found && button) g_signal_emit_by_name(button, "clicked");
            else if (found) gtk_window_destroy(window);
        }
        g_object_unref(window);
    }
    g_strfreev(fragments);
    g_free(fragments_text);
    return found;
}

bool fixture_editor_alert_open(void *self) {
    (void)self;
    simp_gtk_require_owner();
    GtkWindow *editor = editor_window();
    GListModel *windows = gtk_window_get_toplevels();
    bool open = false;
    for (guint i = 0; i < g_list_model_get_n_items(windows); ++i) {
        GtkWindow *window = g_list_model_get_item(windows, i);
        if (window != editor && gtk_window_get_transient_for(window) == editor &&
            gtk_widget_get_visible(GTK_WIDGET(window)) && gtk_window_get_modal(window))
            open = true;
        g_object_unref(window);
    }
    return open;
}

void fixture_editor_select(void *self, int64_t start, int64_t end) {
    (void)self;
    simp_gtk_require_owner();
    GtkWidget *view = find_view(GTK_WIDGET(editor_window()));
    if (!view) abort();
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    GtkTextIter from, to;
    gtk_text_buffer_get_iter_at_offset(buffer, &from, (int)start);
    gtk_text_buffer_get_iter_at_offset(buffer, &to, (int)end);
    gtk_text_buffer_select_range(buffer, &from, &to);
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
    GListModel *windows = gtk_window_get_toplevels();
    for (guint i = 0; i < g_list_model_get_n_items(windows); ++i) {
        GtkWindow *window = g_list_model_get_item(windows, i);
        GtkWidget *focus = gtk_window_get_focus(window);
        for (GtkWidget *widget = focus; widget; widget = gtk_widget_get_parent(widget)) {
            if (GTK_IS_ENTRY(widget)) {
                const char *actual = gtk_editable_get_text(GTK_EDITABLE(widget));
                bool found = strlen(actual) == length && memcmp(actual, bytes, length) == 0;
                g_object_unref(window);
                return found;
            }
        }
        g_object_unref(window);
    }
    return false;
}

bool fixture_editor_search_dialog(void *self, bool replace_mode) {
    (void)self;
    simp_gtk_require_owner();
    GtkWindow *dialog = search_window();
    if (!dialog) return false;
    const char *title = replace_mode ? "Find and Replace" : "Find";
    GtkWidget *root = gtk_window_get_child(dialog);
    GtkWidget *replace = find_button(root, "Replace all");
    GtkWidget *replace_row = replace ? gtk_widget_get_parent(replace) : NULL;
    bool matches = gtk_widget_get_visible(GTK_WIDGET(dialog)) &&
        g_strcmp0(gtk_window_get_title(dialog), title) == 0 &&
        replace_row && (gtk_widget_get_visible(replace_row) == replace_mode);
    g_object_unref(dialog);
    return matches;
}

bool fixture_editor_close_search_dialog(void *self) {
    (void)self;
    simp_gtk_require_owner();
    GtkWindow *dialog = search_window();
    if (!dialog) return false;
    gtk_window_close(dialog);
    bool closed = !gtk_widget_get_visible(GTK_WIDGET(dialog));
    g_object_unref(dialog);
    return closed;
}

bool fixture_editor_monospace(void *self) {
    (void)self;
    simp_gtk_require_owner();
    GtkWidget *view = find_view(GTK_WIDGET(editor_window()));
    return view && GTK_SOURCE_IS_VIEW(view) &&
        gtk_text_view_get_monospace(GTK_TEXT_VIEW(view));
}

void fixture_editor_cleanup(void *self) {
    (void)self;
    if (automation) abort();
    g_clear_pointer(&chosen_path, g_free);
    g_clear_pointer(&many_folder, g_free);
}
