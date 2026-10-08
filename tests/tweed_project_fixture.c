#include "project_explorer_fixture.c"

static char *fixture_text(void *value) {
    const char *bytes;
    uint64_t length;
    simp_string_bytes(value, &bytes, &length);
    return g_strndup(bytes, length);
}

static GtkWindow *titled_window(const char *title) {
    GListModel *windows = gtk_window_get_toplevels();
    for (guint i = 0; i < g_list_model_get_n_items(windows); ++i) {
        GtkWindow *window = g_list_model_get_item(windows, i);
        g_object_unref(window);
        if (g_strcmp0(gtk_window_get_title(window), title) == 0) return window;
    }
    return NULL;
}

static GtkWidget *mapped_button(GtkWidget *widget, const char *label) {
    if (GTK_IS_BUTTON(widget) && gtk_widget_get_mapped(widget) &&
        gtk_widget_is_sensitive(widget) &&
        g_strcmp0(gtk_button_get_label(GTK_BUTTON(widget)), label) == 0)
        return widget;
    for (GtkWidget *child = gtk_widget_get_first_child(widget); child;
         child = gtk_widget_get_next_sibling(child)) {
        GtkWidget *found = mapped_button(child, label);
        if (found) return found;
    }
    return NULL;
}

/* Clicks a mapped, sensitive button labelled label inside the window titled title. */
bool project_click(void *self, void *title_text, void *label_text) {
    (void)self;
    simp_gtk_require_owner();
    char *title = fixture_text(title_text);
    char *label = fixture_text(label_text);
    GtkWindow *window = titled_window(title);
    GtkWidget *button = window ? mapped_button(GTK_WIDGET(window), label) : NULL;
    if (button) g_signal_emit_by_name(button, "clicked");
    g_free(title);
    g_free(label);
    return button != NULL;
}

/* A visible, non-modal window parented to the editor. */
bool project_window_visible(void *self, void *title_text) {
    (void)self;
    simp_gtk_require_owner();
    char *title = fixture_text(title_text);
    GtkWindow *window = titled_window(title);
    g_free(title);
    return window && gtk_widget_get_visible(GTK_WIDGET(window)) &&
           gtk_window_get_transient_for(window) == editor_window();
}

bool project_window_has(void *self, void *title_text, void *fragment_text) {
    (void)self;
    simp_gtk_require_owner();
    char *title = fixture_text(title_text);
    char *fragment = fixture_text(fragment_text);
    GtkWindow *window = titled_window(title);
    bool found = false;
    if (window) {
        GString *text = g_string_new(NULL);
        label_text(GTK_WIDGET(window), text);
        found = strstr(text->str, fragment) != NULL;
        if (!found) g_printerr("window text: %s\n", text->str);
        g_string_free(text, TRUE);
    }
    g_free(title);
    g_free(fragment);
    return found;
}

int64_t project_tab_width(void *self) {
    (void)self;
    simp_gtk_require_owner();
    GtkWidget *view = find_view(GTK_WIDGET(editor_window()));
    if (!view) return -1;
    return gtk_source_view_get_tab_width(GTK_SOURCE_VIEW(view));
}
