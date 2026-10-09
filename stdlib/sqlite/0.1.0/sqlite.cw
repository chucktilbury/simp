import sql as SQL

namespace SQLite {
    class Connection : SQL.Connection {
        private:
        handle _native
        handle _nativeOpen(String path)
        bool _nativeClose(handle connection)
        void _nativeReleaseConnection(handle connection)
        String _nativeConnectionError(handle connection)
        bool _nativeExecute(handle connection, String sql)
        bool _nativePrepare(handle connection, Statement target, String sql)
        bool _nativeBusyTimeout(handle connection, int milliseconds)
        bool _nativeBeginTransaction(handle connection, Transaction target)

        public:
        Connection(String path) {
            _native = _nativeOpen(path)
        }

        bool isOpen() {
            return _native != null
        }

        String error() {
            return _nativeConnectionError(_native)
        }

        bool execute(String sql) {
            if (_native == null) { raise(Exception("SQLite connection is closed")) }
            return _nativeExecute(_native, sql)
        }

        SQL.Statement prepare(String sql) {
            if (_native == null) { raise(Exception("SQLite connection is closed")) }
            Statement result = Statement()
            if (!_nativePrepare(_native, result, sql)) { return null }
            return result
        }

        bool setBusyTimeout(int milliseconds) {
            if (_native == null) { raise(Exception("SQLite connection is closed")) }
            return _nativeBusyTimeout(_native, milliseconds)
        }

        SQL.Transaction beginTransaction() {
            if (_native == null) { raise(Exception("SQLite connection is closed")) }
            Transaction transaction = Transaction()
            if (!_nativeBeginTransaction(_native, transaction)) {
                raise(Exception(_nativeConnectionError(_native)))
            }
            return transaction
        }

        bool close() {
            if (_native == null) { return true }
            bool closed = _nativeClose(_native)
            if (closed) { _native = null }
            return closed
        }

        destroy {
            if (_native != null) {
                _nativeReleaseConnection(_native)
                _native = null
            }

        }
    }

    class Statement : SQL.Statement {
        private:
        bool _hasRow
        bool _done
        bool _failed
        bool _hasStepped

        private:
        bool _nativeExists()
        bool _nativeFinalize()
        int _nativeStep()
        String _nativeStatementError()
        bool _nativeBindNull(int index)
        bool _nativeBindInteger(int index, int value)
        bool _nativeBindReal(int index, float value)
        bool _nativeBindText(int index, String value)
        bool _nativeBindBlob(int index, buffer value)
        int _nativeColumnType(int index)
        int _nativeColumnInteger(int index)
        float _nativeColumnReal(int index)
        String _nativeColumnText(int index)
        buffer _nativeColumnBlob(int index)

        public:
        Statement() {
            _hasRow = false
            _done = false
            _failed = false
            _hasStepped = false
        }

        String error() {
            return _nativeStatementError()
        }

        bool bind(int index, SQL.Value value) {
            if (!_nativeExists()) { raise(Exception("SQLite statement is not prepared or has been finalized")) }
            if (_hasStepped) { raise(Exception("SQLite parameters must be bound before stepping")) }
            if (value.kind() == 0) { return _nativeBindNull(index) }
            if (value.kind() == 1) { return _nativeBindInteger(index, value.asInteger()) }
            if (value.kind() == 2) { return _nativeBindReal(index, value.asReal()) }
            if (value.kind() == 3) { return _nativeBindText(index, value.asText()) }
            if (value.kind() == 4) { return _nativeBindBlob(index, value.asBlob()) }
            raise(Exception("Invalid SQL value kind"))
        }

        bool step() {
            if (!_nativeExists()) { raise(Exception("SQLite statement is not prepared or has been finalized")) }
            if (_done || _failed) { return false }
            int status = _nativeStep()
            _hasStepped = true
            _hasRow = status == 1
            _done = status == 0
            _failed = status < 0
            return _hasRow
        }

        bool hasRow() {
            return _hasRow
        }

        bool isDone() {
            return _done
        }

        bool failed() {
            return _failed
        }

        SQL.Value value(int zeroBasedColumnIndex) {
            if (!_nativeExists()) { raise(Exception("SQLite statement is not prepared or has been finalized")) }
            if (!_hasRow) { raise(Exception("SQLite statement has no current row")) }
            int kind = _nativeColumnType(zeroBasedColumnIndex)
            if (kind == 0) { return SQL.Value() }
            if (kind == 1) { return SQL.Value(_nativeColumnInteger(zeroBasedColumnIndex)) }
            if (kind == 2) { return SQL.Value(_nativeColumnReal(zeroBasedColumnIndex)) }
            if (kind == 3) { return SQL.Value(_nativeColumnText(zeroBasedColumnIndex)) }
            if (kind == 4) { return SQL.Value(_nativeColumnBlob(zeroBasedColumnIndex)) }
            raise(Exception("SQLite returned an unsupported storage class"))
        }

        bool close() {
            bool finalized = _nativeFinalize()
            _hasRow = false
            _done = true
            return finalized
        }

        destroy {
            close()
        }

    }

    class Transaction : SQL.Transaction {
        private:
        bool _active
        bool _nativeExists()
        bool _nativeActive()
        bool _nativeCommit()
        bool _nativeRollback()
        bool _nativeClose()

        public:
        Transaction() {
            _active = false
        }

        bool isActive() {
            if (!_nativeExists()) { return false }
            _active = _nativeActive()
            if (!_active) {
                _nativeClose()
            }
            return _active
        }

        bool commit() {
            if (!isActive()) { return false }
            bool committed = _nativeCommit()
            if (committed) {
                _active = false
                _nativeClose()
            }
            return committed
        }

        bool rollback() {
            if (!isActive()) { return false }
            bool rolledBack = _nativeRollback()
            if (rolledBack) {
                _active = false
                _nativeClose()
            }
            return rolledBack
        }

        bool close() {
            bool closed = _nativeClose()
            _active = false
            return closed
        }

        destroy {
            close()
        }

    }

    handle Connection._nativeOpen(String path) from "cwhip_sqlite_open"
    bool Connection._nativeClose(handle connection) from "cwhip_sqlite_close"
    void Connection._nativeReleaseConnection(handle connection) from "cwhip_sqlite_release_connection"
    String Connection._nativeConnectionError(handle connection) from "cwhip_sqlite_connection_error"
    bool Connection._nativeExecute(handle connection, String sql) from "cwhip_sqlite_execute"
    bool Connection._nativePrepare(handle connection, Statement target, String sql) from "cwhip_sqlite_prepare"
    bool Connection._nativeBusyTimeout(handle connection, int milliseconds) from "cwhip_sqlite_busy_timeout"
    bool Connection._nativeBeginTransaction(handle connection, Transaction target) from "cwhip_sqlite_begin_transaction"

    bool Statement._nativeExists() from "cwhip_sqlite_statement_exists"
    bool Statement._nativeFinalize() from "cwhip_sqlite_finalize"
    int Statement._nativeStep() from "cwhip_sqlite_step"
    String Statement._nativeStatementError() from "cwhip_sqlite_statement_error"
    bool Statement._nativeBindNull(int index) from "cwhip_sqlite_bind_null"
    bool Statement._nativeBindInteger(int index, int value) from "cwhip_sqlite_bind_integer"
    bool Statement._nativeBindReal(int index, float value) from "cwhip_sqlite_bind_real"
    bool Statement._nativeBindText(int index, String value) from "cwhip_sqlite_bind_text"
    bool Statement._nativeBindBlob(int index, buffer value) from "cwhip_sqlite_bind_blob"
    int Statement._nativeColumnType(int index) from "cwhip_sqlite_column_type"
    int Statement._nativeColumnInteger(int index) from "cwhip_sqlite_column_integer"
    float Statement._nativeColumnReal(int index) from "cwhip_sqlite_column_real"
    String Statement._nativeColumnText(int index) from "cwhip_sqlite_column_text"
    buffer Statement._nativeColumnBlob(int index) from "cwhip_sqlite_column_blob"

    bool Transaction._nativeExists() from "cwhip_sqlite_transaction_exists"
    bool Transaction._nativeActive() from "cwhip_sqlite_transaction_active"
    bool Transaction._nativeCommit() from "cwhip_sqlite_transaction_commit"
    bool Transaction._nativeRollback() from "cwhip_sqlite_transaction_rollback"
    bool Transaction._nativeClose() from "cwhip_sqlite_transaction_close"
}
