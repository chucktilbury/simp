import sql as SQL
import sqlite as SQLite
import system as Sys

start {
    String path = Sys.FileSystem().tempFile()
    SQLite.Connection sqlite = SQLite.Connection(path)
    SQL.Connection database = sqlite
    print(database.isOpen())
    print(database.execute("CREATE TABLE sample (value INTEGER)"))

    SQL.Statement insert = database.prepare("INSERT INTO sample VALUES (?)")
    print(insert != null)
    print(insert.bind(1, SQL.Value(42)))
    print(insert.step() == false)
    print(insert.isDone())
    print(insert.close())

    SQL.Statement query = database.prepare("SELECT value FROM sample")
    print(query.step())
    print(query.value(0).asInteger() == 42)
    print(query.close())
    print(database.close())
    print(Sys.FileSystem().remove(path))
}
