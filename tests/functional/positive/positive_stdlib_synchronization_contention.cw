import synchronization as Sync

class Thread {
    handle launch()
    void join(handle worker)
}

handle Thread.launch() from "cwhip_thread_start"
void Thread.join(handle worker) from "cwhip_thread_join"

class Counter {
    Sync.Mutex mutex
    int value

    Counter() {
        mutex = Sync.Mutex()
        value = 0
    }
}

class CounterWorker : Thread {
    Counter counter

    CounterWorker(Counter shared) {
        counter = shared
    }

    void run() {
        int index = 0
        while (index < 2000) {
            counter.mutex.lock()
            int current = counter.value
            counter.value = current + 1
            counter.mutex.unlock()
            index = index + 1
        }
    }
}

class PingPong {
    Sync.Mutex mutex
    Sync.Condition turnChanged
    int turn
    int exchanges

    PingPong() {
        mutex = Sync.Mutex()
        turnChanged = Sync.Condition()
        turn = 0
        exchanges = 0
    }
}

class PingPongWorker : Thread {
    PingPong shared
    int mine
    int theirs

    PingPongWorker(PingPong state, int myTurn, int otherTurn) {
        shared = state
        mine = myTurn
        theirs = otherTurn
    }

    void run() {
        int round = 0
        while (round < 100) {
            shared.mutex.lock()
            while (shared.turn != mine) {
                shared.turnChanged.wait(shared.mutex)
            }
            shared.exchanges = shared.exchanges + 1
            shared.turn = theirs
            shared.turnChanged.broadcast()
            shared.mutex.unlock()
            round = round + 1
        }
    }
}

class HoldWorker : Thread {
    Sync.Mutex mutex
    Sync.Semaphore holding
    Sync.Semaphore proceed

    HoldWorker(Sync.Mutex shared, Sync.Semaphore held, Sync.Semaphore go) {
        mutex = shared
        holding = held
        proceed = go
    }

    void run() {
        mutex.lock()
        holding.signal()
        proceed.wait()
        mutex.unlock()
    }
}

start {
    Counter counter = Counter()
    CounterWorker first = CounterWorker(counter)
    CounterWorker second = CounterWorker(counter)
    handle firstThread = first.launch()
    handle secondThread = second.launch()
    first.join(firstThread)
    second.join(secondThread)
    print(counter.value)
    print(counter.mutex.close())

    PingPong game = PingPong()
    PingPongWorker ping = PingPongWorker(game, 0, 1)
    PingPongWorker pong = PingPongWorker(game, 1, 0)
    handle pingThread = ping.launch()
    handle pongThread = pong.launch()
    ping.join(pingThread)
    pong.join(pongThread)
    print(game.exchanges)
    print(game.turn)
    print(game.turnChanged.close())
    print(game.mutex.close())

    Sync.Mutex shared = Sync.Mutex()
    Sync.Semaphore held = Sync.Semaphore(0)
    Sync.Semaphore go = Sync.Semaphore(0)
    HoldWorker holder = HoldWorker(shared, held, go)
    handle holderThread = holder.launch()
    held.wait()
    print(shared.unlock())
    print(shared.close())
    go.signal()
    shared.lock()
    print(shared.lock())
    print(shared.unlock())
    print(shared.unlock())
    holder.join(holderThread)
    print(shared.close())
    held.close()
    go.close()
}
