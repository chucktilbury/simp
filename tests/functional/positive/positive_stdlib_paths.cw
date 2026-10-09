import system as Sys

start {
    Sys.FileSystem paths = Sys.FileSystem()
    print(paths.join("/a/b", "../c").equals("/a/c"))
    print(paths.normalize("a/./b/../c").equals("a/c"))
    print(paths.normalize("../../a/../b").equals("../../b"))
    print(paths.basename("/a/c.txt").equals("c.txt"))
    print(paths.dirname("/a/c.txt").equals("/a"))
    print(paths.extension("/a/c.txt").equals("txt"))
    print(paths.extension(".hidden").length == 0)

    String temporaryFile = paths.tempFile()
    print(temporaryFile.length > 0)
    print(paths.exists(temporaryFile))
    Sys.File temporary = Sys.File(temporaryFile, "w")
    print(temporary.isOpen())
    print(temporary.write("original") == 8)
    temporary.close()
    print(paths.copy(temporaryFile, temporaryFile) == false)
    print(Sys.System().lastError().length > 0)
    print(paths.fileSize(temporaryFile) == 8)
    print(paths.remove(temporaryFile))

    String temporaryDirectory = paths.tempDir()
    print(temporaryDirectory.length > 0)
    print(paths.isDir(temporaryDirectory))
    print(paths.rmdir(temporaryDirectory))

    print(paths.exists("missing-file-for-error-test") == false)
    print(Sys.System().lastError().length > 0)
    print(paths.getCwd().length > 0)
}
