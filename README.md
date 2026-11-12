*This project has been created as part of the 42 curriculum by zleullie.*

## Description

Codexion is a C concurrency simulation in which coder threads share USB
dongles to compile, debug and refactor. The assignment requires FIFO or EDF
arbitration through custom heaps, dongle cooldowns and a burnout monitor.

This repository currently contains only a buildable boilerplate. Parsing,
threading, heaps, cooldowns, monitoring and logging remain to be implemented.
The executable prints a placeholder message and exits with status 1.

## Instructions

Build with `make` using a C compiler and POSIX threads. Use `make clean` to
remove object files, `make fclean` to remove the executable as well, and
`make re` to rebuild.

Run `make norm` to check all C sources and headers with `norminette`.

The required command interface to implement is:

```text
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

All eight arguments are mandatory. Times are in milliseconds; `scheduler`
must be exactly `fifo` or `edf`. Reject invalid and overflowing numeric input.

### Source layout

- [include/codexion.h](include/codexion.h): shared configuration and future types/prototypes.
- [src/main.c](src/main.c): program entry point.
- [src/config.c](src/config.c): argument parsing and validation.
- [src/simulation.c](src/simulation.c): initialization, thread lifecycle and cleanup.
- [src/coder.c](src/coder.c): coder routine.
- [src/dongle.c](src/dongle.c): acquisition, release and cooldown.
- [src/heap.c](src/heap.c): FIFO/EDF priority queues.
- [src/monitor.c](src/monitor.c): burnout and completion detection.
- [src/log.c](src/log.c): serialized output.
- [src/time.c](src/time.c): timing helpers.

## Blocking cases handled

Pending implementation. Document deadlock prevention and Coffman's conditions,
starvation prevention, cooldown handling, precise burnout detection, the
single-coder case and log serialization here once implemented.

## Thread synchronization mechanisms

Pending implementation. The subject requires a mutex for each dongle's state
and serialized logging. Document the chosen mutexes, condition variables,
shared stop state, lock ordering and coder/monitor communication here once
implemented, including examples of how races are prevented.

## Resources

- [Project subject](en.subject.pdf), version 1.5.
- [POSIX threads overview](https://man7.org/linux/man-pages/man7/pthreads.7.html).
- [POSIX mutex documentation](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_mutex_lock.html).
- [POSIX condition variables](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_cond_wait.html).

AI was used to read the subject and generate the initial Makefile, source
layout, configuration type and documentation skeleton. No simulation or
scheduling algorithms were implemented by AI in this scaffold.
