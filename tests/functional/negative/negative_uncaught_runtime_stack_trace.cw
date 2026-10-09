# Runtime failures in nested methods report the active Cwhip call frames.
class Worker {
  int inner(Worker self) {
    return 8 / 0
  }
  int middle(Worker self) {
    return self.inner(self)
  }
  int outer(Worker self) {
    return self.middle(self)
  }
}
start {
  Worker worker = Worker()
  print("begin")
  print(worker.outer(worker))
}
