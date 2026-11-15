*This project has been created as part of the 42 curriculum by zleullie.*

## Description

Codexion is a C concurrency simulation in which coder threads share USB
dongles to compile, debug and refactor. The assignment requires FIFO or EDF
arbitration through custom heaps, dongle cooldowns and a burnout monitor.

The simulation allocates shared state, starts coder, monitor, and logging
threads, and stops on burnout or completion of all compile quotas. Dongles
use FIFO/EDF request heaps, paired acquisition, shared wakeup broadcasts, and
release cooldowns.

## Instructions

Build with `make` using a C compiler and POSIX threads. Use `make clean` to
remove object files, `make fclean` to remove the executable as well, and
`make re` to rebuild.

Run `make norm` to check sources and headers in [src](src) and [include](include) with
`norminette` installed. Run `make test` for the concurrency and wait regressions.
Tests require Bash, `awk`, `timeout`, and a linker supporting GNU-style
`--wrap`. The wait tests cover cooldown expiry, single-coder burnout, and
shutdown during acquisition and phase waits. The simulation tests check log
format and ordering, adjacent compile exclusion, two acquisitions per compile,
compile quotas, zero-quota output, and several FIFO/EDF workloads. Runtime and
large-thread-count results depend on host scheduling and resource limits.

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

- [include/codexion.h](include/codexion.h): shared types, configuration and function prototypes.
- [src/main.c](src/main.c): program entry point.
- [src/config.c](src/config.c): argument parsing and validation.
- [src/simulation.c](src/simulation.c): initialization, thread lifecycle and cleanup.
- [src/simulation_state.c](src/simulation_state.c): shared primitives and phase waits.
- [src/coder.c](src/coder.c): coder routine.
- [src/dongle.c](src/dongle.c): paired acquisition, availability checks and timed waits.
- [src/dongle_queue.c](src/dongle_queue.c): initialization, paired enqueueing, release and cooldown.
- [src/heap.c](src/heap.c): FIFO/EDF priority queues.
- [src/heap_order.c](src/heap_order.c): heap ordering and repair after removal.
- [src/heap_storage.c](src/heap_storage.c): node allocation and tree traversal.
- [src/monitor.c](src/monitor.c): burnout and completion detection.
- [src/log.c](src/log.c): FIFO logger worker, event formatting and unbuffered output.
- [src/log_queue.c](src/log_queue.c): event enqueueing, logger startup and queue shutdown.
- [src/time.c](src/time.c): timing helpers.
- [tests/wait_regression.c](tests/wait_regression.c): focused acquisition and shutdown tests.
- [tests/regression.sh](tests/regression.sh): simulation and log regressions.

## Blocking cases handled

Workers enqueue one request in both neighboring dongle heaps under the shared
state mutex. A pair is granted only when the request heads both heaps and both
dongles are free and past cooldown. With two distinct dongles, a worker holds
neither while waiting. This breaks Coffman's hold-and-wait condition and
prevents resource deadlock. A single coder instead takes its only dongle and
waits until its deadline or shutdown because no second dongle exists.

FIFO orders requests by arrival; EDF orders them by burnout deadline with
arrival order as the tie-breaker. Inserting each pair within one critical
section keeps relative arrival order consistent across shared heaps. New
requests cannot bypass higher-priority pending requests on a dongle. This
arbitration does not by itself guarantee that every timing configuration can
finish without burnout. Coders with odd zero-based indices initially wait
half a compile duration, rounded down to whole milliseconds, to stagger demand.

Release starts a cooldown on each dongle and broadcasts to waiting threads.
Timed waits allow eligible requests to retry when cooldown expires even if
no further release occurs. Shutdown broadcasts wake both acquisition and
phase waits; pending requests are removed before their stack storage expires.

Burnout deadlines start at simulation launch and reset when a coder starts
compiling. The monitor scans active coders, sleeping one millisecond between
scans. Coders retire after their compile quota and skip the final debug and
refactor waits. The subject requires burnout output within 10 ms; monitor
lock contention, OS scheduling, and output backlog can delay delivery, so the
polling interval alone does not guarantee that bound. A single logger serializes
event output, and publishing burnout under the state mutex prevents new events
from being queued afterward.

## Thread synchronization mechanisms

The simulation owns arrays of coders, dongles, worker contexts, and thread
handles. Worker contexts borrow pointers into these arrays and the simulation's
configuration. All threads are joined before the arrays are freed. Initialization
tracks completed dongle primitives and launched threads to clean up partial
failures safely.

The shared state mutex protects the result, start gate, coder states, burnout
deadlines, completed compile counts, all dongle request heaps, and the log queue.
A start condition variable holds coder and monitor threads until creation
finishes and all initial deadlines share the same start time.
The monitor reads each deadline and count while holding the same mutex used
by workers to update them.

Phase waits use a shared condition variable with a timeout at the earlier of
phase completion and burnout. They release the state mutex while asleep;
shutdown broadcasts on this condition to wake phase and acquisition waiters.
Waiters recheck their predicates after waking. A coder logs its transition
to debugging before releasing either dongle, including after its
final compile, so the log reflects the end of compilation before a neighbor
can begin.

Each dongle's mutex protects only its availability flag and cooldown deadline.
Heap operations and paired grant decisions run under the state mutex. A request
contains a coder pointer and deadline snapshot, with no condition variable of
its own. Its stack address stays valid until it has been removed from both heaps.

Acquisition waits use `phase_condition` with `state_mutex`. The timeout defaults
to the request's burnout deadline. When the request heads both heaps, pending
cooldowns on free dongles can shorten that timeout. Expired cooldowns do not
cause repeated waits on a past deadline. Release and acquisition cleanup
broadcast on the shared condition so waiters can recheck availability and heap
priority. This also wakes phase waiters, which recheck their own end times.

When locks are nested during acquisition, the state mutex comes first, followed
by one dongle mutex at a time. Release updates each dongle under its own mutex,
unlocks it, then takes the state mutex for the broadcast. It never holds a
dongle mutex while acquiring the state mutex. Shutdown sets the result and
broadcasts on the start and phase conditions under the state mutex.

Timing uses `CLOCK_REALTIME` to match the condition variables' default clock;
wall-clock adjustments can affect waits and event timestamps.

### Asynchronous logging

The logger starts before the coder and monitor threads. Its condition variable
is destroyed immediately if logger thread creation fails; cleanup only joins
the logger when startup succeeded.

Callers hold the shared state mutex when enqueueing an event. Each event is a
separately allocated linked-list node containing elapsed milliseconds since
simulation start, a one-based coder number, and the event kind. Head and tail
pointers make enqueueing O(1), and each append signals the logger's condition
variable. Events are ignored once the simulation stops or the queue closes.
The monitor enqueues burnout and publishes the stop result under the same
mutex, preventing later state messages.

The dedicated logger sleeps while the queue is empty and open. It removes
events in FIFO order under the state mutex, then releases the mutex to write
and free each event. Only this thread writes simulation logs, using the format
`elapsed_ms coder_number message`. Timestamps record enqueue time, not output
time. Messages are `has taken a dongle`, `is compiling`, `is debugging`,
`is refactoring`, and `burned out`.

The mutex protects the queue; the output writes themselves occur without a
mutex held. Serialization comes from having one output thread. This prevents
log lines from interleaving, but does not literally follow the subject's
instruction to protect output with a mutex.

After all producers join, shutdown marks the queue closed under the state
mutex and signals the logger. The logger drains pending events before exiting;
shutdown then joins it and destroys its condition variable. Allocation or
output failures set the simulation result to `SIM_ERROR`, and the simulation
reports a fatal error. Unbuffered writes avoid stdio delays when output is
redirected, but OS scheduling and slow output can still delay delivery. The
queue has no fixed capacity, so pending events can accumulate when output is
slower than production.

### Scheduling heaps

The scheduling queue is a complete binary min-heap built from explicit nodes
with parent, left-child, and right-child pointers. FIFO compares request arrival
numbers; EDF compares deadline snapshots and breaks equal-deadline ties by arrival
number. Insertion attaches a new leaf and sifts its entry upward. Removal fills
the gap with the last leaf's entry and repairs heap order. Tree positions are
numbered from 1 to find insertion and removal locations while preserving the
complete-tree shape. This is a heap, not a binary search tree.

Heap entries borrow pointers to requests; moving an entry does not move the
request itself. The state mutex protects heap access and the coder deadline
while taking a request snapshot. Each heap has its own arrival counter, reset
when inserting into an empty heap. Push and pop take O(log n), and removing a named
request takes O(n) to find it plus O(log n) to repair the tree. Destruction pops
all remaining nodes in O(n log n). Node pointers make the tree relationships
explicit, at the cost of extra pointers and one allocation per entry. The logging
queue remains a linked list because the subject's heap requirement concerns
resource scheduling.

## Resources

- [Project subject](en.subject.pdf), version 1.5.
- [POSIX threads overview](https://man7.org/linux/man-pages/man7/pthreads.7.html).
- [POSIX mutex documentation](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_mutex_lock.html).
- [POSIX condition variables](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_cond_wait.html).

AI assisted with the scaffold, coder routine, FIFO/EDF request heap, timing
helpers, simulation lifecycle, monitor, asynchronous logging implementation,
and README review and updates.
