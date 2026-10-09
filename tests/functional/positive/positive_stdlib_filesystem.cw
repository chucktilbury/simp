import system as Sys

start {
    Sys.FileSystem fileSystem = Sys.FileSystem()
    String directory = "filesystem-test"
    String path = "filesystem-test/item.txt"
    String copyPath = "filesystem-test/copy.txt"
    print(fileSystem.mkdir(directory))
    Sys.File output = Sys.File(path, "w")
    output.write("abc")
    output.close()
    print(fileSystem.exists(path))
    print(fileSystem.isFile(path))
    print(fileSystem.isDir(directory))
    print(fileSystem.fileSize(path))
    print(fileSystem.listDir(directory).length)
    print(fileSystem.copy(path, copyPath))
    print(fileSystem.rename(copyPath, "filesystem-test/renamed.txt"))
    print(fileSystem.exists("filesystem-test/renamed.txt"))
    print(fileSystem.remove(path))
    print(fileSystem.remove("filesystem-test/renamed.txt"))
    print(fileSystem.rmdir(directory))
}
