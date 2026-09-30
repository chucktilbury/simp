# Preserves the original raise location when finally runs before an exception escapes.
start {
  try {
    raise(Exception("original source"))
  } finally {
    print("finally before rethrow")
  }
}
