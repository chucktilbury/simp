#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "cwhip/RuntimeGc.h"

extern const CwhipClassMeta cwhip_string_class_meta __attribute__((weak));

static _Thread_local char last_error[512];

typedef struct CwhipSqliteConnection {
    sqlite3 *database;
    uint64_t references;
    int owner_open;
} CwhipSqliteConnection;

typedef struct CwhipSqliteStatement {
    sqlite3_stmt *statement;
    CwhipSqliteConnection *connection;
    void *owner_object;
    struct CwhipSqliteStatement *next;
} CwhipSqliteStatement;

typedef struct CwhipSqliteTransaction {
    CwhipSqliteConnection *connection;
    void *owner_object;
    struct CwhipSqliteTransaction *next;
} CwhipSqliteTransaction;

static CwhipSqliteStatement *statement_registry;
static CwhipSqliteTransaction *transaction_registry;

static void set_error(const char *message) {
    if (message == NULL) message = "unknown SQLite error";
    (void)snprintf(last_error, sizeof(last_error), "%s", message);
}

static CwhipSqliteStatement *find_statement(void *owner_object) {
    for (CwhipSqliteStatement *entry = statement_registry;
         entry != NULL; entry = entry->next) {
        if (entry->owner_object == owner_object) return entry;
    }
    return 0;
}

static void unlink_statement(CwhipSqliteStatement *statement) {
    CwhipSqliteStatement **entry = &statement_registry;
    while (*entry != NULL && *entry != statement) entry = &(*entry)->next;
    if (*entry == statement) *entry = statement->next;
}

static CwhipSqliteTransaction *find_transaction(void *owner_object) {
    for (CwhipSqliteTransaction *entry = transaction_registry;
         entry != NULL; entry = entry->next) {
        if (entry->owner_object == owner_object) return entry;
    }
    return NULL;
}

static void unlink_transaction(CwhipSqliteTransaction *transaction) {
    CwhipSqliteTransaction **entry = &transaction_registry;
    while (*entry != NULL && *entry != transaction) entry = &(*entry)->next;
    if (*entry == transaction) *entry = transaction->next;
}

static void *make_string(const char *text, uint64_t length) {
    if (&cwhip_string_class_meta == NULL) return NULL;
    return cwhip_string_new(&cwhip_string_class_meta, text, length);
}

static void *error_string(const char *message) {
    if (message == NULL) message = last_error;
    return make_string(message, (uint64_t)strlen(message));
}

static int connection_retain(CwhipSqliteConnection *connection) {
    if (connection == NULL || connection->database == NULL ||
        connection->references == UINT64_MAX) {
        set_error("SQLite connection is closed or its reference count is exhausted");
        return 0;
    }
    ++connection->references;
    return 1;
}

static void connection_release(CwhipSqliteConnection *connection) {
    if (connection == NULL || connection->references == 0) return;
    --connection->references;
    if (connection->references == 0) {
        if (connection->database != NULL) {
            (void)sqlite3_close_v2(connection->database);
        }
        free(connection);
    }
}

static int connection_live(CwhipSqliteConnection *connection) {
    if (connection == NULL || connection->database == NULL) {
        set_error("SQLite connection is closed");
        return 0;
    }
    return 1;
}

void *cwhip_sqlite_open(void *self, void *path) {
    (void)self;
    last_error[0] = '\0';
    if (path == NULL) {
        set_error("SQLite path must not be null");
        return 0;
    }
    const char *bytes = NULL;
    uint64_t length = 0;
    cwhip_string_bytes(path, &bytes, &length);
    if (length > INT_MAX ||
        (length != 0 && memchr(bytes, '\0', (size_t)length) != NULL)) {
        set_error("SQLite path is too long or contains a NUL byte");
        return 0;
    }
    char *path_copy = (char *)malloc((size_t)length + 1);
    if (path_copy == NULL) {
        set_error("out of memory while opening SQLite database");
        return 0;
    }
    if (length != 0) memcpy(path_copy, bytes, (size_t)length);
    path_copy[length] = '\0';
    sqlite3 *database = NULL;
    int result = sqlite3_open_v2(path_copy, &database,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
    free(path_copy);
    if (result != SQLITE_OK) {
        set_error(database == NULL ? sqlite3_errstr(result) : sqlite3_errmsg(database));
        if (database != NULL) (void)sqlite3_close_v2(database);
        return 0;
    }
    CwhipSqliteConnection *connection =
        (CwhipSqliteConnection *)calloc(1, sizeof(*connection));
    if (connection == NULL) {
        set_error("out of memory while allocating SQLite connection");
        (void)sqlite3_close(database);
        return 0;
    }
    connection->database = database;
    connection->references = 1;
    connection->owner_open = 1;
    return connection;
}

int32_t cwhip_sqlite_close(void *self, void *connection) {
    (void)self;
    last_error[0] = '\0';
    if (connection == NULL) return 1;
    CwhipSqliteConnection *owner = (CwhipSqliteConnection *)connection;
    if (!owner->owner_open) return 1;
    if (owner->references != 1) {
        set_error("cannot close a SQLite connection while statements or transactions are live");
        return 0;
    }
    int result = sqlite3_close(owner->database);
    if (result != SQLITE_OK) {
        set_error(sqlite3_errmsg(owner->database));
        return 0;
    }
    owner->database = NULL;
    owner->owner_open = 0;
    connection_release(owner);
    return 1;
}

void cwhip_sqlite_release_connection(void *self, void *connection) {
    (void)self;
    CwhipSqliteConnection *owner = (CwhipSqliteConnection *)connection;
    if (owner == NULL || !owner->owner_open) return;
    owner->owner_open = 0;
    connection_release(owner);
}

void *cwhip_sqlite_connection_error(void *self, void *connection) {
    (void)self;
    const char *message = last_error[0] != '\0' ? last_error :
        connection == NULL ? "" :
        ((CwhipSqliteConnection *)connection)->database == NULL ? "" :
        sqlite3_errmsg(((CwhipSqliteConnection *)connection)->database);
    return error_string(message);
}

int32_t cwhip_sqlite_execute(void *self, void *connection, void *sql) {
    (void)self;
    last_error[0] = '\0';
    CwhipSqliteConnection *owner = (CwhipSqliteConnection *)connection;
    if (!connection_live(owner) || sql == NULL) {
        set_error("SQLite connection and SQL text must not be null");
        return 0;
    }
    const char *bytes = NULL;
    uint64_t length = 0;
    cwhip_string_bytes(sql, &bytes, &length);
    if (length > INT_MAX ||
        (length != 0 && memchr(bytes, '\0', (size_t)length) != NULL)) {
        set_error("SQLite SQL is too long or contains a NUL byte");
        return 0;
    }
    char *sql_copy = (char *)malloc((size_t)length + 1);
    if (sql_copy == NULL) {
        set_error("out of memory while executing SQLite SQL");
        return 0;
    }
    if (length != 0) memcpy(sql_copy, bytes, (size_t)length);
    sql_copy[length] = '\0';
    char *message = NULL;
    int result = sqlite3_exec(owner->database, sql_copy, NULL, NULL, &message);
    if (result != SQLITE_OK) {
        set_error(message == NULL ? sqlite3_errmsg(owner->database) : message);
    }
    sqlite3_free(message);
    free(sql_copy);
    return result == SQLITE_OK;
}

int32_t cwhip_sqlite_prepare(void *self, void *connection, void *target,
                            void *sql) {
    (void)self;
    last_error[0] = '\0';
    CwhipSqliteConnection *owner = (CwhipSqliteConnection *)connection;
    if (!connection_live(owner) || target == NULL || sql == NULL) {
        set_error("SQLite connection and SQL text must not be null");
        return 0;
    }
    if (find_statement(target) != NULL) {
        set_error("SQLite statement object is already attached");
        return 0;
    }
    const char *bytes = NULL;
    uint64_t length = 0;
    cwhip_string_bytes(sql, &bytes, &length);
    if (length > INT_MAX ||
        (length != 0 && memchr(bytes, '\0', (size_t)length) != NULL)) {
        set_error("SQLite SQL is too long or contains a NUL byte");
        return 0;
    }
    sqlite3_stmt *statement = NULL;
    const char *tail = NULL;
    static const char empty_sql[] = "";
    const char *sql_bytes = length == 0 ? empty_sql : bytes;
    int result = sqlite3_prepare_v2(owner->database, sql_bytes, (int)length,
        &statement, &tail);
    if (result != SQLITE_OK) {
        set_error(sqlite3_errmsg(owner->database));
        if (statement != NULL) (void)sqlite3_finalize(statement);
        return 0;
    }
    if (statement == NULL) {
        set_error("SQLite SQL did not contain a statement");
        return 0;
    }
    const char *end = sql_bytes + length;
    while (tail < end && (*tail == ' ' || *tail == '\t' || *tail == '\r' ||
                          *tail == '\n' || *tail == '\f' || *tail == '\v')) {
        ++tail;
    }
    if (tail != end) {
        set_error("SQLite prepare accepts exactly one statement");
        (void)sqlite3_finalize(statement);
        return 0;
    }
    CwhipSqliteStatement *wrapped =
        (CwhipSqliteStatement *)calloc(1, sizeof(*wrapped));
    if (wrapped == NULL || !connection_retain(owner)) {
        set_error("out of memory while retaining SQLite statement connection");
        free(wrapped);
        (void)sqlite3_finalize(statement);
        return 0;
    }
    wrapped->statement = statement;
    wrapped->connection = owner;
    wrapped->owner_object = target;
    wrapped->next = statement_registry;
    statement_registry = wrapped;
    return 1;
}

int32_t cwhip_sqlite_busy_timeout(void *self, void *connection, int64_t milliseconds) {
    (void)self;
    last_error[0] = '\0';
    CwhipSqliteConnection *owner = (CwhipSqliteConnection *)connection;
    if (!connection_live(owner) || milliseconds < 0 || milliseconds > INT_MAX) {
        set_error("SQLite busy timeout must be between 0 and INT_MAX milliseconds");
        return 0;
    }
    int result = sqlite3_busy_timeout(owner->database, (int)milliseconds);
    if (result != SQLITE_OK) set_error(sqlite3_errmsg(owner->database));
    return result == SQLITE_OK;
}

int32_t cwhip_sqlite_statement_exists(void *self) {
    return find_statement(self) != NULL;
}

int32_t cwhip_sqlite_finalize(void *self) {
    last_error[0] = '\0';
    CwhipSqliteStatement *wrapped = find_statement(self);
    if (wrapped == NULL) return 1;
    unlink_statement(wrapped);
    int result = sqlite3_finalize(wrapped->statement);
    if (result != SQLITE_OK) set_error(sqlite3_errstr(result));
    connection_release(wrapped->connection);
    free(wrapped);
    return result == SQLITE_OK;
}

int64_t cwhip_sqlite_step(void *self) {
    last_error[0] = '\0';
    CwhipSqliteStatement *wrapped = find_statement(self);
    if (wrapped == NULL) {
        set_error("SQLite statement is not prepared or has been finalized");
        return -1;
    }
    int result = sqlite3_step(wrapped->statement);
    if (result == SQLITE_ROW) return 1;
    if (result == SQLITE_DONE) return 0;
    set_error(sqlite3_errmsg(wrapped->connection->database));
    return -1;
}

void *cwhip_sqlite_statement_error(void *self) {
    CwhipSqliteStatement *wrapped = find_statement(self);
    const char *message = last_error[0] != '\0' ? last_error :
        wrapped == NULL ? last_error :
        sqlite3_errmsg(wrapped->connection->database);
    return error_string(message);
}

static CwhipSqliteStatement *checked_statement(void *owner_object,
                                               int64_t index) {
    CwhipSqliteStatement *wrapped = find_statement(owner_object);
    if (wrapped == NULL) {
        set_error("SQLite statement is not prepared or has been finalized");
        return NULL;
    }
    if (index < 1 || index > INT_MAX) {
        set_error("SQLite parameter indexes are 1-based positive integers");
        return NULL;
    }
    return wrapped;
}

int32_t cwhip_sqlite_bind_null(void *self, int64_t index) {
    last_error[0] = '\0';
    CwhipSqliteStatement *wrapped = checked_statement(self, index);
    if (wrapped == NULL) return 0;
    int result = sqlite3_bind_null(wrapped->statement, (int)index);
    if (result != SQLITE_OK) set_error(sqlite3_errmsg(wrapped->connection->database));
    return result == SQLITE_OK;
}

int32_t cwhip_sqlite_bind_integer(void *self, int64_t index, int64_t value) {
    last_error[0] = '\0';
    CwhipSqliteStatement *wrapped = checked_statement(self, index);
    if (wrapped == NULL) return 0;
    int result = sqlite3_bind_int64(wrapped->statement, (int)index, value);
    if (result != SQLITE_OK) set_error(sqlite3_errmsg(wrapped->connection->database));
    return result == SQLITE_OK;
}

int32_t cwhip_sqlite_bind_real(void *self, int64_t index, double value) {
    last_error[0] = '\0';
    CwhipSqliteStatement *wrapped = checked_statement(self, index);
    if (wrapped == NULL) return 0;
    int result = sqlite3_bind_double(wrapped->statement, (int)index, value);
    if (result != SQLITE_OK) set_error(sqlite3_errmsg(wrapped->connection->database));
    return result == SQLITE_OK;
}

int32_t cwhip_sqlite_bind_text(void *self, int64_t index, void *value) {
    last_error[0] = '\0';
    CwhipSqliteStatement *wrapped = checked_statement(self, index);
    if (wrapped == NULL) return 0;
    if (value == NULL) {
        set_error("SQLite text binding requires a non-null String");
        return 0;
    }
    const char *bytes = NULL;
    uint64_t length = 0;
    cwhip_string_bytes(value, &bytes, &length);
    if (length > INT_MAX) {
        set_error("SQLite text binding exceeds INT_MAX bytes");
        return 0;
    }
    static const char empty = '\0';
    int result = sqlite3_bind_text(wrapped->statement, (int)index,
        length == 0 ? &empty : bytes, (int)length, SQLITE_TRANSIENT);
    if (result != SQLITE_OK) set_error(sqlite3_errmsg(wrapped->connection->database));
    return result == SQLITE_OK;
}

int32_t cwhip_sqlite_bind_blob(void *self, int64_t index, void *value) {
    last_error[0] = '\0';
    CwhipSqliteStatement *wrapped = checked_statement(self, index);
    if (wrapped == NULL) return 0;
    if (value == NULL) {
        set_error("SQLite blob binding requires a non-null buffer");
        return 0;
    }
    const CwhipBuffer *buffer = (const CwhipBuffer *)value;
    static const uint8_t empty = 0;
    int result = sqlite3_bind_blob64(wrapped->statement, (int)index,
        buffer->length == 0 ? &empty : buffer->data, buffer->length,
        SQLITE_TRANSIENT);
    if (result != SQLITE_OK) set_error(sqlite3_errmsg(wrapped->connection->database));
    return result == SQLITE_OK;
}

int64_t cwhip_sqlite_column_type(void *self, int64_t index) {
    CwhipSqliteStatement *wrapped = find_statement(self);
    if (wrapped == NULL || index < 0 ||
        index >= sqlite3_column_count(wrapped->statement)) {
        set_error("SQLite result column index is out of range");
        return -1;
    }
    switch (sqlite3_column_type(wrapped->statement, (int)index)) {
        case SQLITE_NULL: return 0;
        case SQLITE_INTEGER: return 1;
        case SQLITE_FLOAT: return 2;
        case SQLITE_TEXT: return 3;
        case SQLITE_BLOB: return 4;
        default:
            set_error("SQLite returned an unknown storage class");
            return -1;
    }
}

int64_t cwhip_sqlite_column_integer(void *self, int64_t index) {
    CwhipSqliteStatement *wrapped = find_statement(self);
    if (wrapped == NULL) return 0;
    return sqlite3_column_int64(wrapped->statement,
                                (int)index);
}

double cwhip_sqlite_column_real(void *self, int64_t index) {
    CwhipSqliteStatement *wrapped = find_statement(self);
    if (wrapped == NULL) return 0.0;
    return sqlite3_column_double(wrapped->statement,
                                 (int)index);
}

void *cwhip_sqlite_column_text(void *self, int64_t index) {
    CwhipSqliteStatement *wrapped = find_statement(self);
    if (wrapped == NULL) return NULL;
    sqlite3_stmt *stmt = wrapped->statement;
    const unsigned char *bytes = sqlite3_column_text(stmt, (int)index);
    int length = sqlite3_column_bytes(stmt, (int)index);
    if (bytes == NULL && length != 0) return NULL;
    return make_string(bytes == NULL ? "" : (const char *)bytes, (uint64_t)length);
}

void *cwhip_sqlite_column_blob(void *self, int64_t index) {
    CwhipSqliteStatement *wrapped = find_statement(self);
    if (wrapped == NULL) return NULL;
    sqlite3_stmt *stmt = wrapped->statement;
    const void *bytes = sqlite3_column_blob(stmt, (int)index);
    int length = sqlite3_column_bytes(stmt, (int)index);
    if (length < 0) return NULL;
    void *result = cwhip_buffer_new((int64_t)length, "<sqlite>", 8, 0, 0);
    if (result == NULL) return NULL;
    CwhipBuffer *buffer = (CwhipBuffer *)result;
    if (length != 0) {
        if (bytes == NULL) return NULL;
        memcpy(buffer->data, bytes, (size_t)length);
    }
    return result;
}

int32_t cwhip_sqlite_begin_transaction(void *self, void *connection,
                                      void *target) {
    (void)self;
    last_error[0] = '\0';
    CwhipSqliteConnection *owner = (CwhipSqliteConnection *)connection;
    if (!connection_live(owner) || target == NULL) return 0;
    if (find_transaction(target) != NULL) {
        set_error("SQLite transaction object is already attached");
        return 0;
    }
    CwhipSqliteTransaction *transaction =
        (CwhipSqliteTransaction *)calloc(1, sizeof(*transaction));
    if (transaction == NULL) {
        set_error("out of memory while beginning SQLite transaction");
        return 0;
    }
    if (!connection_retain(owner)) {
        free(transaction);
        return 0;
    }
    char *message = NULL;
    int result = sqlite3_exec(owner->database, "BEGIN IMMEDIATE", NULL, NULL, &message);
    if (result != SQLITE_OK) {
        set_error(message == NULL ? sqlite3_errmsg(owner->database) : message);
        sqlite3_free(message);
        connection_release(owner);
        free(transaction);
        return 0;
    }
    sqlite3_free(message);
    transaction->connection = owner;
    transaction->owner_object = target;
    transaction->next = transaction_registry;
    transaction_registry = transaction;
    return 1;
}

int32_t cwhip_sqlite_transaction_exists(void *self) {
    return find_transaction(self) != NULL;
}

int32_t cwhip_sqlite_transaction_active(void *self) {
    CwhipSqliteTransaction *wrapper = find_transaction(self);
    if (wrapper == NULL) return 0;
    return wrapper->connection->database != NULL &&
        !sqlite3_get_autocommit(wrapper->connection->database);
}

static int transaction_finish(CwhipSqliteTransaction *transaction,
                              const char *sql) {
    if (transaction == NULL || transaction->connection->database == NULL) {
        set_error("SQLite transaction is closed");
        return 0;
    }
    if (sqlite3_get_autocommit(transaction->connection->database)) {
        set_error("SQLite transaction is no longer active");
        return 0;
    }
    char *message = NULL;
    int result = sqlite3_exec(transaction->connection->database, sql, NULL, NULL,
                              &message);
    if (result != SQLITE_OK) {
        set_error(message == NULL ?
            sqlite3_errmsg(transaction->connection->database) : message);
    }
    sqlite3_free(message);
    return result == SQLITE_OK;
}

int32_t cwhip_sqlite_transaction_commit(void *self) {
    last_error[0] = '\0';
    return transaction_finish(find_transaction(self), "COMMIT");
}

int32_t cwhip_sqlite_transaction_rollback(void *self) {
    last_error[0] = '\0';
    return transaction_finish(find_transaction(self), "ROLLBACK");
}

int32_t cwhip_sqlite_transaction_close(void *self) {
    last_error[0] = '\0';
    CwhipSqliteTransaction *wrapper = find_transaction(self);
    if (wrapper == NULL) return 1;
    int result = 1;
    if (wrapper->connection->database != NULL &&
        !sqlite3_get_autocommit(wrapper->connection->database)) {
        result = transaction_finish(wrapper, "ROLLBACK");
    }
    unlink_transaction(wrapper);
    connection_release(wrapper->connection);
    free(wrapper);
    return result;
}
