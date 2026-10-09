namespace Time {
    class Clock {
        unsigned epochSeconds()
        unsigned epochMilliseconds()
        unsigned monotonicMilliseconds()
        bool sleepMilliseconds(unsigned milliseconds)
    }

    unsigned Clock.epochSeconds() from "cwhip_time_epoch_seconds"
    unsigned Clock.epochMilliseconds() from "cwhip_time_epoch_milliseconds"
    unsigned Clock.monotonicMilliseconds() from "cwhip_time_monotonic_milliseconds"
    bool Clock.sleepMilliseconds(unsigned milliseconds) from "cwhip_time_sleep_milliseconds"
}
