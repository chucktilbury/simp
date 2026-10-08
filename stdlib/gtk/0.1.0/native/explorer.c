/* Included by gtk.c: shares the package's widget tokens and owner-thread lifecycle. */
typedef struct TreeItem {
    GObject parent;
    int64_t id;
    char *label;
    char *icon;
    char *value;
    int64_t data;
    GListStore *children;
} TreeItem;
typedef struct TreeItemClass {
    GObjectClass parent;
} TreeItemClass;
G_DEFINE_TYPE(TreeItem, simp_tree_item, G_TYPE_OBJECT)

static void simp_tree_item_finalize(GObject *object) {
    TreeItem *item = (TreeItem *)object;
    g_free(item->label);
    g_free(item->icon);
    g_free(item->value);
    g_clear_object(&item->children);
    G_OBJECT_CLASS(simp_tree_item_parent_class)->finalize(object);
}
static void simp_tree_item_class_init(TreeItemClass *klass) {
    G_OBJECT_CLASS(klass)->finalize = simp_tree_item_finalize;
}
static void simp_tree_item_init(TreeItem *item) { (void)item; }

typedef struct TreeState {
    unsigned refs;
    unsigned active;
    bool stopped;
    SimpCallbackContext *request;
    SimpCallbackContext *activate;
    GListStore *roots;
    GHashTable *items;
} TreeState;

static TreeState *tree_ref(TreeState *tree) {
    ++tree->refs;
    return tree;
}
static void tree_release_callbacks(TreeState *tree) {
    if (tree->active) return;
    if (tree->request) {
        simp_callback_release(tree->request);
        simp_callback_dispose(tree->request);
        tree->request = NULL;
        simp_callback_release(tree->activate);
        simp_callback_dispose(tree->activate);
        tree->activate = NULL;
    }
}
static void tree_unref(gpointer data) {
    TreeState *tree = data;
    if (--tree->refs) return;
    g_object_unref(tree->roots);
    g_hash_table_unref(tree->items);
    free(tree);
}
static void tree_stop(GtkWidget *widget) {
    TreeState *tree = g_object_get_data(G_OBJECT(widget), "simp-tree");
    if (!tree) return;
    tree->stopped = true;
    tree_release_callbacks(tree);
}
static void tree_widget_destroy(gpointer data) { tree_unref(data); }

static void tree_invoke(TreeState *tree, bool request, int64_t id) {
    if (tree->stopped) return;
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    tree_ref(tree);
    ++tree->active;
    SimpCallbackContext *context = request ? tree->request : tree->activate;
    typedef void (*Adapter)(SimpCallbackContext *, int64_t);
    ((Adapter)simp_callback_adapter(context))(context, id);
    --tree->active;
    if (tree->stopped) tree_release_callbacks(tree);
    tree_unref(tree);
    simp_runtime_managed_leave(acquired);
}

static GListModel *tree_children(gpointer object, gpointer data) {
    (void)data;
    TreeItem *item = object;
    if (!item->children) return NULL;
    return G_LIST_MODEL(g_object_ref(item->children));
}
static void tree_expanded(GtkTreeListRow *row, GParamSpec *property, gpointer data) {
    (void)property;
    if (gtk_tree_list_row_get_expanded(row)) {
        TreeItem *item = gtk_tree_list_row_get_item(row);
        tree_invoke(data, true, item->id);
    }
}
static void tree_setup_row(GtkSignalListItemFactory *factory, GtkListItem *row, gpointer data) {
    (void)factory;
    (void)data;
    GtkWidget *expander = gtk_tree_expander_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(box), gtk_image_new());
    GtkWidget *label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_box_append(GTK_BOX(box), label);
    gtk_tree_expander_set_child(GTK_TREE_EXPANDER(expander), box);
    gtk_list_item_set_child(row, expander);
}
static void tree_bind_row(GtkSignalListItemFactory *factory, GtkListItem *row, gpointer data) {
    (void)factory;
    GtkTreeListRow *model_row = gtk_list_item_get_item(row);
    TreeItem *item = gtk_tree_list_row_get_item(model_row);
    GtkWidget *expander = gtk_list_item_get_child(row);
    GtkWidget *box = gtk_tree_expander_get_child(GTK_TREE_EXPANDER(expander));
    GtkWidget *image = gtk_widget_get_first_child(box);
    gtk_image_set_from_icon_name(GTK_IMAGE(image), item->icon);
    gtk_label_set_text(GTK_LABEL(gtk_widget_get_next_sibling(image)), item->label);
    gtk_widget_set_tooltip_text(expander, item->label);
    gtk_tree_expander_set_list_row(GTK_TREE_EXPANDER(expander), model_row);
    g_signal_connect(model_row, "notify::expanded", G_CALLBACK(tree_expanded), data);
    tree_expanded(model_row, NULL, data);
}
static void tree_unbind_row(GtkSignalListItemFactory *factory, GtkListItem *row, gpointer data) {
    (void)factory;
    GtkTreeListRow *model_row = gtk_list_item_get_item(row);
    if (model_row) g_signal_handlers_disconnect_by_data(model_row, data);
    gtk_tree_expander_set_list_row(GTK_TREE_EXPANDER(gtk_list_item_get_child(row)), NULL);
}
static void tree_activated(GtkListView *view, guint position, gpointer data) {
    GtkTreeListRow *row =
        g_list_model_get_item(G_LIST_MODEL(gtk_list_view_get_model(view)), position);
    if (!row) return;
    TreeItem *item = gtk_tree_list_row_get_item(row);
    int64_t id = item->id;
    if (item->children) gtk_tree_list_row_set_expanded(row, !gtk_tree_list_row_get_expanded(row));
    else tree_invoke(data, false, id);
    g_object_unref(row);
}
static TreeState *tree_live(int64_t token) {
    GtkWidget *widget = widget_live(token);
    TreeState *tree = g_object_get_data(G_OBJECT(widget), "simp-tree");
    if (!tree || tree->stopped) fatal("operation requires a live Tree");
    return tree;
}
void simp_gtk_tree_setup(void *self, int64_t token, void *request, void *activate) {
    (void)self;
    GtkWidget *widget = widget_live(token);
    if (!GTK_IS_LIST_VIEW(widget) || g_object_get_data(G_OBJECT(widget), "simp-tree"))
        fatal("invalid Tree setup");
    TreeState *tree = calloc(1, sizeof(*tree));
    if (!tree) fatal("allocation failed");
    tree->refs = 1;
    tree->roots = g_list_store_new(simp_tree_item_get_type());
    tree->items = g_hash_table_new_full(g_int64_hash, g_int64_equal, g_free, g_object_unref);
    tree->request = simp_callback_acquire(request, "callback<void(int)>");
    tree->activate = simp_callback_acquire(activate, "callback<void(int)>");
    GtkTreeListModel *model =
        gtk_tree_list_model_new(G_LIST_MODEL(g_object_ref(tree->roots)), FALSE, FALSE,
                                tree_children, tree_ref(tree), tree_unref);
    GtkSingleSelection *selection = gtk_single_selection_new(G_LIST_MODEL(model));
    gtk_single_selection_set_autoselect(selection, FALSE);
    gtk_list_view_set_model(GTK_LIST_VIEW(widget), GTK_SELECTION_MODEL(selection));
    g_object_unref(selection);
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(tree_setup_row), NULL);
    g_object_set_data_full(G_OBJECT(factory), "simp-tree", tree_ref(tree), tree_unref);
    g_signal_connect(factory, "bind", G_CALLBACK(tree_bind_row), tree);
    g_signal_connect(factory, "unbind", G_CALLBACK(tree_unbind_row), tree);
    gtk_list_view_set_factory(GTK_LIST_VIEW(widget), factory);
    g_object_unref(factory);
    g_object_set_data_full(G_OBJECT(widget), "simp-tree", tree, tree_widget_destroy);
    g_signal_connect(widget, "activate", G_CALLBACK(tree_activated), tree);
}
void simp_gtk_tree_add(void *self, int64_t token, int64_t parent, int64_t id, void *label,
                       void *icon, bool expandable, void *value, int64_t item_data) {
    (void)self;
    TreeState *tree = tree_live(token);
    TreeItem *ancestor = parent ? g_hash_table_lookup(tree->items, &parent) : NULL;
    GListStore *store = parent ? (ancestor ? ancestor->children : NULL) : tree->roots;
    if (!store || id <= 0 || g_hash_table_contains(tree->items, &id))
        fatal("invalid Tree parent or duplicate id");
    TreeItem *item = g_object_new(simp_tree_item_get_type(), NULL);
    item->id = id;
    item->label = text_copy(label);
    item->icon = text_copy(icon);
    item->value = text_copy(value);
    item->data = item_data;
    if (expandable) item->children = g_list_store_new(simp_tree_item_get_type());
    int64_t *key = g_new(int64_t, 1);
    *key = id;
    g_hash_table_insert(tree->items, key, item);
    g_list_store_append(store, item);
}
static TreeItem *tree_item_live(int64_t token, int64_t id) {
    TreeItem *item = g_hash_table_lookup(tree_live(token)->items, &id);
    if (!item) fatal("invalid Tree item");
    return item;
}
void *simp_gtk_tree_value(void *self, int64_t token, int64_t id) {
    (void)self;
    const char *value = tree_item_live(token, id)->value;
    return simp_string_new(&simp_string_class_meta, value, strlen(value));
}
int64_t simp_gtk_tree_data(void *self, int64_t token, int64_t id) {
    (void)self;
    return tree_item_live(token, id)->data;
}
int64_t simp_gtk_tree_find(void *self, int64_t token, void *value) {
    (void)self;
    TreeState *tree = tree_live(token);
    char *text = text_copy(value);
    GHashTableIter iter;
    gpointer item_data;
    int64_t id = 0;
    g_hash_table_iter_init(&iter, tree->items);
    while (g_hash_table_iter_next(&iter, NULL, &item_data)) {
        TreeItem *item = item_data;
        if (strcmp(item->value, text) == 0 && (!id || item->id < id)) id = item->id;
    }
    g_free(text);
    return id;
}
int64_t simp_gtk_tree_count(void *self, int64_t token, int64_t parent) {
    (void)self;
    TreeState *tree = tree_live(token);
    GListStore *store = parent ? tree_item_live(token, parent)->children : tree->roots;
    if (!store) fatal("Tree item is not expandable");
    return g_list_model_get_n_items(G_LIST_MODEL(store));
}
static void tree_forget(TreeState *tree, GListStore *store) {
    guint count = g_list_model_get_n_items(G_LIST_MODEL(store));
    for (guint i = 0; i < count; ++i) {
        TreeItem *item = g_list_model_get_item(G_LIST_MODEL(store), i);
        if (item->children) tree_forget(tree, item->children);
        g_hash_table_remove(tree->items, &item->id);
        g_object_unref(item);
    }
    g_list_store_remove_all(store);
}
void simp_gtk_tree_clear(void *self, int64_t token, int64_t parent) {
    (void)self;
    TreeState *tree = tree_live(token);
    TreeItem *item = parent ? g_hash_table_lookup(tree->items, &parent) : NULL;
    GListStore *store = parent ? (item ? item->children : NULL) : tree->roots;
    if (!store) fatal("invalid Tree parent");
    tree_forget(tree, store);
}
void simp_gtk_tree_expand(void *self, int64_t token, int64_t id, bool expanded) {
    (void)self;
    TreeState *tree = tree_ref(tree_live(token));
    TreeItem *target = g_hash_table_lookup(tree->items, &id);
    if (!target || !target->children) fatal("Tree expansion requires an expandable row");
    GtkSelectionModel *selection =
        g_object_ref(gtk_list_view_get_model(GTK_LIST_VIEW(widget_live(token))));
    guint count = g_list_model_get_n_items(G_LIST_MODEL(selection));
    bool found = false;
    for (guint i = 0; i < count; ++i) {
        GtkTreeListRow *row = g_list_model_get_item(G_LIST_MODEL(selection), i);
        TreeItem *item = gtk_tree_list_row_get_item(row);
        bool match = item->id == id;
        if (match) {
            found = true;
            gtk_tree_list_row_set_expanded(row, expanded);
            if (expanded) tree_invoke(tree, true, id);
        }
        g_object_unref(row);
        if (match) break;
    }
    g_object_unref(selection);
    tree_unref(tree);
    if (!found) fatal("Tree row is not visible; expand its ancestors first");
}
bool simp_gtk_tree_is_expanded(void *self, int64_t token, int64_t id) {
    (void)self;
    tree_live(token);
    GtkSelectionModel *selection =
        g_object_ref(gtk_list_view_get_model(GTK_LIST_VIEW(widget_live(token))));
    guint count = g_list_model_get_n_items(G_LIST_MODEL(selection));
    bool expanded = false;
    for (guint i = 0; i < count; ++i) {
        GtkTreeListRow *row = g_list_model_get_item(G_LIST_MODEL(selection), i);
        TreeItem *item = gtk_tree_list_row_get_item(row);
        bool match = item->id == id;
        if (match) expanded = gtk_tree_list_row_get_expanded(row);
        g_object_unref(row);
        if (match) break;
    }
    g_object_unref(selection);
    return expanded;
}
int64_t simp_gtk_paned_get_position(void *self, int64_t token) {
    (void)self;
    GtkWidget *widget = widget_live(token);
    if (!GTK_IS_PANED(widget)) fatal("invalid Paned");
    return gtk_paned_get_position(GTK_PANED(widget));
}
void simp_gtk_paned_position(void *self, int64_t token, int64_t position) {
    (void)self;
    GtkWidget *widget = widget_live(token);
    if (!GTK_IS_PANED(widget) || position < 0 || position > G_MAXINT)
        fatal("invalid Paned position");
    gtk_paned_set_position(GTK_PANED(widget), (int)position);
}

typedef struct DirectoryEntry {
    char *path;
    char *name;
    int64_t kind;
} DirectoryEntry;
typedef struct DirectoryJob {
    struct DirectoryJob *next;
    gint refs;
    gint done;
    int64_t token;
    int64_t owner;
    char *path;
    char *root;
    char **excludes;
    bool hidden;
    GCancellable *cancel;
    GPtrArray *entries;
    char *error;
    guint position;
    GSource *source;
    SimpCallbackContext *context;
    unsigned active;
    bool closed;
} DirectoryJob;
static DirectoryJob *directory_jobs;

static void directory_entry_free(gpointer data) {
    DirectoryEntry *entry = data;
    g_free(entry->path);
    g_free(entry->name);
    free(entry);
}
static void directory_unref(DirectoryJob *job) {
    if (!g_atomic_int_dec_and_test(&job->refs)) return;
    g_free(job->path);
    g_free(job->root);
    g_strfreev(job->excludes);
    g_free(job->error);
    g_ptr_array_unref(job->entries);
    g_object_unref(job->cancel);
    free(job);
}
static gint directory_compare(gconstpointer a, gconstpointer b) {
    const DirectoryEntry *first = *(DirectoryEntry *const *)a;
    const DirectoryEntry *second = *(DirectoryEntry *const *)b;
    if ((first->kind == 1) != (second->kind == 1)) return first->kind == 1 ? -1 : 1;
    return strcmp(first->name, second->name);
}
static bool directory_excluded(const DirectoryJob *job, const char *name, const char *path) {
    if (!job->excludes) return false;
    const char *relative = NULL;
    size_t root_length = job->root ? strlen(job->root) : 0;
    if (root_length && strncmp(path, job->root, root_length) == 0 &&
        (path[root_length] == '/' || (root_length == 1 && job->root[0] == '/')))
        relative = path + root_length + (path[root_length] == '/' ? 1 : 0);
    for (char **pattern = job->excludes; *pattern; ++pattern) {
        if (!**pattern) continue;
        if (strchr(*pattern, '/')) {
            if (relative && g_pattern_match_simple(*pattern, relative)) return true;
        } else if (g_pattern_match_simple(*pattern, name))
            return true;
    }
    return false;
}
static gpointer directory_worker(gpointer data) {
    DirectoryJob *job = data;
    GError *error = NULL;
    GFile *folder = g_file_new_for_path(job->path);
    GFileEnumerator *enumerator = g_file_enumerate_children(
        folder, G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE,
        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, job->cancel, &error);
    if (enumerator) {
        while (!g_cancellable_is_cancelled(job->cancel)) {
            GFileInfo *info = g_file_enumerator_next_file(enumerator, job->cancel, &error);
            if (!info) break;
            const char *name = g_file_info_get_name(info);
            if (name && (job->hidden || name[0] != '.')) {
                if (!g_utf8_validate(name, -1, NULL)) {
                    g_set_error_literal(&error, G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME,
                                        "Directory contains a filename that is not valid UTF-8");
                    g_object_unref(info);
                    break;
                }
                char *entry_path = g_build_filename(job->path, name, NULL);
                if (directory_excluded(job, name, entry_path)) {
                    g_free(entry_path);
                    g_object_unref(info);
                    continue;
                }
                DirectoryEntry *entry = g_new0(DirectoryEntry, 1);
                entry->name = g_strdup(name);
                entry->path = entry_path;
                GFileType type = g_file_info_get_file_type(info);
                entry->kind = type == G_FILE_TYPE_DIRECTORY       ? 1
                              : type == G_FILE_TYPE_SYMBOLIC_LINK ? 2
                              : type == G_FILE_TYPE_REGULAR       ? 0
                                                                  : 3;
                g_ptr_array_add(job->entries, entry);
            }
            g_object_unref(info);
        }
        GError *close_error = NULL;
        if (!g_file_enumerator_close(enumerator, job->cancel, &close_error)) {
            if (!error) error = close_error;
            else g_error_free(close_error);
        }
        g_object_unref(enumerator);
    }
    g_object_unref(folder);
    if (error) {
        job->error = g_strdup(error->message);
        g_error_free(error);
        g_ptr_array_set_size(job->entries, 0);
    } else g_ptr_array_sort(job->entries, directory_compare);
    g_atomic_int_set(&job->done, 1);
    directory_unref(job);
    return NULL;
}
static void directory_release(DirectoryJob *job) {
    if (job->active || !job->context) return;
    simp_callback_release(job->context);
    simp_callback_dispose(job->context);
    job->context = NULL;
}
static void directory_close(DirectoryJob *job) {
    if (job->closed) return;
    job->closed = true;
    DirectoryJob **link = &directory_jobs;
    while (*link != job)
        link = &(*link)->next;
    *link = job->next;
    g_cancellable_cancel(job->cancel);
    g_source_destroy(job->source);
    g_source_unref(job->source);
    job->source = NULL;
    directory_release(job);
    directory_unref(job);
}
static void directory_cancel_owner(int64_t owner_token) {
    for (DirectoryJob *job = directory_jobs; job;) {
        DirectoryJob *next = job->next;
        if (!owner_token || job->owner == owner_token) directory_close(job);
        job = next;
    }
}
static void directory_deliver(DirectoryJob *job, int64_t count, const char *error) {
    void *error_value = simp_string_new(&simp_string_class_meta, error, strlen(error));
    void *slots[] = {&error_value};
    SimpRootFrame frame = {0};
    simp_gc_push_or_abort(&frame, slots, 1);
    ++job->active;
    typedef void (*Adapter)(SimpCallbackContext *, int64_t, void *);
    ((Adapter)simp_callback_adapter(job->context))(job->context, count, error_value);
    --job->active;
    if (job->closed) directory_release(job);
    simp_gc_pop_or_abort(&frame);
}
static gboolean directory_dispatch(gpointer data) {
    DirectoryJob *job = data;
    if (!g_atomic_int_get(&job->done)) return G_SOURCE_CONTINUE;
    simp_gtk_require_owner();
    int acquired = simp_runtime_managed_enter();
    g_atomic_int_inc(&job->refs);
    if (job->position < job->entries->len) {
        guint count = MIN(64u, job->entries->len - job->position);
        directory_deliver(job, count, "");
        job->position += count;
    }
    if (!job->closed && job->position == job->entries->len) {
        directory_deliver(job, -1, job->error ? job->error : "");
        directory_close(job);
    }
    bool closed = job->closed;
    directory_unref(job);
    simp_runtime_managed_leave(acquired);
    return closed ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
}
static int64_t directory_start(int64_t owner_token, void *path, bool hidden, void *root,
                               void *excludes, void *callback) {
    tree_live(owner_token);
    DirectoryJob *job = g_new0(DirectoryJob, 1);
    job->refs = 2; /* Owner thread and filesystem worker own independent references. */
    job->token = token_new();
    job->owner = owner_token;
    job->path = text_copy(path);
    job->hidden = hidden;
    if (root) job->root = text_copy(root);
    if (excludes) {
        char *patterns = text_copy(excludes);
        if (*patterns) job->excludes = g_strsplit(patterns, "\n", -1);
        g_free(patterns);
    }
    job->entries = g_ptr_array_new_with_free_func(directory_entry_free);
    job->cancel = g_cancellable_new();
    job->context = simp_callback_acquire(callback, "callback<void(int,String)>");
    job->source = g_timeout_source_new(5);
    g_source_set_callback(job->source, directory_dispatch, job, NULL);
    g_source_attach(job->source, main_context);
    job->next = directory_jobs;
    directory_jobs = job;
    GThread *thread = g_thread_new("simp-directory", directory_worker, job);
    g_thread_unref(thread);
    return job->token;
}
int64_t simp_gtk_directory_start(void *self, int64_t owner_token, void *path, bool hidden,
                                 void *callback) {
    (void)self;
    return directory_start(owner_token, path, hidden, NULL, NULL, callback);
}
int64_t simp_gtk_directory_start_filtered(void *self, int64_t owner_token, void *path,
                                          bool hidden, void *root, void *excludes,
                                          void *callback) {
    (void)self;
    return directory_start(owner_token, path, hidden, root, excludes, callback);
}
void simp_gtk_directory_cancel(void *self, int64_t token) {
    (void)self;
    require_managed();
    simp_gtk_require_owner();
    for (DirectoryJob *job = directory_jobs; job; job = job->next) {
        if (job->token == token) {
            directory_close(job);
            return;
        }
    }
}
void simp_gtk_directory_append(void *self, int64_t token, int64_t tree_token, int64_t parent,
                               int64_t first_id, int64_t count, void *icons) {
    (void)self;
    TreeState *tree = tree_ref(tree_live(tree_token));
    DirectoryJob *job = directory_jobs;
    while (job && job->token != token)
        job = job->next;
    if (!job || !job->active || job->owner != tree_token || count <= 0 || count > 64 ||
        count > job->entries->len - job->position || first_id <= 0 || first_id > INT64_MAX - count)
        fatal("DirectoryScan.append requires the current batch and owning Tree");
    TreeItem *ancestor = parent ? tree_item_live(tree_token, parent) : NULL;
    GListStore *store = parent ? ancestor->children : tree->roots;
    if (!store) fatal("invalid Tree parent");
    g_object_ref(store);
    char *rules_text = text_copy(icons);
    char **rules = g_strsplit(rules_text, "\n", -1);
    const char *defaults[4] = {NULL, NULL, NULL, NULL};
    for (char **rule = rules; *rule; ++rule) {
        char *separator = strchr(*rule, '=');
        if (!separator || separator == *rule || !separator[1]) fatal("invalid icon rule");
        *separator = 0;
        if ((*rule)[0] >= '0' && (*rule)[0] <= '3' && !(*rule)[1])
            defaults[(*rule)[0] - '0'] = separator + 1;
        else if ((*rule)[0] != '.') fatal("icon rule key must be a kind or filename suffix");
    }
    for (unsigned i = 0; i < 4; ++i)
        if (!defaults[i]) fatal("icon rules require defaults for kinds 0 through 3");
    gpointer additions[64];
    for (int64_t i = 0; i < count; ++i) {
        DirectoryEntry *entry = g_ptr_array_index(job->entries, job->position + i);
        int64_t id = first_id + i;
        if (g_hash_table_contains(tree->items, &id)) fatal("duplicate Tree id");
        const char *icon = defaults[entry->kind];
        if (entry->kind == 0) {
            for (char **rule = rules; *rule; ++rule) {
                if ((*rule)[0] == '.' && g_str_has_suffix(entry->name, *rule)) {
                    icon = *rule + strlen(*rule) + 1;
                    break;
                }
            }
        }
        TreeItem *item = g_object_new(simp_tree_item_get_type(), NULL);
        item->id = id;
        item->label = g_strdup(entry->name);
        item->icon = g_strdup(icon);
        item->value = g_strdup(entry->path);
        item->data = entry->kind;
        if (entry->kind == 1) item->children = g_list_store_new(simp_tree_item_get_type());
        int64_t *key = g_new(int64_t, 1);
        *key = id;
        g_hash_table_insert(tree->items, key, item);
        additions[i] = item;
    }
    g_list_store_splice(store, g_list_model_get_n_items(G_LIST_MODEL(store)), 0, additions,
                        (guint)count);
    g_strfreev(rules);
    g_free(rules_text);
    g_object_unref(store);
    tree_unref(tree);
}
