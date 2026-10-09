# Rejects assignment to the read-only name bound by an except clause.
start {
  try {
    raise(Exception("read only"))
  } except() as message {
    message = "changed"
  }
}
