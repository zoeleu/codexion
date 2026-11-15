*This project has been created as part of the 42 curriculum by zleullie.*

## Description

Codexion is a C concurrency simulation in which coder threads share USB
dongles to compile, debug and refactor. The assignment requires FIFO or EDF
arbitration through custom heaps, dongle cooldowns and a burnout monitor.

The simulation starts one thread per coder plus a monitor thread, and stops
on burnout or once every coder has met the compile quota. Dongles use
FIFO/EDF request heaps, paired acquisition and release cooldowns.

## Instructions

Build with `make` using a C compiler and POSIX threads. Use `make clean` to
remove object files, `make fclean` to remove the executable as well, and
`make re` to rebuild.

Run `make norm` to check sources and headers in [src](src) and [include](include) with
`norminette` installed.

Run the simulation with:

```text
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

All eight arguments are mandatory. Times are in milliseconds; `scheduler`
must be exactly `fifo` or `edf`. Numeric arguments accept decimal digits only,
without signs or whitespace. The coder count and four action/burnout durations
must be positive; the compile quota and cooldown may be zero. Counts are
limited to `INT_MAX` and durations/cooldown to `LONG_MAX`; numeric conversion
overflow is rejected.
A compile quota of zero succeeds immediately without starting threads or
printing simulation events.

For example, run four coders with EDF scheduling:

```sh
./codexion 4 800 200 200 200 5 10 edf
```

Both quota completion and burnout normally return exit status 0. Invalid
arguments and initialization/runtime errors return 1.

### Source layout

- [include/codexion.h](include/codexion.h): shared types and function prototypes.
- [src/main.c](src/main.c): argument parsing and the entry point.
- [src/simulation.c](src/simulation.c): setup, thread start and join, cleanup.
- [src/monitor.c](src/monitor.c): burnout and completion detection, stopping the run.
- [src/coder.c](src/coder.c): the coder routine and its timed phases.
- [src/dongle.c](src/dongle.c): taking and releasing a pair of dongles.
- [src/heap.c](src/heap.c), [src/heap_order.c](src/heap_order.c), [src/heap_storage.c](src/heap_storage.c): the FIFO/EDF request heap.
- [src/log.c](src/log.c): printing one state line.
- [src/time.c](src/time.c): timing helpers.

## Blocking cases handled

To compile, a coder puts one request in the heaps of both neighbouring dongles,
in a single critical section. It gets the pair only when its request heads both
heaps and both dongles are free and past their cooldown; then it takes both at
once. A coder never holds one dongle while waiting for the other, which breaks
the hold-and-wait condition, so the coders cannot deadlock. A single coder is
the exception: it takes its only dongle and waits until it burns out, because
no second dongle exists.

FIFO serves requests by arrival order; EDF serves the earliest burnout deadline
first, with arrival order as the tie-breaker. A new request can never overtake a
higher-priority one already waiting for the same dongle. This makes the
arbitration fair, but no scheduler can save a configuration whose timings are
too tight. Coders with an odd index start half a compile late, so neighbours do
not all ask for the same dongles at the start.

Burnout deadlines start with the simulation and reset when a coder starts
compiling. The monitor checks every coder once per millisecond. A coder that has
reached its quota retires and skips its last debug and refactor phases.

## Thread synchronization mechanisms

The simulation uses one mutex, `sim->mutex`, and one condition variable,
`sim->cond`.

- `sim->mutex` protects the result, every coder's deadline and compile count,
  and every dongle's request heap. It is also the output mutex: every line is
  printed while holding it, so two lines can never mix.
- `sim->cond` is used for every wait: phases (compiling, debugging,
  refactoring) and dongle requests. Each wait has a timeout, so a sleeping
  thread also wakes up on its own at the end of its phase, at its burnout
  deadline, or when a dongle's cooldown ends. Releasing dongles and stopping
  the simulation broadcast on it, and every waiter then rechecks its condition.

Each dongle also has its own mutex, as the subject requires. It protects only
the `in_use` flag and the cooldown end time. Lock order is always `sim->mutex`
first, then one dongle mutex; release locks each dongle mutex on its own and
takes `sim->mutex` only afterwards, so the two can never deadlock.

**Start gate.** The main thread holds `sim->mutex` while it creates all
threads. Each new thread blocks on that mutex as soon as it needs it. Once
everything is created, the main thread sets the start time and the first
deadlines and unlocks, so everybody starts from the same clock.

**Logging.** A line is `timestamp coder_number message`, printed with
`printf` while holding `sim->mutex`. Nothing is printed once the simulation has
stopped. The monitor prints `burned out` and sets the result in the same
critical section, so no other line can appear after it. When the output is
redirected to a file or pipe, `printf` buffers it until the program exits;
the timestamps are taken when each event happens.

**Requests.** A request lives on the waiting coder's stack. It is removed from
both heaps before the coder returns from `dongle_acquire`, so the heaps never
point at a dead stack frame.

Timing uses `CLOCK_REALTIME`, the default clock of `pthread_cond_timedwait`.

### Scheduling heaps

Each dongle has a complete binary min-heap built from nodes with parent, left
and right pointers. FIFO compares arrival numbers; EDF compares deadlines and
uses arrival numbers to break ties. Insertion adds a leaf and sifts it up.
Removal moves the last leaf into the gap and repairs the order. Positions are
numbered from 1 (children of `p` are `2p` and `2p + 1`), which finds the next
free leaf or the last leaf in O(log n). Removing a request in the middle of the
heap first finds it with a tree walk in O(n).

## Resources

- [Project subject](en.subject.pdf), version 1.5.
- [POSIX threads overview](https://man7.org/linux/man-pages/man7/pthreads.7.html).
- [POSIX mutex documentation](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_mutex_lock.html).
- [POSIX condition variables](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_cond_wait.html).

AI assisted with the scaffold, coder routine, FIFO/EDF request heap, timing
helpers, simulation lifecycle, monitor, logging, a readability-focused
simplification, and README review and updates.
