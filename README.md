*This project has been created as part of the 42 curriculum by zleullie.*

## Description

Codexion is a C concurrency simulation in which coder threads share USB
dongles to compile, debug and refactor. The assignment requires FIFO or EDF
arbitration through custom heaps, dongle cooldowns and a burnout monitor.

The simulation allocates shared state, starts coder, monitor, and logging
threads, and stops on burnout or completion of all compile quotas. Dongles
use FIFO/EDF request heaps, targeted wakeups, and release cooldowns.

## Instructions

Build with `make` using a C compiler and POSIX threads. Use `make clean` to
remove object files, `make fclean` to remove the executable as well, and
`make re` to rebuild.

Run `make norm` to check all C sources and headers with `norminette`.
Run `make test` for the reported concurrency, zero-quota, timeout, and wait
regressions. The complete run takes about 35 seconds.

The required command interface to implement is:

```text
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

All eight arguments are mandatory. Times are in milliseconds; `scheduler`
must be exactly `fifo` or `edf`. Reject invalid and overflowing numeric input.
A compile quota of zero succeeds immediately without starting threads or
printing simulation events.

### Source layout

- [include/codexion.h](include/codexion.h): shared configuration and future types/prototypes.
- [src/main.c](src/main.c): program entry point.
- [src/config.c](src/config.c): argument parsing and validation.
- [src/simulation.c](src/simulation.c): initialization, thread lifecycle and cleanup.
- [src/simulation_state.c](src/simulation_state.c): shared primitives and phase waits.
- [src/coder.c](src/coder.c): coder routine.
- [src/dongle.c](src/dongle.c): acquisition, release and cooldown.
- [src/dongle_queue.c](src/dongle_queue.c): request waits and targeted wakeups.
- [src/heap.c](src/heap.c): FIFO/EDF priority queues.
- [src/heap_order.c](src/heap_order.c): heap ordering and repair after removal.
- [src/heap_storage.c](src/heap_storage.c): node allocation and tree traversal.
- [src/monitor.c](src/monitor.c): burnout and completion detection.
- [src/log.c](src/log.c): serialized output.
- [src/log_queue.c](src/log_queue.c): logging queue and logger lifecycle.
- [src/time.c](src/time.c): timing helpers.

## Blocking cases handled

Workers acquire odd-indexed dongles before even-indexed dongles. The wraparound
pair in an odd-sized ring acquires index zero first. This follows a consistent
resource order to prevent circular waits without creating long acquisition
chains. Odd-indexed coder threads initially wait half a compile duration so
alternating seats can acquire disjoint pairs before their neighbors.
A single coder waits for its unavailable second dongle until burnout. The
monitor checks active coders approximately every millisecond; coders retire
after completing their compile quota. On burnout or success, the shared result
stops phase waits and signals every pending dongle request. OS scheduling
can delay monitor checks. EDF orders each dongle's pending requests by deadline;
full liveness across two-dongle acquisition still needs feasibility testing.

## Thread synchronization mechanisms

The simulation owns arrays of coders, dongles, worker contexts, and thread
handles. Worker contexts borrow pointers into these arrays and the simulation's
configuration. All threads are joined before the arrays are freed. Initialization
tracks completed dongle primitives and launched threads to clean up partial
failures safely.

The shared state mutex protects the result, start gate, coder states, burnout
deadlines, and completed compile counts. A start condition variable holds threads
until creation finishes and all initial deadlines share the same start time.
The monitor reads each deadline and count while holding the same mutex used
by workers to update them.

Phase waits use a shared condition variable with a timeout at the earlier of
phase completion and burnout. They release the state mutex while asleep;
shutdown broadcasts on this condition to interrupt every phase immediately.
This avoids frequent polling by hundreds of active coders. A coder logs its
transition to debugging before releasing either dongle, including after its
final compile, so the log reflects the end of compilation before a neighbor
can begin.

Each dongle has a mutex protecting availability, its cooldown deadline, and
its request heap. Each acquisition creates a request with its own condition
variable. The request stays at a stable stack address until it leaves the heap;
its condition variable is destroyed only after removal and unlocking the dongle.
Release signals only the heap's first request. During cooldown, that request
uses a timed wait ending at cooldown expiry or its burnout deadline, whichever
comes first. If cooldown has already expired, its past deadline makes the wait
return immediately and retry acquisition. Timeout and shutdown remove requests
and repair heap order.

When both locks are needed, the dongle mutex precedes the state mutex. Shutdown
publishes the result and releases the state mutex before locking any dongles
to signal every pending request, avoiding an inversion of this order. Timing
uses the realtime clock to match the condition variables' default clock.

Coders enqueue timestamped dongle and phase events under the shared state
mutex. A dedicated logger waits on a condition variable, removes events in FIFO
order, and writes complete lines without holding the state mutex. Only this
thread writes simulation logs. The monitor enqueues burnout and publishes the
stop result while holding the state mutex, preventing later state messages.
After the producers join, shutdown closes the queue and joins the logger after
it drains pending events. Queue allocation or output failures report an error
and stop the simulation. Unbuffered writes avoid stdio delays when output is
redirected; OS scheduling and slow output can still delay delivery.

The scheduling queue is a complete binary min-heap built from explicit nodes
with parent, left-child, and right-child pointers. FIFO compares request arrival
numbers; EDF compares deadline snapshots and breaks equal-deadline ties by arrival
number. Insertion attaches a new leaf and sifts its entry upward. Removal fills
the gap with the last leaf's entry and repairs heap order. Tree positions are
numbered from 1 to find insertion and removal locations while preserving the
complete-tree shape. This is a heap, not a binary search tree.

Heap entries point to requests rather than owning or moving condition variables.
The dongle mutex protects heap access; the state mutex protects the coder deadline
while taking a request snapshot. Push and pop take O(log n), and removing a named
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
helpers, simulation lifecycle, monitor, and asynchronous logging implementation.
