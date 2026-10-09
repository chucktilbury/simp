class Box {}
start {
  try {
    raise(Exception("message"))
  } except(Box) {
    print("not an exception")
  }
}
