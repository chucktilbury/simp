/* Included by gtk.c to reuse its callback ownership and runtime boundary. */
#include "toml.h"
#include <errno.h>
#include <glib/gstdio.h>
#include <pango/pangocairo.h>

void *simp_gtk_config_file_status(void *self, void *path_text) {
    (void)self;
    char *path = text_copy(path_text);
    GStatBuf info;
    int status = g_stat(path, &info);
    int code = errno;
    const char *message = "";
    if (status != 0) {
        if (code == ENOENT) {
            message = g_file_test(path, G_FILE_TEST_IS_SYMLINK) ? "Broken symbolic link" : "missing";
        } else message = g_strerror(code);
    }
    void *result = simp_string_new(&simp_string_class_meta, message, strlen(message));
    g_free(path);
    return result;
}

static void *config_string(const char *text) {
    return simp_string_new(&simp_string_class_meta, text, strlen(text));
}

static bool config_raw_valid(const char *raw) {
    if (*raw == '"' || *raw == '\'') {
        if (*raw == '"') {
            for (const char *p = raw; *p; ++p) {
                if (*p != '\\' || !p[1]) continue;
                ++p;
                if (*p == 'u' && strncmp(p + 1, "0000", 4) == 0) return false;
                if (*p == 'U' && strncmp(p + 1, "00000000", 8) == 0) return false;
            }
        }
        char *string = NULL;
        bool valid = toml_rtos(raw, &string) == 0;
        free(string);
        return valid;
    }
    toml_timestamp_t timestamp;
    return toml_rtob(raw, NULL) == 0 || toml_rtoi(raw, NULL) == 0 ||
           toml_rtod(raw, NULL) == 0 || toml_rtots(raw, &timestamp) == 0;
}

static bool config_table_valid(const toml_table_t *table);
static bool config_array_valid(const toml_array_t *array) {
    for (int i = 0; i < toml_array_nelem(array); ++i) {
        const char *raw = toml_raw_at(array, i);
        if (raw && !config_raw_valid(raw)) return false;
        if (toml_array_at(array, i) && !config_array_valid(toml_array_at(array, i))) return false;
        if (toml_table_at(array, i) && !config_table_valid(toml_table_at(array, i))) return false;
    }
    return true;
}

static bool config_table_valid(const toml_table_t *table) {
    for (int i = 0; toml_key_in(table, i); ++i) {
        const char *key = toml_key_in(table, i);
        const char *raw = toml_raw_in(table, key);
        if (raw && !config_raw_valid(raw)) return false;
        if (toml_array_in(table, key) && !config_array_valid(toml_array_in(table, key))) return false;
        if (toml_table_in(table, key) && !config_table_valid(toml_table_in(table, key))) return false;
    }
    return true;
}

static toml_table_t *config_parse(void *text, char error[256]) {
    char *bytes = text_copy(text);
    toml_table_t *table = toml_parse(bytes, error, 256);
    g_free(bytes);
    if (table && !config_table_valid(table)) {
        toml_free(table);
        table = NULL;
        g_strlcpy(error, "Invalid TOML scalar (including unsupported NUL escape)", 256);
    }
    return table;
}

void *simp_gtk_config_validate(void *self, void *text) {
    (void)self;
    char error[256] = "";
    toml_table_t *table = config_parse(text, error);
    toml_free(table);
    return config_string(error);
}

static toml_table_t *config_section(toml_table_t *root, const char *section) {
    return *section ? toml_table_in(root, section) : root;
}

/* Tagged values keep conversion/type validation in the Simple model. */
void *simp_gtk_config_value(void *self, void *text, void *section_text, void *key_text) {
    (void)self;
    char error[256] = "";
    toml_table_t *root = config_parse(text, error);
    char *section = text_copy(section_text), *key = text_copy(key_text);
    toml_table_t *table = root ? config_section(root, section) : NULL;
    GString *value = g_string_new("");
    if (table && toml_key_exists(table, key)) {
        toml_datum_t s = toml_string_in(table, key);
        toml_datum_t i = toml_int_in(table, key);
        toml_datum_t b = toml_bool_in(table, key);
        if (s.ok) {
            g_string_append_c(value, 's');
            g_string_append(value, s.u.s);
            free(s.u.s);
        } else if (i.ok) g_string_append_printf(value, "i%" G_GINT64_FORMAT, i.u.i);
        else if (b.ok) g_string_append(value, b.u.b ? "btrue" : "bfalse");
        else g_string_append(value, toml_table_in(table, key) ? "t" : "x");
    }
    void *result = config_string(value->str);
    g_string_free(value, TRUE);
    g_free(section);
    g_free(key);
    toml_free(root);
    return result;
}

void *simp_gtk_config_key(void *self, void *text, void *section_text, int64_t index) {
    (void)self;
    char error[256] = "";
    toml_table_t *root = config_parse(text, error);
    char *section = text_copy(section_text);
    toml_table_t *table = root ? config_section(root, section) : NULL;
    const char *key = table && index >= 0 && index <= INT_MAX ? toml_key_in(table, (int)index) : NULL;
    void *result = config_string(key ? key : "");
    g_free(section);
    toml_free(root);
    return result;
}

static void config_quote(GString *out, const char *text) {
    g_string_append_c(out, '"');
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (*p == '\\' || *p == '"') g_string_append_c(out, '\\');
        if (*p < 32 || *p == 127) g_string_append_printf(out, "\\u%04x", *p);
        else g_string_append_c(out, *p);
    }
    g_string_append_c(out, '"');
}

void *simp_gtk_config_quote(void *self, void *text) {
    (void)self;
    char *bytes = text_copy(text);
    GString *out = g_string_new("");
    config_quote(out, bytes);
    void *result = config_string(out->str);
    g_string_free(out, TRUE);
    g_free(bytes);
    return result;
}

static void config_table(GString *out, const toml_table_t *base, const toml_table_t *updates);

static void config_array(GString *out, const toml_array_t *array) {
    g_string_append_c(out, '[');
    for (int i = 0; i < toml_array_nelem(array); ++i) {
        if (i) g_string_append(out, ", ");
        const char *raw = toml_raw_at(array, i);
        if (raw) g_string_append(out, raw);
        else if (toml_array_at(array, i)) config_array(out, toml_array_at(array, i));
        else config_table(out, toml_table_at(array, i), NULL);
    }
    g_string_append_c(out, ']');
}

static void config_value(GString *out, const toml_table_t *base,
                         const toml_table_t *updates, const char *key) {
    const toml_table_t *source = updates && toml_key_exists(updates, key) ? updates : base;
    const char *raw = toml_raw_in(source, key);
    if (raw) g_string_append(out, raw);
    else if (toml_array_in(source, key)) config_array(out, toml_array_in(source, key));
    else config_table(out, base ? toml_table_in(base, key) : NULL,
                      updates ? toml_table_in(updates, key) : NULL);
}

static void config_table(GString *out, const toml_table_t *base, const toml_table_t *updates) {
    g_string_append_c(out, '{');
    bool first = true;
    for (int pass = 0; pass < 2; ++pass) {
        const toml_table_t *table = pass ? updates : base;
        for (int i = 0; table && toml_key_in(table, i); ++i) {
            const char *key = toml_key_in(table, i);
            if (pass && base && toml_key_exists(base, key)) continue;
            if (!first) g_string_append(out, ", ");
            first = false;
            config_quote(out, key);
            g_string_append(out, " = ");
            config_value(out, base, updates, key);
        }
    }
    g_string_append_c(out, '}');
}

void *simp_gtk_config_merge(void *self, void *original, void *changes) {
    (void)self;
    char error[256] = "";
    toml_table_t *base = config_parse(original, error);
    toml_table_t *updates = config_parse(changes, error);
    if (!base || !updates) fatal("merge requires validated TOML");
    GString *out = g_string_new("");
    /* Scalars first, then readable top-level tables. Nested values are inline. */
    for (int tables = 0; tables < 2; ++tables) {
        for (int pass = 0; pass < 2; ++pass) {
            toml_table_t *table = pass ? updates : base;
            for (int i = 0; toml_key_in(table, i); ++i) {
                const char *key = toml_key_in(table, i);
                if (pass && toml_key_exists(base, key)) continue;
                const toml_table_t *source = toml_key_exists(updates, key) ? updates : base;
                bool is_table = toml_table_in(source, key) != NULL;
                if (is_table != (tables != 0)) continue;
                if (is_table) {
                    g_string_append(out, "\n[");
                    config_quote(out, key);
                    g_string_append(out, "]\n");
                    const toml_table_t *b = toml_table_in(base, key), *u = toml_table_in(updates, key);
                    for (int p = 0; p < 2; ++p) {
                        const toml_table_t *t = p ? u : b;
                        for (int j = 0; t && toml_key_in(t, j); ++j) {
                            const char *k = toml_key_in(t, j);
                            if (p && b && toml_key_exists(b, k)) continue;
                            config_quote(out, k);
                            g_string_append(out, " = ");
                            config_value(out, b, u, k);
                            g_string_append_c(out, '\n');
                        }
                    }
                } else {
                    config_quote(out, key);
                    g_string_append(out, " = ");
                    config_value(out, base, updates, key);
                    g_string_append_c(out, '\n');
                }
            }
        }
    }
    void *result = config_string(out->str);
    g_string_free(out, TRUE);
    toml_free(base);
    toml_free(updates);
    return result;
}

typedef struct ConfigSave {
    struct ConfigSave *next;
    SimpCallbackContext *context;
    char *contents;
    GFile *file;
    char *path;
    char *expected;
} ConfigSave;
static ConfigSave *config_saves;

static void config_complete(ConfigSave *save, GError *error) {
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    /* Unlink before invoking: the callback may shut down and drain other saves. */
    ConfigSave **link = &config_saves;
    while (*link != save) link = &(*link)->next;
    *link = save->next;
    if (!stopped) {
        void *message = config_string(error ? error->message : "");
        void *slots[] = { &message };
        SimpRootFrame frame = {0};
        simp_gc_push_or_abort(&frame, slots, 1);
        typedef void (*Adapter)(SimpCallbackContext *, void *);
        ((Adapter)simp_callback_adapter(save->context))(save->context, message);
        simp_gc_pop_or_abort(&frame);
    } else if (error) fprintf(stderr, "Settings save failed: %s: %s\n", save->path, error->message);
    simp_callback_release(save->context);
    simp_callback_dispose(save->context);
    g_clear_error(&error);
    g_object_unref(save->file);
    g_free(save->contents);
    g_free(save->path);
    g_free(save->expected);
    free(save);
    simp_runtime_managed_leave(acquired);
}

static void config_saved(GObject *object, GAsyncResult *result, gpointer data) {
    GError *error = NULL;
    g_file_replace_contents_finish(G_FILE(object), result, NULL, &error);
    config_complete(data, error);
}

static void config_loaded(GObject *object, GAsyncResult *result, gpointer data) {
    ConfigSave *save = data;
    GError *error = NULL;
    char *contents = NULL, *etag = NULL;
    gsize length = 0;
    bool loaded = g_file_load_contents_finish(G_FILE(object), result, &contents, &length, &etag, &error);
    bool missing = error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
    if (missing && !*save->expected) g_clear_error(&error);
    if (!error && loaded &&
        (length != strlen(save->expected) || memcmp(contents, save->expected, length) != 0))
        error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_WRONG_ETAG,
                                   "Settings changed on disk; restart before saving preferences");
    if (error) config_complete(save, error);
    else g_file_replace_contents_async(save->file, save->contents, strlen(save->contents),
                                      etag, FALSE, G_FILE_CREATE_PRIVATE | G_FILE_CREATE_REPLACE_DESTINATION,
                                      NULL, config_saved, save);
    g_free(contents);
    g_free(etag);
}

void simp_gtk_config_save(void *self, void *path_text, void *text, void *expected, void *callback) {
    (void)self;
    require_live();
    ConfigSave *save = calloc(1, sizeof(*save));
    if (!save) fatal("allocation failed");
    save->path = text_copy(path_text);
    save->contents = text_copy(text);
    save->expected = text_copy(expected);
    save->context = simp_callback_acquire(callback, "callback<void(String)>");
    save->file = g_file_new_for_path(save->path);
    save->next = config_saves;
    config_saves = save;
    g_file_load_contents_async(save->file, NULL, config_loaded, save);
}

static void config_finish_shutdown(void) {
    while (config_saves) g_main_context_iteration(main_context, TRUE);
}

void *simp_gtk_keyboard_normalize(void *self, void *text) {
    (void)self;
    char *bytes = text_copy(text);
    GtkShortcutTrigger *trigger = gtk_shortcut_trigger_parse_string(bytes);
    /* One key combination only; alternatives/never triggers are not recordable. */
    char *canonical = trigger && GTK_IS_KEYVAL_TRIGGER(trigger) ?
        gtk_shortcut_trigger_to_string(trigger) : g_strdup("");
    void *result = config_string(canonical);
    g_clear_object(&trigger);
    g_free(canonical);
    g_free(bytes);
    return result;
}

bool simp_gtk_font_is_monospace(void *self, void *text) {
    (void)self;
    char *name = text_copy(text);
    bool valid = g_ascii_strcasecmp(name, "monospace") == 0;
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p)
        if (*p < 32 || *p == 127) { g_free(name); return false; }
    PangoFontFamily **families = NULL;
    int count = 0;
    pango_font_map_list_families(pango_cairo_font_map_get_default(), &families, &count);
    for (int i = 0; i < count; ++i)
        if (g_ascii_strcasecmp(name, pango_font_family_get_name(families[i])) == 0 &&
            pango_font_family_is_monospace(families[i])) valid = true;
    g_free(families);
    g_free(name);
    return valid;
}

static gboolean config_record_key(GtkEventControllerKey *controller, guint key,
                                  guint code, GdkModifierType state, gpointer data) {
    (void)controller;
    (void)code;
    if (key == GDK_KEY_Control_L || key == GDK_KEY_Control_R ||
        key == GDK_KEY_Shift_L || key == GDK_KEY_Shift_R ||
        key == GDK_KEY_Alt_L || key == GDK_KEY_Alt_R ||
        key == GDK_KEY_Super_L || key == GDK_KEY_Super_R) return TRUE;
    Connection *connection = data;
    int acquired = signal_enter(connection);
    state &= gtk_accelerator_get_default_mod_mask();
    if (key >= GDK_KEY_A && key <= GDK_KEY_Z) key = gdk_keyval_to_lower(key);
    char *trigger = key == GDK_KEY_Escape ? g_strdup("") : gtk_accelerator_name(key, state);
    void *text = config_string(trigger);
    void *slots[] = { &text };
    SimpRootFrame frame = {0};
    simp_gc_push_or_abort(&frame, slots, 1);
    typedef void (*Adapter)(SimpCallbackContext *, void *);
    ((Adapter)simp_callback_adapter(connection->context))(connection->context, text);
    simp_gc_pop_or_abort(&frame);
    g_free(trigger);
    signal_leave(connection, acquired);
    return TRUE;
}

int64_t simp_gtk_keyboard_record(void *self, void *entry, void *callback) {
    (void)self;
    GtkWidget *widget = widget_live(source_token(entry));
    GtkEventController *controller = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(controller, GTK_PHASE_CAPTURE);
    int64_t token = connect_signal(G_OBJECT(controller), "key-pressed",
                                  G_CALLBACK(config_record_key), callback, "callback<void(String)>");
    gtk_widget_add_controller(widget, controller);
    gtk_widget_grab_focus(widget);
    return token;
}

bool simp_gtk_keyboard_cancel(void *self, int64_t token) {
    (void)self;
    require_live();
    for (Connection *connection = connections; connection; connection = connection->next) {
        if (connection->token != token || connection->closed) continue;
        if (!GTK_IS_EVENT_CONTROLLER_KEY(connection->object)) return false;
        GtkEventController *controller = GTK_EVENT_CONTROLLER(g_object_ref(connection->object));
        GtkWidget *widget = gtk_event_controller_get_widget(controller);
        bool disconnected = simp_gtk_disconnect(token);
        if (widget) gtk_widget_remove_controller(widget, controller);
        g_object_unref(controller);
        return disconnected;
    }
    return false;
}
