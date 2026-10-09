import system as Sys

start {
    Sys.File output = Sys.File("file-io-test.txt", "w+")
    print(output.isOpen())
    print(output.writeLine("alpha"))
    print(output.writeLine("beta"))
    output.flush()
    print(output.tell())
    output.seek(0, 0)
    print(output.readLine())
    list remainingLines = output.readLines()
    print(remainingLines.length)
    String remaining = remainingLines[0]
    print(remaining)
    output.seek(0, 0)
    print(output.readAll())
    output.seek(0, 0)
    print(output.read(5))
    output.close()
    print(output.isOpen())
}
