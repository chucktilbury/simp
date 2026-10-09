# Rejects two classes whose base declarations form an inheritance cycle.
class First : Second {}
class Second : First {}
start {}
