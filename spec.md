# AtomicX 2 — Specification (SDD), v2.0.0.proto

**What AtomicX 2 is.** AtomicX 2 is a C++11 library for **cooperative multithreading** on small microcontrollers
(AVR such as the Arduino Nano, ESP8266, ESP32, STM32) and on computers (Linux, macOS). *Cooperative* means a thread
runs until it gives the processor back by itself; nothing interrupts it. A thread is an ordinary object: you derive a
class from `ax::thread_item`, write its `run()`, and construct it with an `ax::thread_context`. Constructing the
object registers the thread in the context; destroying it unregisters it; nothing else has to be called. AtomicX 2 is
the successor of atomicx v1.3.0 (github.com/solariun/atomicx), whose threads switch by copying each thread's part of
the stack in and out of a buffer (§3.6).

**Status (2026-10-31): prototype, version `2.0.0.proto`, rebuilt from scratch since 2026-10-09; the last commit before
this sync is `042c11d` (2026-10-29).** Built: the self-registering list (`auto_obj_list`, with `swap` and a stable
insertion sort), threads that register themselves and own a stack buffer (`thread_item`), the context that holds them
(`thread_context`), the schedule order that ranks them by state and by time (`thread_item::compare`), and the
`TRACE` and `LOG_ERROR` macros (A15, A16). In progress: the context switch (`yield()`, `start()`, `save`, `restore`,
§3.5), a first cut of atomicx v1's stack copy with the scheduling inside `yield()` (A12). Every thread's `run()` is
started from `start()`'s frame; a paused thread is resumed by a `longjmp` into the `yield()` that chose it, whose
restore rewrites the stack with that thread's slice. The demo's threads take turns (§6), but the switch works only
because every thread's frames sit at the same addresses (P24); the overflow check uses a size 8 times the buffer
(P27), and `_stk_top` marks a variable whose block has ended (P33). Not built: the `NOW`, `SLEEPING` and `WAITING`
states, which are ranked but never set. Build state: `make build` and `make build TRACE=1` are clean under
`-Werror`, and the demo runs until stopped (§3.5); for the ESP32 and the Nano, `arduino/simple` compiles and links
(P1, P3 and P30 fixed; not run on a board yet, P36). The earlier prototypes (2024) stay in the history; the last one
is commit `683195f`.

This file is normative: every change to behaviour, API, file layout or tests is recorded here first (Spec-Driven
Development, §0.3). Where this file and the code disagree, the disagreement is a defect to be resolved here first.

**Repository:** github.com/solariun/atomicx2. **Author:** Gustavo Campos. **License:** undecided (P14).

### Changelog

| Date | Change |
|---|---|
| 2024-02-25 to 2024-09-24 | **Earlier attempts** (`5fd1afe` to `969611b`): a `thread` class that registers itself on a static list (`5fd1afe`); the "first thread engine" (`09bf6dd`, 2024-02-29), in namespace `atomicx`: `setjmp`/`longjmp` with a save and a restore of the stack slice and one global context `g_ctx`; restarted on 2024-09-23 ("new version prototype started", `e1d6c12`) with `Context` and `Thread` classes (`edb369f`), each thread taking its own context and a program-given buffer of `size_t` words (`b834acc`). |
| 2024-09-24 to 2024-11-02 | **The first running prototype** (`8611af1` to `683195f`): threads switched with `setjmp`/`longjmp` and a copy of each thread's used stack slice, as atomicx v1 does; stack buffers given by the program through `VMEM`; a context per thread at first, one global context `ctx` from `9b0ea0d` (namespace `ax` from `ef0b9da`); a scheduler that ran `READY` threads first, then the sleeping thread due first (the "momentum scheduler" of `1539f25`); `wait`/`notify` keyed by a variable's address and a channel, with tags and timeouts, and a system channel 255 (`ATIMICX_SYS_CHANEL`) used inside. A PC demo and an Arduino Nano demo. |
| 2026-10-09 | **Redesign started** (`8607977`, "redesign: initial"): the implementation removed (`atomicx.cpp` reduced to its includes and an empty namespace, 351 lines deleted; 649 lines deleted over five files, the PC demo included); a new skeleton: the self-registering list `auto_obj_list`, `thread_context` objects passed to each `thread_item`, stack buffers on the heap, `enum class State`. The Arduino sketch kept the old API. The header failed `-Werror` (an unused variable). |
| 2026-10-10 | `710c462`: `Time` renamed `time` and `RefId` renamed `ref`; `getNextItem` and `getPrevItem`; the links made protected. The header did not compile (an unfinished struct in `thread_item`). |
| 2026-10-12 | `8eea8f3`: the stable merge sort `auto_obj_list::sort` (no STL, no recursion); `st_data` (`lastRunTime`, `nice`, `state`), which made the header compile again; the stack pointers renamed `_v_stackPointer` and `_l_stackPointer`; the sketch's threads, `setup()` and `loop()` removed. |
| 2026-10-13, `fa67484` | **`start()` runs the first thread** (Gustavo): `thread_context` moved after `thread_item`, hence A4; `start()` returns false when `running` is set and calls the first item's `run()` through a cast to `thread_item*` (R3.9, R3.11, P11), where at `8eea8f3` it ran nothing and returned true on every call; the comment "BY DESIGN would never return…" added (R3.10). |
| 2026-10-13, `fa67484` | **Thread construction fixed (A2, A3, A4).** `class thread_context;` is declared before `thread_item` (Gustavo), and the `thread_item` constructor is defined where `thread_context` is complete, because `item(parent)` must convert `thread_context&` to `auto_obj_list&`. The stack buffer comes from `new (std::nothrow)`, so the failure branch can run on every target; the constructor's parameter is `stack_size`, so `stackSize = 0` resets the member. Measured on the PC: a request of 2^61 bytes aborted the program (`std::bad_alloc`, exit 134) before, and is reported and survived (buffer null, `stackSize` 0, exit 0) after (§6). |
| 2026-10-13, `fa67484` | **Declarations in the header, definitions in the source (A1).** The 15 non-template member function bodies moved from `atomicx.h` to `atomicx.cpp`, in declaration order, their text unchanged (only shifted to namespace level); the template `sort` stays in the header; `#include <new>` moved with the constructor. Proof: the moved bodies byte-identical to Gustavo's working copy, `make build` clean with `-Werror`, the demo's output unchanged. Cost on the ESP32: +184 bytes with the probe sketch of §6, and +251,641 bytes of flash and +7,132 bytes of RAM in a sketch that uses nothing of AtomicX, because the out-of-line definitions bring the C++ stream library in (P17). |
| 2026-10-13, `fa67484` | **`volatile` list changes and a typed parent (A5, Gustavo).** `add` and `remove` became `volatile` member functions, `item::parent` a `volatile auto_obj_list&`, and `thread_item` gained `_parent` (`volatile thread_context&`); the stack buffer is allocated in the initializer list. The initializer list was then written in construction order (R3.1), which `-Werror` requires (`-Wreorder-ctor`). |
| 2026-10-13, `fa67484` | **`thread_item`'s data made private; `yield()` declared (A6, Gustavo).** Every data member of `thread_item` is now private, and a private `void yield()` is declared and defined empty. `make build` then failed under `-Werror`: `-Wunused-private-field` on `_l_stackPointer` and `_parent` (P20); `fa67484` includes this edit, although its message says that `make build` is clean. |
| 2026-10-13, `fa67484` | **This specification created** (the commit holds its first version): the code as built, the earlier history, atomicx v1's thread switch as the reference (§3.6), and the open problems found while writing it (§7), each with the evidence given in its row. Reviewed independently the same day (claims against the code and the history, v1's citations, diagrams, tables and cross-references); its 32 findings corrected in `1245c3b`. |
| 2026-10-14, `1245c3b` | **The schedule order, applied by `yield()` (A7, A8, Gustavo).** `thread_item::compare(a, b, data)`, public and static, orders a context for `sort`: `READY` threads first, then due threads (`lastRunTime + nice` not after now, wrap-safe), ties in list order; now is read once per sort, from `data->_data.tm_now`, which `yield()` sets from `getTick()` before sorting. `sort` became `sort(cmp, data)`, passing `data` to `cmp` as a third argument; `sort` and `count` became `volatile`. `st_data` became public and gained `tm_now`, and starts at `lastRunTime = 0`, `nice = 1` (P6 fixed). `dump_threads()` lists the context's threads; `yield()` became protected; `State` gained `WAITING`; `main.cpp`'s `run()` calls `yield()`. `make build` clean again (P20 fixed). The library now calls `getTick()` (R4.1). |
| 2026-10-14, `d8bb034` | **The stack as bytes (A9, Gustavo).** `_v_stackPointer` and `_l_stackPointer` became `volatile char*`, and the buffer `new (std::nothrow) volatile char[stack_size]`: `stack_size` bytes, while `stackSize` still multiplies by `sizeof(size_t)` (P27). `yield()` marks the live stack with a `volatile char` on its own frame. `_parent` lost its `volatile` (A5). `main.cpp` calls `myThreadContext.start()`. |
| 2026-10-17, `2f9a027` | **`yield()` traced (A10, Gustavo).** `yield()` prints each step (`[YIELD]`) and dumps the context before and after the sort; `dump_threads()` first prints the yielding thread's address, its stack pointers, `tm_now`, `nice`, `lastRunTime`, its state, and the context's address with its head and tail (`[DUMP]`). |
| 2026-10-17, `fcdb0d0` | **First cut of the stack-copy switch (A9, Gustavo, work in progress).** `yield()` takes the head of the sorted context as the next thread, records it in `thread_context::_current`, calls `setjmp` on its `jmp_buf`, marks its stack bottom, copies the stack slice into the yielding thread's buffer, and calls the next thread's `run()` when it is `READY`; any other state prints an error and exits. `start()` marks `_stk_top` on its own frame and sets the first thread `RUNNING`. `save` and `restore` became `memcpy` in each direction. `_l_stackPointer` renamed `_stk_bottom`, `_parent` renamed `_context`; new members `jmp_buf _register_context`, `_current`, `_stk_top`. `compare` counted `NOW` as ready and returned true whenever either thread was ready; `State` gained `NOW`. |
| 2026-10-22, `93feabe` | **The schedule rank (A7, Gustavo).** `compare` ranks by state first, from one `switch` over every `State` with no `default`: `NOW` and `READY`, then `RUNNING`, then `SLEEPING` and `WAITING`, then `TERMINATED`, always last; then due before not due; ties in list order. It replaces `fcdb0d0`'s rule, which was no strict weak ordering. Proof (§6): Gustavo's case (three `RUNNING`, one `TERMINATED`) `TYZX` before, `YZXT` after; one thread of each state `RNKWSU` before, `RNUWSK` after; 0 ordering violations in 2000 random rounds. |
| 2026-10-22, `93feabe` | **`dump_data(data, size, label)` (A10).** A function at the start of namespace `ax` in `atomicx.cpp` that prints memory as a hex dump: an optional `[label]:` line, then 16 bytes per line in two blocks of 8 and the same bytes as text, with `.` for every byte that is not printable. Written on Gustavo's request; Gustavo added `label` and calls it from `yield()` to show `jmp_buf` contents. Proof: 4 output tests pass byte for byte. |
| 2026-10-22, `93feabe` | **`yield()` goes on after a thread ends (A9, Gustavo, work in progress).** A thread whose `run()` returns is marked `TERMINATED` and the choice starts over (`goto start`); a `RUNNING` next thread is resumed with `longjmp` on its `jmp_buf`; the second return of `setjmp` restores the stack copy. As measured, the `longjmp` lands in the same `yield()` call that has just filled that `jmp_buf`, so no thread is resumed (P24). |
| 2026-10-22 | **This specification synced** with `1245c3b` to `fcdb0d0` and the uncommitted changes: decisions A3 and A5 to A10, sections 2.2, 2.3 and 3 rewritten for the code as built, §3.5 the switch in progress, the proposed switch in §3.6 (waiting for Gustavo, D2), the evidence of 2026-10-14 to 2026-10-22 (§6, AddressSanitizer included), P20 and P6 fixed, P23 to P29 new. |
| 2026-10-23, `93feabe` | **`setNice()`, and `yield()` sleeps until the chosen thread is due (A11, Gustavo).** `thread_item::setNice(ax::time)` (protected) sets the thread's `nice`. When the chosen thread is `RUNNING`, `yield()` computes the ticks left until it is due, `lastRunTime + nice - getTick()` in `ssize_t`, sleeps that long with `sleepTicks()` when it is positive, then `longjmp`s; after the restore it stamps the chosen thread's `lastRunTime` with `getTick()`. `dump_threads()` prints each thread's `nice` and `lastRunTime`; the dump after the sort and the `jmp_buf` dumps are commented out. `main.cpp`: four threads of 500 bytes with nice 500, 1000, 2000 and 5000 ticks and a counter each (100 to 400), whose `run()` prints its counter and yields, forever. *Measured* (§6): only the last thread started keeps running (P24, P25); slices reach 640 bytes for 500-byte buffers (P27); the sleep is not wrap-safe and `ssize_t` does not exist on AVR (P30). |
| 2026-10-23 | **This specification synced** with the changes of 2026-10-23: A10 and A11, §1, §3.2, §3.5, §3.6, §4, §5 and §6 updated; P23, P24, P25 and P27 re-measured; P30 and P31 new. |
| 2026-10-31 | **`TRACE(…)`: compiled out without `TRACE_ON` (A15, Gustavo).** Gustavo's macro was `(void)0` when `TRACE_ON` is not defined, so `TRACE << …` in `yield()` did not compile and `make build` failed. It is now a function-like macro, `TRACE(a << b);`: without `TRACE_ON` it becomes `do { } while (0)` and its arguments disappear before compiling; with it, it prints `[TRACE] file:line - ` and the message. `yield()`'s one call changed to the new form. Proof (§6): on the PC without `TRACE_ON`, no output, the arguments never evaluated, a call naming an undeclared identifier compiles; with it, the same lines as before; inside an `if` with an `else`, both ways; avr-g++ 7.3 compiles and links the test without `TRACE_ON`; `make build` and a `-DTRACE_ON` build clean under `-Werror`; preprocessed, `yield()`'s call is `do { } while (0);`. |
| 2026-10-31 | **Every `std::cout` through `TRACE(…)`; `make build TRACE=1` (A10, A15, Gustavo).** The 23 one-line `std::cout` statements of `atomicx.cpp` (`dump_threads()`, `yield()`, and the commented-out traces) became `TRACE(…)`; `dump_data` builds each line in a buffer and traces it once, and tests 0x20 to 0x7e instead of calling `isprint()` (`<ctype.h>` removed). The Makefile adds `-DTRACE_ON` when `TRACE=1` is given, to the native build only; `make help` lists it. `std::cerr` stays for the two errors (allocation failure, stack overflow). Proof (§6): `make -n` shows `-DTRACE_ON` only for `make build TRACE=1`; the demo prints 0 `[TRACE]` lines without it and 171 in 6 s with it, the same thread order both ways; `dump_data`'s output equal, byte for byte, to the old one once the prefix is removed, over 6 inputs and all 256 byte values; at `-Os` without `TRACE_ON`, `dump_data` compiles to a single `ret`; `make build` and `make build TRACE=1` clean under `-Werror`. |
| 2026-10-31 | **`TRACE=1` for the local build and for Arduino; off by default (A15, Gustavo).** The Makefile's `TRACE_FLAGS` (`-DTRACE_ON` when `TRACE=1`) goes into `CFLAGS` and, as `--build-property "compiler.cpp.extra_flags=-DTRACE_ON"`, into `make nano`, `make nano_flash` and `make nodemcu`; `make help` says so. On Arduino, `TRACE` prints through `serial_stream`, which hands each piece to `Serial.print`; the macro ends the line itself (`std::endl` on the PC, `Serial.println()` on Arduino), so the 26 `std::endl` and the 3 `std::flush` came off the calls. Proof (§6): `make -n` shows the flag only with `TRACE=1`, for both kinds of target; the PC demo with `TRACE=1` prints the same lines as before (addresses and line numbers masked) and none without it; `dump_data` still equal to the original; the adapter compiles for the Nano with every argument type the library passes; `arduino/simple` compiles for the ESP32 with and without `TRACE=1`, `[TRACE] ` in the image only with it; for the Nano it still stops at `atomicx.h`'s `<iostream>` either way (P1). |
| 2026-10-31 | **The Nano builds (A16, Gustavo: "revisit the arduino code and test the building for a nano").** `atomicx.h` no longer includes `<iostream>`, which it never used. The two errors print through `LOG_ERROR(…)`, always compiled: `std::cerr` on the PC, with the same text as before, and `Serial` on Arduino, through `serial_stream` (`serial_stream` renamed, as it now serves both macros). `yield()`'s sleep takes the distance to the deadline modulo the tick range, as the schedule order does (R3.16), instead of in `ssize_t`, which AVR does not have (P30 fixed). `arduino/simple` sizes its thread stacks with `k_stack_size`: 64 bytes on AVR, 500 elsewhere. Proof (§6): the Nano build failed at `<iostream>`, then at `std::cerr`, `std::endl` and `ssize_t`; it now compiles and links with and without `TRACE=1`, with no warning from AtomicX or the sketch; the ESP32 build too, 254,605 bytes of flash and 7,140 bytes of RAM smaller; the sleep equal to the old one in the normal cases and right across the wrap; the PC demo's schedule unchanged; `make build` and `make build TRACE=1` clean. |
| 2026-10-31 | **`main.cpp`'s counters start at 1000 to 8000 (Gustavo).** The four `MyThreadItem` threads count from 1000, 2000, 3000 and 4000, the four `MySecThreadItem` threads from 5000, 6000, 7000 and 8000; stacks and nice unchanged. |
| 2026-10-31 | **This specification synced** with `e6e9d97` (P35 fixed) and with the changes of 2026-10-31: the status, §3.2, §3.3, §3.5, §4 and §5 rewritten for the code as built; P24 and P26 updated, P25 and P32 fixed. |
| 2026-10-31 | **The CI builds the project (A17, P13).** `.github/workflows/c-cpp.yml` replaced: the PC builds on Ubuntu and macOS, `TRACE=1` and `SYMBOLS=1` included, a 5-s demo run for information, and the Nano build of `arduino/simple` with and without `TRACE_ON`. *Measured* before: run 41239665284 (`de25d6a`) failed at its first step, "./configure: No such file or directory", exit 127, as had every run since `8607977`. |
| 2026-10-29, `042c11d` | **Debug symbols with `SYMBOLS=1` (A13, Gustavo).** The Makefile adds `-g` only when `SYMBOLS=1` is given, and `make debug` always gives it; `make help` lists it. VS Code: `.vscode/tasks.json` holds the task `make build SYMBOLS=1`, which `.vscode/launch.json` runs before the debugger (it named the task, which did not exist). Proof: in a scratch copy, `atomicx.o` carries DWARF debug information (`__debug_info`) with `SYMBOLS=1` and none without; `make -n` shows `-g` only then. |
| 2026-10-29, `042c11d` | **`save` and `restore` copy the slice below the top (P23 fixed, Gustavo).** The stack grows down, so a thread's slice is the `stackSize` bytes **below** `stackTop`, `[stackTop - stackSize, stackTop)`, which `yield()`'s arguments make `[_stk_bottom, _stk_top)`; both functions copied the bytes above it, `[stackTop, stackTop + stackSize)`: the rest of `start()`'s frame and `main()`'s thread objects. Only the two `memcpy` changed; the signatures and every call are as before ("change only the save and restore"). It is atomicx v1's copy, from the low end (`atomicx.cpp:413` and `422-429`). *Measured* (§6), the demo for 12 s with the list checked after every `restore`: before, the list corrupt after the first `restore` and nothing printed after 401; after, 112 restores with the list intact every time, and the four printing threads take turns, each counter going up by one. AddressSanitizer still stops the first `save`, now just past `yield()`'s `dummy`: the slice crosses the redzones it puts around locals, so it cannot check a stack-copy switch. |
| 2026-10-29, `042c11d` | **`swap` and the swap sort (A14, Gustavo).** `auto_obj_list::swap(a, b)` (public, `volatile`) relinks two items of a list; `sort` became a stable insertion sort built on it, and its doc comment now describes the three-parameter `cmp` (P28 fixed). Gustavo's suspicion that the merge sort lost the list's end was tested: it did not; the corruption seen in the demo comes from `restore`, which writes back stale bytes over a thread object's `next` and `prev` (P23, *measured*). Proof (§6): 14 swap cases and 20,000 random swap rounds against a model; 20,000 random sorts compared with `std::stable_sort`, every link checked; the schedule order's cases through the new sort; `-Werror` clean. |
| 2026-10-23, `f656da0` | **Earliest deadline first (A7, Gustavo).** Between threads of the same rank, `compare` now orders by deadline, `lastRunTime + nice`, the earliest first and the most overdue first, wrap-safe (R3.16): each deadline is keyed as `deadline - now + 2^31`, so a plain `<` orders them across the tick wrap. It replaces "due before not due", which put no order among threads that were all due or all not due. Gustavo's report: four `RUNNING` threads with nice 2000, 1000, 500 and 5000, none due, stayed in list order after the sort; the thread with nice 500, due next, should come first. The new order refines the old one: a due thread's deadline is always before a not-due thread's, so only the ties of the old rule change. Proof (§6): Gustavo's dump sorted 500, 5000, 1000, 2000; §3.4's example, the wrap cases and Gustavo's earlier case unchanged; 0 ordering violations in 2000 random rounds. P22 narrowed. |
| 2026-10-26, `a88952c` | **Fewer traces, `getNice()`, and eight threads in the demo (A10, A11, Gustavo).** Most of the traces are commented out: the "Added…", "Removed…" and "Allocated stack…" lines of the constructors and destructors (the allocation failure still prints on `std::cerr`), `dump_threads()`'s lines about the yielding thread and its context, and every `[YIELD]` step but two, "Stack saved for thread at address: …" after the `save` and "Context restored." after the restore; `yield()` no longer calls `dump_threads()`. `dump_threads()` now prints a header line, one ` * [DUMP]` line per thread and a separator. New: `thread_item::getNice()` (protected, `const`) returns the thread's `nice`. `main.cpp`: the four `MyThreadItem` threads no longer print; four `MySecThreadItem` threads, with the same stacks, nice and counters, print their counter, which now counts up, and their `nice`, and call `yield()` from a member function one call deeper than `run()`. *Measured* (§6): unchanged behaviour, only the last thread started keeps running (P24, P25). |
| 2026-10-28, `ef5eeee` | **A way back to `start()` ("provisioning commit", Gustavo).** `thread_context` gains `jmp_buf _top_context`, and `thread_item` a `_stk_top` of its own, never set. `yield()` marks its **own** `_stk_bottom` (the slice's size still used the head's); the restore reads the buffer and the size from `_current`. `start()` sorts the context before choosing the first thread, stamps its `lastRunTime` (P31 fixed), calls `setjmp` on `_top_context` and returns false. *Measured*: it does not compile ("use of undeclared identifier 'set_jmp'" and "'dummy'", used before its declaration). |
| 2026-10-28 (not kept) | **A kernel loop in `start()`** (written by Claude on Gustavo's request, then replaced by Gustavo, A12). `start()` looped over the schedule order, started each new thread through a never-inlined `launch()` that set `_stk_top` and jumped back to the kernel when `run()` ended, and resumed a paused thread with `longjmp`; `yield()` saved its own slice and registers and jumped to the kernel, and restored its slice reading only a static pointer. *Measured*: the demo's threads took turns at their `nice` rates (in 12 s, 23, 12, 6 and 3 lines for nice 500, 1000, 2000 and 5000); three threads that yielded 3 to 5 times kept their locals and ended, at `-O0`, `-O2` and `-Os`; slices of 129 to 161 bytes. The same working copy's `SYMBOLS=1` option in the Makefile and its fixes to `save`, `restore` and `stackSize` were not kept either. |
| 2026-10-28, `bc919db` | **The scheduling stays in `yield()` (A12, Gustavo).** `start()` sets `_stk_top` to a `volatile char` declared in an inner block, then calls `setjmp` on `_top_context` and runs the sorted head; when a `longjmp` lands there, it returns false (`goto error`). `yield()` computes the slice from its own `_stk_bottom`, compares it with the buffer (`stackSize` renamed `_stackSize`) and, on an overflow, prints "[YIELD] *** STACK OVERFLOW ***…" and `longjmp`s to `_top_context`; a `RUNNING` head, after the sleep, and a head in any other state now `longjmp` to `_top_context` too, where they `longjmp`ed to the head's `jmp_buf` or exited. `main.cpp` prints "Main thread finished." after `start()`. *Measured* (§6): eight threads started, nested; 100, 200, 300 and 400 printed once each; `start()` returns after about 0.5 s (P32); AddressSanitizer still stops the first `save` (P23, P33). |
| 2026-10-28, `e6e9d97` | **"saving": a `READY` head starts from `start()`, a `RUNNING` head is resumed by `longjmp` (A12, Gustavo).** `yield()` no longer calls a `READY` head's `run()`: it `longjmp`s to `_top_context` with 1, and `start()`, whose `setjmp` returns 1, runs the new head from its own frame, so threads no longer nest (P25 fixed). A `RUNNING` head, after the sleep, is resumed by a `longjmp` to its own `_register_context`; an overflow `longjmp`s to `_top_context` with 3 and any other state with 2, and `start()` returns false only for values above 1 (P32 fixed). The restore's size uses `_context._stk_top`; `thread_item::_stk_top` is removed; `count()` returns a counter, `_count`, kept by `add` and `remove`; `arduino/simple/simple.ino` gets `setup()` and `loop()` back (P3 fixed). *Measured* (2026-10-29, §6): 100, 200, 300, 400 and 401 printed, then nothing: the first restore corrupted the list (P23). |

### How to read this document

- Every section starts **in plain words**, often with a picture; the **exact rules** follow.
- **As built** describes the code today. **Work in progress** marks code that exists but does not do its job yet; it
  is described, not normative. **Planned** and **proposed** describe intentions that have no code yet; they are not
  normative for the code until they are built and this file says so.
- "Must", "never" and "always" are normative. Names in `code` are the ones in `atomicx/atomicx.h`; rules are numbered
  (R1.1 …), decisions (A1 …), open problems (P1 …).
- Values marked *measured* come from runs (§6). Every term is defined in the glossary (§8).

---

## 0. Decisions

### 0.1 The idea in one picture

```mermaid
flowchart LR
    subgraph program["your program"]
        a["blink: a class derived from ax::thread_item<br/>its run() is the thread"]
        b["serial: another one"]
    end
    ctx["ax::thread_context<br/>the list of threads"]
    a -->|its constructor registers it| ctx
    b -->|its constructor registers it| ctx
    ctx -->|start runs the threads| a
```

**In plain words.** You never call "create thread" or "add thread". Making the object is enough: its constructor puts
it in the context's list, and its destructor takes it out. The context then runs the threads one after the other,
each one until it hands the processor back by calling `yield()` (cooperative). The order in which they run is the
schedule order (§3.4); the switch from one thread to the next is work in progress (§3.5).

### 0.2 Decisions

| # | Decision | In plain words: why |
|---|---|---|
| A1 | **The header declares, the source defines** (Gustavo, 2026-10-13). `atomicx.h` holds declarations, macros, type aliases, enums, data members with their default initializers, deleted and pure virtual functions, and templates with their bodies (`sort`). Every other member function is defined in `atomicx.cpp`, in the order of its declaration, its text kept as written. | Gustavo asked for the member function bodies in the source and only declarations and templates in the header, keeping the same formatting and writing structure. The interface reads in one place, and the definitions see every class complete, so the order of the classes in the header no longer matters for them (A4). Rejected: a header-only library (bodies inside the classes), by Gustavo's instruction. Not chosen, open to revisit: definitions back in the header. *Measured* on the ESP32: the split costs 184 bytes with the probe sketch of §6, and 251,641 bytes of flash and 7,132 bytes of RAM in a sketch that uses nothing of AtomicX, because the out-of-line definitions bring the C++ stream library in (P17). Making only the one-line getters `inline` would recover nothing: the sketch compiles `atomicx.cpp` inside its own unit, where they are inlined anyway. |
| A2 | **The stack buffer comes from `new (std::nothrow)`** (Gustavo, 2026-10-13). When it fails, the constructor prints `Failed to allocate stack for thread item.` with `LOG_ERROR` (`std::cerr` on the PC, `Serial` on Arduino, A16) and sets `stackSize` to 0; the thread stays registered. | On the PC (*measured*: exit 134) and on the ESP32 (its core builds with `-fexceptions`), plain `new` throws `std::bad_alloc` when it fails, so the old `if (!_v_stackPointer)` branch could never run there. The Arduino AVR core (the Nano, `-fno-exceptions`) returns null from plain `new` instead (`cores/arduino/new.cpp`, where `NEW_TERMINATES_ON_FAILURE` is left undefined), which the standard does not allow. `new (std::nothrow)` returns null on all three, so the failure branch is portable. Rejected: letting it throw. Open: what a thread without a buffer should do (P7), and whether the buffer should come from the program instead of the heap (P16). |
| A3 | **The constructor's parameter is `stack_size`; the member `stackSize` holds the buffer's size** (Gustavo, 2026-10-13). Since A9 the buffer holds `stack_size` bytes. | The parameter used to be called `stackSize` too, so `stackSize = 0` on failure changed the parameter and the member kept its size (*measured*: with `new (std::nothrow)` and the old name, the member kept 2^61; with `stack_size`, 0). The parameter follows Gustavo's snake_case; the member keeps its name. Rejected: keeping the name and writing `this->stackSize = 0` (the shadowing stays a trap). As built, `stackSize` is still `stack_size * sizeof(size_t)`, a word count from before A9: 8 times the buffer on a 64-bit PC (P27). |
| A4 | **`class thread_context;` is declared before `thread_item`, and the `thread_item` constructor is defined where `thread_context` is complete** (since A1: in `atomicx.cpp`). | Gustavo moved `thread_context` after `thread_item`: its `start()` calls `run()` on a `thread_item`, which needs `thread_item` complete while the body is in the class. Then `thread_item`'s constructor names `thread_context` before its definition. A forward declaration gives only the name: it cannot say that `thread_context` derives from `auto_obj_list`, and `item(parent)` needs exactly that conversion, so that body cannot sit inside the class (*measured*: "cannot convert argument of incomplete type 'thread_context' to 'auto_obj_list &'"). Rejected: `thread_context` first, as at `8eea8f3`; possible again since A1, but not needed. |
| A5 | **`add` and `remove` are `volatile` member functions, and `item::parent` is a `volatile auto_obj_list&`** (Gustavo, 2026-10-13). `thread_item` keeps its context with its own type in `_context`, a plain `thread_context&` since `d8bb034` (it was `volatile thread_context& _parent` in `fa67484`). | *Reason to be recorded by Gustavo* (P19). Consequence (*measured*): through `parent`, only `volatile` member functions can be called (`add`, `remove`, `count`, `sort`). `volatile` gives no atomicity (R2.4). |
| A6 | **`thread_item`'s data members are private; `yield()` is protected** (Gustavo, 2026-10-13; `yield()` protected since `1245c3b`). `st_data`, `compare` and `dump_threads` are public. | *Reason to be recorded by Gustavo* (P19). Consequence: a class derived from `thread_item` calls `yield()` from its `run()` but cannot reach the buffer, `stackSize`, `_data` or `_context`. |
| A7 | **The schedule order: `thread_item::compare(const item& a, const item& b, const thread_item* data)`** (Gustavo, 2026-10-14, 2026-10-22 and 2026-10-23), public and static, for `auto_obj_list::sort`. First key, the rank of the state: `NOW` and `READY`, then `RUNNING`, then `SLEEPING` and `WAITING`, then `TERMINATED`, always last. Second key, between threads of the same rank: the deadline, `lastRunTime + nice`, earliest first, relative to now (`data->_data.tm_now`) and wrap-safe (R3.16), so the most overdue thread comes first and, when none is due, the one due next. Ties keep the list's order. | Gustavo: "now \| ready first, running, sleep \| waiting, terminated (always last)"; and whether nice plus the last run time goes beyond now is the next order; then (2026-10-23) the thread that will be due next must come first. The rank comes from one `switch` with no `default`, so a new `State` does not compile until it is ranked (`-Wswitch`, an error under `-Werror`, *measured*). Now comes from the caller, read once per sort (`yield()` sets `tm_now` before sorting). The wrap-safe test: `ax::time` is a `uint32_t` that wraps (2^32 ms is about 49.7 days), and the naive `lastRunTime + nice <= now` misorders threads around the wrap (*measured*, §6). Rejected: `fcdb0d0`'s rule, true whenever either thread was ready, which is no strict weak ordering (*measured*: thousands of violations, a reversed tie); a two-level ready or not-ready rank, under which a due `TERMINATED` thread can come first (*measured*: `TYZX`); `93feabe`'s second key, due before not due, which leaves threads that are all due or all not due in list order (*measured*: Gustavo's four `RUNNING` threads, none due, nice 2000, 1000, 500 and 5000, unchanged by the sort). Open (P22): equal deadlines still keep the list's order instead of taking turns. |
| A8 | **`yield()` sorts its context by the schedule order** (Gustavo, 2026-10-14): it sets `_data.tm_now = getTick()`, then calls `_context.sort(compare, this)`. `sort` and `count` are `volatile` member functions. | Gustavo asked for the sort in `yield()`. `sort` and `count` were made `volatile` so that `yield()` could call them through the `volatile` `_parent` of the time (A5); since `_context` is not `volatile` (`d8bb034`), that is no longer needed, but both still work on lists that are not `volatile`. Rejected then: casting `volatile` away (`const_cast`). |
| A9 | **The stack is handled as bytes, and `yield()` switches threads by copying stack slices, as atomicx v1 does** (Gustavo, 2026-10-14 to 2026-10-22, **work in progress**): the buffer is `volatile char[stack_size]`; `start()` marks the top of the stack; `yield()` marks a bottom, copies the slice into a buffer, and uses `setjmp`/`longjmp` on a `jmp_buf` per thread (§3.5). | Gustavo's first cut toward v1's switch ("This is the first step toward atomicx v1's stack-copy switch", `d8bb034`). As built it does not switch threads yet (P23 to P27). A complete design following v1's structure is proposed in §3.6, waiting for Gustavo's confirmation (D2). |
| A10 | **Tracing tools while the switch is built** (Gustavo, 2026-10-17, 2026-10-22 and 2026-10-26): `dump_threads()` prints every thread of the context with its state, `nice` and `lastRunTime`; `yield()` prints "Stack saved…" and "Context restored."; `dump_data(data, size, label)` prints memory as a hex dump. Since 2026-10-26 the other traces are commented out, not removed, and `yield()` does not call `dump_threads()`. | Gustavo: so that the schedule order and the switch can be watched while they are built (`2f9a027`); fewer of them so that the demo's own output can be read (2026-10-26). They printed with `std::cout`, which added to P1; since 2026-10-31 every trace goes through `TRACE(…)` and is compiled only in a `TRACE=1` build (A15). `dump_data` builds each line in a buffer and traces it once, so the `[TRACE] file:line - ` prefix comes once per line, not once per byte; its text column keeps the bytes 0x20 to 0x7e, what `isprint()` keeps in the C locale, without calling `isprint()`, a library call the compiler cannot remove. |
| A11 | **A thread sets its own `nice` with `setNice()` and reads it with `getNice()`, and `yield()` sleeps until the chosen thread is due** (Gustavo, 2026-10-23; `getNice()` 2026-10-26). `nice` is how many ticks after its last run a thread becomes due; when the chosen thread is not due yet, `yield()` waits the gap with the program's `sleepTicks()`. | The same as v1's kernel, which sleeps until the earliest wake-up with `Atomicx_SleepTick` (§3.6); `sleepTicks()` is now called by the library. Since 2026-10-31 the gap is `lastRunTime + nice - getTick()` in `ax::time`, modulo the tick range; `yield()` sleeps it when it is neither 0 nor at least `k_tick_half_range` (a deadline already passed), as the schedule order measures deadlines (R3.16, A16). Until then it was computed in `ssize_t`: not wrap-safe, and `ssize_t` does not exist on AVR (P30). |
| A12 | **The scheduling stays in `yield()`; `start()` is no kernel loop** (Gustavo, 2026-10-28). `yield()` chooses the next thread, starts it or resumes it, and sleeps until it is due; `start()` marks the top of the threads' stack, keeps `_top_context` as the way back to it, and runs the first thread. | Gustavo: "I didn't want to have the loop tied to the start …, it was my original design from the atomicx 1". Rejected: a kernel loop in `start()` that every `yield()` jumps to (built and measured on 2026-10-28, not kept; §6). §3.6 records atomicx v1.3.0's `Start()` as the loop that starts and resumes threads (`atomicx.cpp:320-340`); which v1 design A12 follows is to be recorded by Gustavo (P34). |
| A13 | **Debug symbols only on request: `SYMBOLS=1`** (Gustavo, 2026-10-29). `-g` is no longer in the default `CFLAGS`; `make build SYMBOLS=1` adds it, and `make debug` always sets it. | Gustavo: build with `SYMBOLS=1` "to enable the symbols", as VS Code's debug launch does. `make debug` sets it itself so that `lldb` never gets a binary without symbols. Rejected: `-g` always, the former default, which would leave the switch without effect. |
| A14 | **`auto_obj_list::swap(a, b)`, and `sort` built on it** (Gustavo, 2026-10-29): `swap` exchanges the places of two items of a list by relinking them; `sort` is a stable insertion sort that moves each item back with `swap` while it must come before the item in front of it. | Gustavo asked for a `swap` of two items and for `sort` to use it, and suspected that the merge sort lost the null at the end of the list, so the list never reached its end. *Measured*: the merge sort did not (20,000 random lists, 0 corrupted, 0 out of order); the list breaks because `restore` writes stale bytes over a thread object's links (P23). The swap sort is as correct (the same test), keeps the links consistent after every step, and fits a scheduler's list, which stays almost in order between sorts: on 8 threads, 7 comparisons for a list in order (12 with the merge sort) and 13 with one thread out of place (14); 28 for a reversed list (12), its worst case. Rejected: keeping the merge sort, correct but not built on `swap`. |
| A15 | **`TRACE(a << b);` prints one line when `TRACE_ON` is defined; without it, nothing is compiled; `TRACE=1` turns it on for the native build and the Arduino targets, off by default** (Gustavo, 2026-10-31). Without `TRACE_ON`, `TRACE(...)` is `do { } while (0)`: the preprocessor drops the arguments, so the compiler never sees them, they are never evaluated, and nothing refers to a stream. With it, on the PC: `do { std::cout << "[TRACE] " << __FILE__ << ":" << __LINE__ << " - " << __VA_ARGS__ << std::endl; } while (0)`; on Arduino (`ARDUINO` defined), the same pieces go to `serial_stream` (`trace_stream` until A16), an adapter in `atomicx.cpp` whose `operator<<` calls `Serial.print` for `const char*`, `char`, `int`, `unsigned int`, `long` and `unsigned long`, and prints any pointer as `0x` and its address in hex; the line ends with `Serial.println()`. The macro ends the line itself, so calls carry no `std::endl` or `std::flush`, which AVR does not have. The `do`/`while (0)` makes the call one statement, safe inside an `if` with an `else`; `...` lets the message hold commas. Every `std::cout` of `atomicx.cpp`, the commented-out ones included, is a `TRACE(…)`. The Makefile turns `TRACE=1` into `-DTRACE_ON`: in `CFLAGS` for `build`, `run` and `debug`, and as `--build-property "compiler.cpp.extra_flags=-DTRACE_ON"` for `nano`, `nano_flash` and `nodemcu`, because `arduino-cli` does not read `CFLAGS`; that property is empty on the AVR and ESP32 cores. | Gustavo's `TRACE << …` with `(void)0` for the off case cannot take `<<`: `make build` failed at the first `TRACE` ("invalid operands to binary expression ('void' and 'const char[52]')"). Gustavo asked that nothing be compiled when tracing is off, so it also builds on AVR, which has no `<iostream>`; that needs the message as a macro argument, hence `TRACE(…)`. Then he asked for `TRACE=1` to work for the local build and for Arduino, off by default: AVR has neither `std::cout` nor `std::endl`, and `Serial` has no `<<`, hence the adapter and the line end in the macro. Rejected: `while (false) std::cout` (built on 2026-10-31, then replaced: the arguments still compile and need `std::cout`); a null stream object (the same, plus an overload for `std::endl`); `std::cout` on every Arduino (the ESP32 has it, AVR does not); a template `operator<<` in the adapter (a `char[]` buffer would bind to a pointer template and print as an address). The cost: an error inside a `TRACE` shows only in a `TRACE=1` build; a function or variable used only in traces is unused, an error under `-Werror`, when tracing is off; a type the adapter does not take (`long long`, `double`) fails only in an Arduino `TRACE=1` build; on AVR the trace texts, `__FILE__` included, sit in RAM (*measured*: 326 bytes for the 5 traces of the test sketch). |
| A16 | **The library builds for the Nano: no stream in the header, errors through `LOG_ERROR(…)`, a wrap-safe sleep, and stacks sized per platform in the sketch** (Gustavo, 2026-10-31). `atomicx.h` includes no `<iostream>`. `LOG_ERROR(a << b);` is always compiled and prints one line: `do { std::cerr << __VA_ARGS__ << std::endl; } while (0)` on the PC, `serial_stream() << __VA_ARGS__` and `Serial.println()` on Arduino; the allocation failure and the stack overflow use it. `yield()` sleeps `(ax::time)(lastRunTime + nice - getTick())` when it is not 0 and below `k_tick_half_range`. `arduino/simple` gives each thread `k_stack_size` bytes: 64 on AVR, where a thread's slice is about 30 bytes, and 500 elsewhere. | Gustavo asked to revisit the Arduino code and build it for a Nano. AVR has no `<iostream>`, no `std::cerr` and no `ssize_t`; those were the only errors (§6). The errors stay visible in every build, and the PC prints them exactly as before; Arduino has no `stderr`, so they go to `Serial`. The sleep reuses R3.16's measure, which also fixes the 49.7-day sleep across the wrap (P30). The sketch's 8 threads of 500 bytes needed 4,000 bytes of heap; the Nano has 2,048 bytes of RAM, 1,564 of them free after the globals. *Estimated* from `-fstack-usage` (AVR, without LTO): `start()` 9 bytes, each `run()` 2 (`dump()` inlined), `yield()` 13, plus 2-byte return addresses: about 30 bytes; 64 leaves room for twice that. Rejected: printing the errors with `TRACE` (silent unless `TRACE=1`, where an overflow on a board would end the system without a word); fewer threads on AVR (all 8 fit: `setup()`'s frame, 460 bytes, holds the 8 objects of 55 bytes; the buffers take 8 × 66 bytes of heap). The cost: `LOG_ERROR`'s texts sit in RAM on AVR. |
| A17 | **The CI builds what the Makefile and the sketch build** (Gustavo: "monitor the CI and fix in case of error", 2026-10-31). `.github/workflows/c-cpp.yml`, on every push and pull request to `main`: job `pc`, on `ubuntu-latest` (GCC) and `macos-latest` (clang), runs `make build`, `make build TRACE=1` and `make build SYMBOLS=1`, all under `-Werror`, then runs the demo for 5 s and requires more than 8 counter lines and no overflow message, as information only (`continue-on-error`); job `nano` installs `arduino-cli` (`arduino/setup-arduino-cli@v2`) and the `arduino:avr` core and compiles `arduino/simple` for `arduino:avr:nano`, with and without `TRACE_ON`. | The workflow was GitHub's autotools starter: `./configure`, `make check` and `make distcheck` exist nowhere here, so every run since `8607977` failed at its first step (P13). The new jobs check the gates of R1.3 and A16 on both compilers the project names (§1). The demo is not a test with a pass condition (there is no test target, P13), so its run informs without failing the build. Rejected: an ESP32 job (its core is a large download; the Nano is the tighter target); running the demo as a blocking step (it never ends on its own and its output depends on timing). |

### 0.3 Development standards: the definition of done

The workspace standard (Gustavo, 2026-09-27; full text in `unlimited/spec.md` §0.5, with the same numbering)
applies here:

| # | Standard | What it asks for here |
|---|---|---|
| D1 | **Spec first** | The change is written in this file before or with the code: plain words first, exact rules after, each decision with its reason and the alternatives rejected, a changelog row, every affected section. |
| D2 | **Design explained, with diagrams** | Plain language first, complete after, at least one picture (Mermaid, or SVG generated from the code). A design choice that changes behaviour is shown to Gustavo and confirmed before it is built. |
| D3 | **Proof that it works** | Tests that fail before and pass after, numbers measured before and after, every target green. Today the targets are `make build` and `make run` on the PC and an `arduino-cli compile` of `arduino/simple`; there is no `make test` yet (P13). Limits stated in §7, never hidden. |
| D4 | **Every document updated** | This file, `README.md`, the examples and the comments describe the code as it is. |
| D5 | **Clarity** | Accessible to newcomers and to C++ and embedded programmers; every term defined (§8); numbers with units, marked *measured* or *expected*. |
| D6 | **Git** | Commit and push only when Gustavo asks; the author is Gustavo Campos, with no co-author lines. |
| D7 | **Pedagogic, with pictures everywhere** | Plain words for newcomers first, depth for experts after, in the documents and in every report to Gustavo. |

---

## 1. Files and build (normative)

### 1.1 Files

**In plain words.** The library is two files. The header says *what* the library offers; the source says *how* it
works. A program compiles the source once and includes the header wherever it uses the library.

```mermaid
flowchart LR
    h["atomicx/atomicx.h<br/>declarations, macros, enums<br/>template sort"]
    c["atomicx/atomicx.cpp<br/>every other function body"]
    m["main.cpp<br/>PC demo"]
    s["arduino/simple/simple.ino<br/>Arduino sketch"]
    c -->|includes| h
    m -->|includes| h
    s -->|includes the header and the source| c
```

| File | Role |
|---|---|
| `atomicx/atomicx.h` | The interface (A1): the `ax` namespace, its types, the classes and the template `sort`. |
| `atomicx/atomicx.cpp` | The definitions of every non-template member function (A1), and the file's own helpers: `dump_data`, the schedule rank (`Rank`, `rank`), `k_tick_half_range`, the `TRACE` (A15) and `LOG_ERROR` (A16) macros, and on Arduino `serial_stream`, their adapter to `Serial`. |
| `main.cpp` | The PC demo: eight threads that yield forever, four of them printing a counter, run by `start()` (§4). |
| `arduino/simple/simple.ino` | The Arduino sketch, the PC demo's eight threads on `Serial`, with `k_stack_size` bytes of stack each (64 on AVR, 500 elsewhere, A16); `arduino/simple/atomicx` is a symbolic link to `../../atomicx`. |
| `Makefile` | The PC build and the Arduino targets (§1.2). |
| `README.md` | A stub with a Nano compile command (P15). |
| `bard.txt` | Prompt-and-answer notes about a self-registering C++ list and a Makefile; never built. |
| `.github/workflows/c-cpp.yml` | GitHub's autotools starter workflow; it fails (P13). |

**Exact rules.**

1. **R1.1** `atomicx.h` follows A1: a new non-template member function is declared in the header and defined in
   `atomicx.cpp`, in declaration order.
2. **R1.2** `atomicx.cpp` is compiled exactly once per program. The Makefile compiles it as its own unit
   (`SRCS = $(wildcard *.cpp) $(wildcard $(CPX_DIR)/*.cpp)`). The Arduino sketch includes it as text
   (`#include "atomicx/atomicx.cpp"`), because `arduino-cli` compiles only the sketch folder's own files and its
   `src/` folder, never `atomicx/`; the sketch therefore builds as one unit (a unity build, §8), with no duplicate
   symbols.
3. **R1.3** The PC build is `g++ -O0 -Wall --std=c++11 -Wall -Wextra -Werror` (the Makefile's `CFLAGS`), with `-g` added
   when `SYMBOLS=1` is given (A13) and `-DTRACE_ON` when `TRACE=1` is given (A15); it must stay free of warnings,
   with and without `TRACE=1`. The code is C++11: nothing newer may be used.

### 1.2 Make targets (as built, checked 2026-10-22)

**In plain words.** `make` builds and runs the PC demo; the other targets compile the Arduino sketch and upload it to
a board, or install and run tools. The table says which ones were checked.

| Target | What it does | State |
|---|---|---|
| `make build` (the default) | Cleans, then builds `bin/demo_atomix.bin` from `main.cpp` and `atomicx/atomicx.cpp`; `make build SYMBOLS=1` adds debug symbols (A13); `make build TRACE=1` turns the traces on (A15), for this build only. | Clean under `-Werror` (*measured*). |
| `make run` | `build`, then runs the demo; `make run TRACE=1` with the traces. | Never returns: the threads loop forever, taking turns since the slice range fix (§6, 2026-10-29 and 2026-10-31). |
| `make debug` | `build` with `SYMBOLS=1`, then opens the demo in `lldb`. | Not checked. |
| `make nano`, `make nano_flash` | `arduino-cli compile --upload` of `arduino/simple` for the Nano (serial port or USBasp); `TRACE=1` adds `-DTRACE_ON` through `compiler.cpp.extra_flags` (A15). | Compiles and links, with and without `TRACE=1` (2026-10-31, A16): 5,850 bytes of flash and 484 bytes of RAM, 6,438 and 774 with `TRACE=1`; not uploaded or run on a board (P36). Until 2026-10-31 it failed: AVR has no `<iostream>`, which `atomicx.h` included (P1). |
| `make nodemcu` | The same for the ESP8266 NodeMCU; `TRACE=1` as for the Nano. | Not checked: the ESP8266 core is not installed here. |
| `make bootloader_nano`, `make install_arduino_cli` | Burn the Nano's bootloader; install `arduino-cli` and its cores with Homebrew. | Not checked. |
| `make document` | Runs `Doxygen`. | No Doxyfile in the repository (P15). |
| `make depend`, `make clean`, `make help` | `makedepend`; remove the objects and the binary; list the targets. | `clean` and `help` work. |

The CI (`.github/workflows/c-cpp.yml`, A17) runs `make build`, `make build TRACE=1` and `make build SYMBOLS=1` on Ubuntu
(GCC) and macOS (clang), runs the demo for 5 s for information, and compiles `arduino/simple` for the Nano with and
without `TRACE_ON`, on every push and pull request to `main`.

In VS Code, `.vscode/launch.json` debugs the demo after the task `make build SYMBOLS=1` of `.vscode/tasks.json`; both
files stay local, because `.gitignore` ignores `.vscode` (P18). The ESP32 has no target in the Makefile; the sketch compiled for it with `arduino-cli compile --fqbn
esp32:esp32:esp32` once `setup()` and `loop()` were added to a copy (§6, P3).

---

## 2. The self-registering list: `ax::auto_obj_list`

### 2.1 In plain words

`auto_obj_list` is an **intrusive doubly linked list**: the links live inside the objects themselves, so the list
never allocates memory. An object joins its list when it is constructed and leaves it when it is destroyed.

```
            head                                          tail
             |                                             |
             v                                             v
        +---------+  next   +---------+  next   +---------+
null <--| item A  |-------->| item B  |-------->| item C  |--> null
        |         |<--------|         |<--------|         |
        +---------+  prev   +---------+  prev   +---------+
          every item also keeps `parent`: the list it belongs to
```

```mermaid
sequenceDiagram
    participant P as your code
    participant I as the item
    participant L as its list
    P->>I: constructs the item with the list
    I->>L: add this item at the tail
    Note over L: the item is now reachable from getHeadItem
    P->>I: the item goes out of scope
    I->>L: remove this item, mending head and tail
```

### 2.2 Exact rules (as built)

| Member | Access | Rule |
|---|---|---|
| `item(auto_obj_list& parent)` | public | Keeps `parent`, appends the item at the tail (`add`); its trace, `TRACE("Added item to parent list at address: " …)`, is commented out (A10). |
| `item()` | deleted | An item always belongs to a list. |
| `~item()` | public | Unlinks the item (`remove`) and prints `Removed item from parent list at address: <this>`. |
| `getNextItem(item* current) const`, `getPrevItem(item* current) const` | public | Return `current->next`, `current->prev`, or `nullptr` when `current` is null. They read their argument, not `this` (P10). |
| `next`, `prev` (`item*`, default `nullptr`); `parent` (`volatile auto_obj_list&`) | protected, in `item`; `auto_obj_list` is a friend of `item` | The links, set by `add` and `remove` and rewritten by `swap`, and the list the item belongs to, bound by the constructor; visible to every class derived from `item`. |
| `isEmpty() const` | public | True when `head` is null. |
| `count() const volatile` | public | Returns `_count`, which `add` and `remove` keep: O(1). |
| `getHeadItem() const`, `getTailItem() const` | public | The first and the last item, or `nullptr`. |
| `swap(item* a, item* b) volatile` | public | Swaps the positions of two items of this list by relinking them; no object is copied. Returns false when an item is null or belongs to another list; true otherwise, also when `a` is `b`. Mends `head` and `tail` (A14). |
| `sort(Compare cmp, Data data) volatile` | public, template | §2.3. |
| `add(item*) volatile`, `remove(item*) volatile` | protected | Append at the tail; unlink and mend `head` and `tail`. Both return false only for a null pointer. Only `item`'s constructor and destructor call them. |
| copy constructor and copy assignment of `item` and of `auto_obj_list` | public, implicit | Not declared; the compiler provides them (R2.5, R2.6). |

1. **R2.1** Items are kept in construction order (each one appended at the tail) until `sort` reorders them.
2. **R2.2** A list is walked with `for (item* i = list.getHeadItem(); i; i = i->getNextItem(i))`, as `main.cpp` does.
3. **R2.3** An item must be destroyed before its list: its destructor calls `parent.remove(this)`.
4. **R2.4** As built, `add` and `remove` take no lock and do not mask interrupts.
5. **R2.5** An item must not be copied: the implicit copy constructor copies the links without registering the copy,
   and the copy's destructor unlinks the original (*measured*, P4). The type does not prevent it for `item` and the
   classes derived from it; for `thread_item` only its `volatile` `_data` member does (R3.7).
6. **R2.6** A list must not be copied: the implicit copy constructor and copy assignment of `auto_obj_list` (and so of
   `thread_context`) copy `head` and `tail` but not the items, whose `parent` still names the original; destroying an
   item mends only the original, and the copy keeps a dangling pointer (*measured*, P21). Passing a context by value
   compiles without a warning. The type does not prevent it yet.

### 2.3 Sorting: `sort(cmp, data)`

**In plain words.** `sort` reorders the items with `swap`: no object is copied and no memory is taken. You give it
`cmp`, the rule that says which of two items comes first, and `data`, anything `cmp` needs to decide (the schedule
order passes the thread that holds now). It walks the list once; each item moves back, one swap at a time, while it
must come before the item in front of it. It is a **stable** sort: an item never passes one that `cmp` calls equal,
so equal items keep the order they had.

```
keys in the list:   3   1   2   1'          (1 and 1' are equal keys; 1 was constructed first)

1 :  1 < 3, swap                 -> 1  3  2  1'
2 :  2 < 3, swap; 2 < 1? no      -> 1  2  3  1'
1':  1' < 3, swap; 1' < 2, swap;
     1' < 1? no: equal, it stops -> 1  1' 2  3       (1 stays before 1')
```

**Exact rules.**

1. **R2.7** `cmp(a, b, data)` returns true when `a` must come **strictly** before `b`, like `operator<` (a strict weak
   ordering). `sort` calls it as `cmp(*current, *current->prev, data)` and swaps the two only when `current` is
   strictly smaller, so `cmp` must return false for equal items, or the sort is no longer stable.
2. **R2.8** `cmp` receives the items as `auto_obj_list::item`. To reach a derived class's data it casts,
   `static_cast<const my_type&>(a)`, which is valid only when every item in that list is a `my_type`.
3. **R2.9** Insertion sort by `swap` (A14): n - 1 comparisons on a list already in order, at most n(n - 1)/2 swaps
   and about as many comparisons at worst; O(1) extra memory, no recursion; the links, `head` and `tail` stay
   consistent after every swap. `data` is copied by value into every call of `cmp`, so it should be small: a pointer
   or a number.
4. **R2.10** C++11: `cmp` may be a function pointer, a functor or a lambda with typed parameters (generic lambdas are
   C++14); it always takes three parameters, the third of the type of `data`.

*Measured* (2026-10-29, §6): 20,000 random lists of 0 to 14 items with repeated keys, each sorted and compared with
`std::stable_sort`, with every link checked: 0 corrupted, 0 out of order, for the merge sort that came before and for
this one. `yield()` and `start()` are the callers.

---

## 3. Threads and the context

### 3.1 In plain words

A **thread** is an object of your class derived from `ax::thread_item`; its `run()` is the code the thread executes.
A **context** (`ax::thread_context`) is the list of threads that take turns on one processor. Because
`thread_context` is an `auto_obj_list` and `thread_item` is an `auto_obj_list::item`, §2 holds for them: a thread is
in its context from its construction to its destruction. One difference: a thread cannot be copied (R3.7).

```mermaid
classDiagram
    class auto_obj_list {
        +isEmpty() bool
        +count() size_t
        +getHeadItem()
        +getTailItem()
        +sort(cmp, data)
        #add(newItem) bool
        #remove(itemToRemove) bool
        -head
        -tail
    }
    class item {
        +item(parent)
        +getNextItem(current)
        +getPrevItem(current)
        #next
        #prev
        #parent
    }
    class thread_item {
        <<abstract>>
        +thread_item(parent, stack_size)
        +run()*
        +compare(a, b, data) bool$
        +dump_threads()
        #setNice(nice)
        #getNice() time
        #yield()
        -_v_stackPointer
        -_stk_bottom
        -_register_context
        -_stackSize
        -_data
        -_context
    }
    class thread_context {
        +start() bool
        #save(stackTop, vStackPointer, stackSize)
        #restore(stackTop, vStackPointer, stackSize)
        #_current
        #_stk_top
        -running bool
    }
    auto_obj_list "1" o-- "many" item : links
    item <|-- thread_item
    auto_obj_list <|-- thread_context
    thread_context "1" o-- "many" thread_item : registers
```

`item` is a type nested in `auto_obj_list` (`ax::auto_obj_list::item`); the diagram draws it apart. `thread_item` and
`thread_context` are friends of each other.

### 3.2 `thread_item` (as built)

**In plain words.** Constructing a thread does three things: it joins the context's list, it takes a stack buffer of
`stack_size` bytes for the thread, and it marks the thread `READY`. Destroying it gives the buffer back and leaves the
list. A class derived from it writes `run()` and calls `yield()` from there to give the processor back (A6).

The order in which C++ builds the object, whatever order the initializer list is written in:

```
thread_item(ctx, stack_size)
  1. base   item(ctx)              joins ctx's list, prints "Added item to parent list at address: ..."
  2. member _v_stackPointer        new (std::nothrow) volatile char[stack_size]: stack_size bytes
  3. member _stk_bottom            nullptr (default member initializer)
  4. member _register_context      zeroed (default member initializer)
  5. member _stackSize             stack_size * sizeof(size_t): 8 times the buffer on a 64-bit PC (P27)
  6. member _data                  tm_now = 0, lastRunTime = 0, nice = 1, state = READY
  7. member _context               ctx
  8. body   buffer null?  yes ->   std::cerr "Failed to allocate stack for thread item.", _stackSize = 0
                          no  ->   (commented out) TRACE(">>> Allocated stack for thread item at address: ... with size: <stack_size>")
```

**Exact rules.**

1. **R3.1** The initializer list must be written in the construction order above: the base first, then the members
   in the order of their declaration. `-Werror` turns `-Wreorder-ctor` into an error otherwise.
2. **R3.2** `stack_size` is the buffer's size in bytes (A9). `_stackSize` must hold the same size; as built it holds
   `stack_size * sizeof(size_t)` (P27).
3. **R3.3** When the allocation fails (A2), the thread stays registered, with `_stackSize == 0` and a null buffer.
4. **R3.4** `~thread_item()` frees the buffer (`delete[]`; nothing to do when it is null), then `~item()` leaves the
   list.
5. **R3.5** `run()` is pure virtual: every thread class must define it.
6. **R3.6** A `thread_item` is constructed only with a `thread_context`; `_context` keeps it with that type, and
   `parent` (from `item`) keeps it as a `volatile auto_obj_list`. Both name the same object. Through `parent` only
   `volatile` member functions can be called (A5); through `_context`, any.
7. **R3.7** A `thread_item` cannot be copied: its implicit copy constructor is deleted because `volatile st_data`
   has no usable copy constructor (*measured*: "copy constructor of 'thread_item' is implicitly deleted because field
   '_data' has no copy constructor"). That protects threads from R2.5's hazard, by accident rather than by design.
8. **R3.8** `yield()` is protected: a thread's `run()` calls it to give the processor back. As built it is work in
   progress (§3.5). `setNice()` and `getNice()` are protected too: a thread class sets its own `nice`, usually in its
   constructor, and reads it back, as `main.cpp` does.

| Member | Access | Type | Meaning (as built) |
|---|---|---|---|
| `st_data` | public | struct | `tm_now` (now, for the schedule order), `lastRunTime`, `nice` (`ax::time`) and `state`. |
| `compare(a, b, data)` | public, static | `bool (const item&, const item&, const thread_item*)` | The schedule order (A7, §3.4). |
| `dump_threads()` | public | `void () volatile` | Prints a header line, every thread of the context with its address, state, `nice` and `lastRunTime` (` * [DUMP]` lines), and a separator (A10). Its lines about this thread and the context are commented out; nothing calls it. |
| `setNice(nice)` | protected | `void (ax::time)` | Sets `_data.nice` (A11). |
| `getNice()` | protected | `ax::time () const` | Returns `_data.nice` (A11). |
| `yield()` | protected | `void ()` | Sorts the context and tries to switch to its head (§3.5). |
| `_v_stackPointer` | private | `volatile char*` | The thread's stack buffer ("virtual stack memory"), `stack_size` bytes. |
| `_stk_bottom` | private | `volatile char*` | The lowest address of this thread's slice: the address of a local of its last `yield()` (its own since `ef5eeee`). |
| `_register_context` | private | `jmp_buf` | The registers `setjmp` saves; as built, the `yield()` that chooses this thread fills it with its own registers (P24). |
| `_stackSize` | private | `volatile size_t` | `stack_size * sizeof(size_t)`; 0 after a failed allocation; compared with the slice by `yield()` (P27). `stackSize` until 2026-10-28. |
| `_data` | private | `volatile st_data` | Initially `tm_now` 0, `lastRunTime` 0, `nice` 1, `READY`. |
| `_context` | private | `thread_context&` | The context the thread belongs to. |

### 3.3 `thread_context` (as built)

**In plain words.** The context is the list of threads. `start()` marks where the threads' stack begins, keeps a way
back to itself (`_top_context`), and runs the first thread; everything after that happens inside the threads'
`yield()` calls (§3.5, A12). A `yield()` that chooses a new thread jumps back here, and `start()` runs that thread; on
an error it jumps back to make `start()` return.

```mermaid
flowchart TD
    s["start"] --> e{"list empty or already running?"}
    e -->|yes| f["return false"]
    e -->|no| r["running = true"]
    r --> o["sort the context"]
    o --> m["_stk_top = the address of a local of an inner block"]
    m --> j{"setjmp on _top_context returns"}
    j -->|"0, the first time, or 1: a yield chose a READY head"| h["the head, as a thread_item: RUNNING, lastRunTime = getTick"]
    h --> x["call its run"]
    x -->|"run returns"| t["return false"]
    j -->|"2: the head is in another state; 3: a stack overflow"| t
```

**Exact rules.**

1. **R3.9** `start()` returns false when the list is empty or `running` is true. Otherwise it sets `running`, sorts the
   context, sets `_stk_top` to the address of a `volatile char` declared in an inner block (P33), and saves its
   registers in `_top_context` with `setjmp`. When `setjmp` returns 0 (the first time) or 1 (a `yield()` chose a
   `READY` head), it makes the head (cast to `thread_item*`) `RUNNING`, stamps its `lastRunTime` with `getTick()`, and
   calls its `run()`; it returns false when that `run()` returns. When `setjmp` returns more than 1 (2: the chosen head
   is neither `READY` nor `RUNNING`; 3: a stack overflow), it returns false. `running` is never reset, so a second
   `start()` returns false.
2. **R3.10** The comment on `start()`, "BY DESIGN would never return. If so the system failed to start threads",
   describes the scheduler as intended; as built `start()` returns when a thread's `run()` returns (that thread is not
   marked `TERMINATED`), when a slice overflows its buffer, or when the chosen head is neither `READY` nor `RUNNING`.
3. **R3.11** Every item in a `thread_context` must be a `thread_item`: `start()`, `compare` and `yield()` cast without
   checking. The types do not enforce it (P11).
4. **R3.12** `save(stackTop, vStackPointer, size)` copies the `size` bytes below `stackTop`,
   `[stackTop - size, stackTop)`, into the buffer `vStackPointer`; `restore(stackTop, vStackPointer, size)` copies
   them back to the same addresses. Both are protected and `volatile`, and take `volatile size_t*` arguments.
   `yield()` gives them `_stk_top` and `_stk_top - _stk_bottom`, so they copy `[_stk_bottom, _stk_top)`, the
   threads' slice (P23 fixed on 2026-10-29). This assumes one stack that grows down, as atomicx v1 does; C++ does
   not guarantee it, and every target of §1 has it.
5. **R3.13** `_current` (protected, `thread_item*`) is the thread `yield()` chose last; `_stk_top` (protected,
   `volatile char*`) is the top mark set by `start()`; `_top_context` (protected, `jmp_buf`) holds `start()`'s registers, the way back to
   it (A12). `thread_item` is a friend of `thread_context`, so
   `thread_item`'s own member functions reach them; a class derived from `thread_item` does not, because friendship
   is not inherited.

### 3.4 The schedule order: `thread_item::compare` (as built)

**In plain words.** Before choosing who runs next, the context's threads are put in order. First by what they are
doing: threads that are ready to run come first, then the ones running, then the sleeping and waiting ones, and the
finished ones always last. Between two threads of the same rank, the one whose deadline (its last run plus its
`nice`) comes first runs first: the most overdue thread, or, when none is due yet, the one due next. Threads with the
same deadline keep the order they had, because the sort is stable. `yield()` applies this order to its own context.

| Rank | States | |
|---|---|---|
| 1 | `NOW`, `READY` | first |
| 2 | `RUNNING` | |
| 3 | `SLEEPING`, `WAITING` | |
| 4 | `TERMINATED` | always last |

```
now = 1000    state        deadline = lastRunTime + nice
   A          SLEEPING        0 +   1 =    1     999 ticks overdue
   B          READY         995 +  10 = 1005     due in 5
   C          READY           0 +   1 =    1     999 ticks overdue
   D          READY         500 + 100 =  600     400 ticks overdue
   E          TERMINATED    999 +  50 = 1049     due in 49

sorted:   C   D   B   A   E
          C, D, B: rank 1, by deadline: 1, 600, 1005
                      A: rank 3
                          E: rank 4, always last
```

Gustavo's case of 2026-10-23, four `RUNNING` threads, none due, at now = 1773308405:

```
nice    lastRunTime    deadline        sorted
2000    1773308405     1773310405      4th
1000    1773308357     1773309357      3rd
 500    1773308296     1773308796      1st, due in 391
5000    1773303885     1773308885      2nd, due in 480
```

```mermaid
flowchart TD
    q1{"do a and b have different ranks?"}
    q1 -->|yes| r1["true when the rank of a is lower"]
    q1 -->|no| n["now = data, tm_now"]
    n --> k["key = deadline - now + 2^31, for a and for b"]
    k --> q2{"is the key of a smaller?"}
    q2 -->|yes| t["true: a comes first"]
    q2 -->|no| f["false: a does not come before b"]
```

**Ticks wrap.** `ax::time` is a `uint32_t`: at 1 ms per tick it wraps to 0 after 2^32 ms, about 49.7 days, so two
deadlines cannot be compared as plain numbers. Each one is measured from now instead, shifted by half the range
(`k_tick_half_range`, 2^31), all modulo 2^32: 2^31 ticks overdue becomes 0, now becomes 2^31, and 2^31 - 1 ticks
ahead becomes the largest key. The keys then compare with a plain `<`:

```
key = (lastRunTime + nice - now + 2^31) mod 2^32

now = 0x00000008:   P's deadline 0xFFFFFFF4 (20 ticks overdue)   key 0x7FFFFFEC
                    Q's deadline 0x00000010 (8 ticks ahead)      key 0x80000008    -> P before Q
       the naive deadline order:  0xFFFFFFF4 > 0x10                          -> Q before P: wrong
```

**Exact rules.**

1. **R3.14** `compare(const item& a, const item& b, const thread_item* data)` is a public static member of
   `thread_item`. It takes `auto_obj_list::item` references so that `sort` can call it
   (`ctx.sort(ax::thread_item::compare, thread)`), and casts both to `thread_item`: every item in the list must be a
   thread (R3.11). `data` must point to a thread whose `_data.tm_now` holds now.
2. **R3.15** It returns true when `a` must come before `b`: the rank of `a`'s state is lower than the rank of `b`'s;
   or the ranks are equal, and `a`'s key is smaller than `b`'s (R3.16). Otherwise false. The rank comes from a file-local `rank()`,
   one `switch` over every `State` with no `default`, so a new `State` must be ranked before the code compiles. For a
   fixed now this is a strict weak ordering (*measured*, §6), as `sort` requires (R2.7).
3. **R3.16** A thread's key is `(ax::time)(lastRunTime + nice - now + k_tick_half_range)`, with
   `now = data->_data.tm_now` and `k_tick_half_range` = 2^31, a constant of `atomicx.cpp`. Keys order deadlines
   correctly from 2^31 ticks overdue to 2^31 - 1 ticks ahead, 24.8 days each way at 1 ms per tick; a deadline more
   than 2^31 ticks overdue reads as far ahead and sorts last in its rank. The key is lowest for the most overdue
   thread, so a thread is due exactly when its key is at most 2^31.
4. **R3.17** Now is read from `data`, not from `getTick()`: it stays the same during one sort. `yield()` sets it with
   `_data.tm_now = getTick()` before sorting.
5. **R3.18** Threads that tie, the same rank and the same deadline, keep the list's order (`sort` is stable); equal
   threads are not rotated (P22).

### 3.5 The context switch (as built, work in progress)

**In plain words.** `yield()` is where a thread gives the processor back, and, by Gustavo's design (A12), where the
next thread is chosen: there is no loop in `start()`. `yield()` sorts its context and takes the head as the next
thread. It marks its own part of the stack, checks that it fits its buffer and copies it there, the way atomicx v1
does (§3.6). A `READY` head has never run: `yield()` jumps back to `start()`, which calls the head's `run()` from its
own frame, so every thread's frames start at the same place, under `start()`. A `RUNNING` head has paused before:
`yield()` sleeps until it is due (A11), jumps back into itself, copies the head's saved slice over the stack and
returns, and the return continues in the head's frames, which the copy has just put back at the same addresses. In
the demo the four printing threads take turns, each at its own pace. This section describes the code; it is not
normative.

```mermaid
flowchart TD
    a["tm_now = getTick, sort"] --> b{"a head in the list?"}
    b -->|no| z["return"]
    b -->|yes| c["_current = head, setjmp on the head's jmp_buf"]
    c -->|first return| d["mark this thread's stack bottom"]
    d --> o{"slice larger than _stackSize?"}
    o -->|yes| s["LOG_ERROR STACK OVERFLOW, longjmp to _top_context with 3: start returns false"]
    o -->|no| v["save the slice below _stk_top into this thread's buffer"]
    v --> e{"state of the head"}
    e -->|READY| g["longjmp to _top_context with 1: start runs the head"]
    e -->|RUNNING| w["sleep until the head is due, with sleepTicks"]
    w --> j["longjmp to the head's jmp_buf, the setjmp above"]
    e -->|other| x["longjmp to _top_context with 2: start returns false"]
    c -->|"second return"| k["restore the head's slice, stamp lastRunTime"]
    k --> z
```

What the demo does (*measured*, 2026-10-29 and 2026-10-31: eight threads of 500 bytes, §4, §6):

```
higher addresses
  main()   the eight thread objects
  start()  _stk_top, setjmp(_top_context)
           |  every thread's frames start here: run() -> (dump()) -> yield()
  yield()  dummy = _stk_bottom; [_stk_bottom, _stk_top) is the thread's slice: 128 or 176 bytes
lower addresses

1. start() runs the head's run(); it prints and calls yield()
2. yield() saves its slice; the new head is READY: longjmp(_top_context, 1)
3. start() runs that head's run() from the same place ... until all eight have run once
4. yield() saves its slice; the head is RUNNING: sleep, longjmp(head's jmp_buf, 1)
5. the same yield() returns from setjmp again: restore the head's slice over [head's _stk_bottom, _stk_top)
6. yield() returns into the head's frames, and the head prints its next counter
```

1. Since `e6e9d97`, every thread's `run()` is called from `start()`, so a slice holds that thread's frames only
   (P25 fixed): 128 bytes for a `MyThreadItem`, 176 for a `MySecThreadItem`, whose `yield()` is one call deeper.
2. `setjmp` goes to the head's `_register_context`, not the yielding thread's, and the `longjmp` after the sleep lands
   in the same call (P24). The resumed thread continues only because the restore rewrites the frames this call is
   running in with the head's saved frames, at the same addresses.
3. Since 2026-10-29, `save` and `restore` copy `[_stk_bottom, _stk_top)`, below the top (P23 fixed); until then
   they copied from `_stk_top` upward, the bytes above the threads' stack. `_stk_top` is still the address of a
   variable whose block has ended (P33). AddressSanitizer stops the first `save`: "stack-use-after-scope … READ of
   size 416" at `start()`'s `dummy` (2026-10-28); "stack-buffer-overflow … READ of size 416" just past `yield()`'s
   `dummy` with the fix (2026-10-29), where the slice starts: a stack-slice copy crosses the redzones it puts around
   locals, so it cannot check this switch.
4. The slice is compared with `_stackSize`, but `_stackSize` is 8 times the buffer on a 64-bit PC and twice it on
   AVR: the check lets a slice of up to 4,000 bytes into a 500-byte buffer (P27).
5. The restore, and the stamp of `lastRunTime` after it, read `this` and `current` from the frame the copy
   overwrites (P26).

### 3.6 Planned: the scheduler and the context switch

**In plain words.** atomicx v1.3.0 and the 2024 prototypes switch threads this way: every thread runs on the one
ordinary stack, and when it pauses, the part of the stack it is using is copied into its own buffer; when it resumes,
the copy goes back to the same addresses. The stack pointer never moves into the buffer, which v1 calls *zero stack
displacement*. AtomicX 2's first cut (§3.5) uses the same pieces; the design that completes it is proposed below and
waits for Gustavo's confirmation (D2). This section records v1's way as the reference, from its code
(`atomicx/atomicx.cpp` and `atomicx/atomicx.hpp` of github.com/solariun/atomicx, tag v1.3.0).

```
        the one native stack (it grows down)
   high +-------------------------------+
        | the kernel's Start() frame    |  <- base: the address of a local in Start(), taken before run()
        +-------------------------------+
        | the thread's frames: run(),   |
        | what it calls, ... Yield()    |  the used slice: base - end + 1 bytes
        +-------------------------------+  <- end: the address of a local in Yield()
   low  |                               |
        +-------------------------------+

   pause:   memcpy(buffer, end, used)    setjmp(thread)    longjmp(kernel)
   resume:  longjmp(thread)              memcpy(end, buffer, used)    the thread goes on after Yield()
```

```mermaid
sequenceDiagram
    participant K as the kernel
    participant T as thread A
    participant B as the buffer of A
    K->>T: first run, a plain call to run
    T->>T: Yield measures the used slice
    T->>B: memcpy the slice into the buffer
    T->>K: setjmp, then longjmp to the kernel
    Note over K: picks the next thread
    K->>T: longjmp back into the Yield of A
    B->>T: memcpy the slice back to the same addresses
    Note over T: run goes on after Yield
```

**v1's exact behaviour** (the reference; `file:line` in atomicx v1):

| Topic | v1 |
|---|---|
| Switch | One kernel `jmp_buf` (`atomicx.cpp:24`) and one per thread (`atomicx.hpp:1961`). A new thread starts with a plain call to `run()` from `Start()` (`atomicx.cpp:320-332`); a paused one is resumed by `longjmp` into its `Yield()` (`atomicx.cpp:340`). |
| Slice | Base: a local of `Start()` (`atomicx.cpp:324-328`); end: a local of `Yield()`; used = base − end + 1 bytes (`atomicx.cpp:370-372`). It assumes a stack that grows down and does not check it. |
| Save and restore | `memcpy` of the slice into the buffer, then `setjmp` and `longjmp` to the kernel (`atomicx.cpp:413-420`); after the kernel's `longjmp`, `memcpy` back to the same addresses (`atomicx.cpp:422-429`). No assembly anywhere. |
| Overflow | A slice larger than a fixed buffer calls `StackOverflowHandler()`, then `abort()` (`atomicx.cpp:408-409`); a self-managed buffer is reallocated with `malloc` to used + a growth step, and never shrinks (`atomicx.cpp:374-396`). |
| Buffers | From the program, as an array whose size the template constructor reads (`atomicx(T (&stack)[N])`, `atomicx.hpp:1126`), or self-managed on the heap (`atomicx(size_t nStackSize=0, int nStackIncreasePace=1)`, `atomicx.hpp:999`). |
| Scheduler | Threads in the `start` or `now` state run first, in list order; otherwise the earliest wake-up time wins, and the kernel sleeps the gap with the program's `Atomicx_SleepTick`. A plain `Yield()` sleeps `nice` ticks; an optional dynamic nice averages it with the last run time. When every live thread waits without a timeout, `Start()` returns false. |
| Time | The program defines `Atomicx_GetTick()` and `Atomicx_SleepTick()` (AtomicX 2's `getTick` and `sleepTicks`). Deadlines are absolute `uint32_t` values compared with `<` and `>=`: a 1 ms tick wraps after 2^32 ms, about 49.7 days, and v1 does not handle it. |
| States | Nine; no terminated state: when `run()` returns, the thread starts again unless `finish()` deletes it. Closest to AtomicX 2's `State`: `READY` ≈ start, `NOW` ≈ now, `RUNNING` ≈ running, `SLEEPING` ≈ sleep or a timed wait, `WAITING` ≈ wait; `TERMINATED` has no counterpart. |
| Rule for programs | A thread must never hand the address of something on its stack to another thread: while it is paused, that memory holds another thread's data (v1 README: "DO NOT USE STACK ADDRESS TO COMMUNICATE"). |

**AtomicX 2's names and their v1 counterparts.**

| AtomicX 2 (as built) | v1 | Note |
|---|---|---|
| `thread_context::start()` | `atomicx::Start()`, the kernel loop | v1's returns only when no thread can run; AtomicX 2's has no loop yet. |
| `thread_item::yield()` | `atomicx::Yield()` | Work in progress (§3.5). |
| `thread_item::compare` | `SelectNextThread()`: `start` or `now` threads first, in list order, then the earliest wake-up | AtomicX 2 ranks by state, then orders every rank by the earliest deadline, wrap-safe; v1 compares absolute deadlines and does not handle the wrap (A7). |
| `save`, `restore` | the two `memcpy` of `Yield()` | Both copy from the low end, as v1 does, since 2026-10-29 (P23 fixed). |
| `_v_stackPointer`, `stackSize` | `m_stack`, `m_stackSize` | v1's template constructor stores the element count in `m_stackSize` and compares it with a byte count: exact only for byte arrays, which every v1 example uses. AtomicX 2's buffer is in bytes; `stackSize` is not (P27). |
| `_stk_top`, `_stk_bottom` | `m_pStaskStart`, `m_pStaskEnd` | v1 keeps both ends on the thread; AtomicX 2 keeps the top on the context. |
| `_register_context`, `_current` | `m_context`, `ms_pCurrent` | v1 also has the kernel's own `jmp_buf`, `ms_joinContext`; AtomicX 2 has none yet. |
| `st_data::nice`, `lastRunTime` | `nice`, the run time of dynamic nice | |
| `getTick()`, `sleepTicks()` | `Atomicx_GetTick()`, `Atomicx_SleepTick()` | Renamed. |
| `VMEM`, `ATIMICX_SYS_CHANEL` | — | From the 2024 prototype, not from v1 (§5). |

**Proposed: completing the switch** (2026-10-22). v1's structure with AtomicX 2's names. Steps 1 and 2, a kernel loop
in `start()`, were built on 2026-10-28 and rejected by Gustavo (A12): the scheduling stays in `yield()`. Steps 3 to 5
still apply to a `yield()` that switches by itself:

1. `start()` becomes the kernel: it saves its own registers with `setjmp` in a kernel `jmp_buf` of the context, then
   loops: it runs or resumes `_current` and, when a thread's `run()` returns, marks it `TERMINATED` and chooses the next
   thread that is not `TERMINATED`, until none is left. When the chosen thread is not due yet, it first sleeps the gap
   with `sleepTicks()`, wrap-safe (A11, P30).
2. Threads are always started and resumed from one function called by the kernel, so that every thread's stack
   begins at the same address, `_stk_top`, set there; a new thread never runs inside another one's `yield()`.
3. `yield()` chooses the next thread (the sorted head, skipping `TERMINATED`); when it is itself, it returns at once.
   Otherwise it marks its **own** `_stk_bottom`, checks the slice against its buffer (an overflow stops the program
   with a message), copies `[_stk_bottom, _stk_top)` into its **own** buffer, saves its **own** registers with
   `setjmp`, records the next thread in `_current`, and `longjmp`s to the kernel.
4. A resumed thread copies its slice back as soon as `setjmp` returns the second time, reading only a static
   pointer, never its own frame, until the copy is done: before it, that memory holds another thread's data.
5. `_stackSize` holds the buffer's size in bytes.

**What the design must still decide** (questions, not decisions): where the buffers come from (the heap, as built, or
the program, P16); what an overflow does; whether `run()` returning ends the thread (`TERMINATED`, as built) or starts
it again (v1); how ticks wrap beyond 2^31 ticks; when `NOW`, `SLEEPING` and `WAITING` are set (P29); and how a thread
waits for another (the 2024 prototype's `wait`/`notify`).

---

## 4. What the program provides

**In plain words.** The library does not know how to read a clock or how to wait on your hardware, so the program
gives it two functions. As built, the library calls both in `yield()`: `getTick()` for now and for `lastRunTime`, and
`sleepTicks()` to wait until the chosen thread is due (A11).

| Function | Contract | `main.cpp` (PC) | `simple.ino` (Arduino) |
|---|---|---|---|
| `ax::time ax::getTick()` | The current time in ticks (`ax::time` is `uint32_t`). | Milliseconds from `gettimeofday`, after a `usleep(10000)` ("10ms slow dow to simulate a real system"). | `millis()` |
| `void ax::sleepTicks(ax::time nsleep)` | Wait `nsleep` ticks. | `usleep(nSleep * 1000)` | `delay(nSleep)` |

1. **R4.1** Every program must define both functions in namespace `ax`, once; a program without `getTick()` does not
   link (*measured*).
2. **R4.2** The tick's unit is the program's choice; both examples use milliseconds.

The demo (`main.cpp`): eight threads are constructed with a global `thread_context`, all with stacks of 500 bytes:
four `MyThreadItem`, then four `MySecThreadItem`, each four with nice 500, 1000, 2000 and 5000 ticks (set with
`setNice()` in the constructor); the counters start at 1000, 2000, 3000 and 4000, then 5000, 6000, 7000 and 8000
(2026-10-31). They are listed with `getHeadItem` and `getNextItem`, and `main()` calls `start()`, then prints "Main
thread finished." when it returns. Every `run()` loops forever. A `MyThreadItem`'s `run()` only calls `yield()` (its
print is commented out); a `MySecThreadItem`'s `run()` calls its private `dump()`, which prints the thread's address,
its counter (then counts it up) and `getNice()`, and calls `yield()` from there, one call deeper. `arduino/simple`
runs the same eight threads on `Serial`, with `k_stack_size` bytes of stack each (A16). *Measured* (2026-10-31, 12
s): the four printing threads take turns, about 40, 18, 11 and 4 lines for nice 500, 1000, 2000 and 5000 (§6).

---

## 5. Declared, not used yet

**In plain words.** These names are in the code, but nothing uses them yet: most are left from the 2024 prototype,
or wait for the scheduler that is still to be completed (§3.6).

| Name | Where | As built |
|---|---|---|
| `class thread;` | header | A forward declaration with no definition; the 2024 prototype's thread class. |
| `ATIMICX_SYS_CHANEL` | macro, 255 | Never used. In the 2024 prototype, the channel `wait` and `notify` used inside: `wait()` first signalled on it, and a `notify()` with a timeout that found no waiter waited on it, within that timeout, for one. v1 has no channels. |
| `ATOMIC_VERSION_LABEL` | macro | `"AtomicX v" ATOMICX_VERSION " built at " __TIMESTAMP__`; never used. |
| `VMEM(vmemory)` | macro | Expands to `vmemory[0], sizeof(vmemory) / sizeof(size_t)`: an array's first element and its size in words, the arguments of the 2024 prototype's thread constructor. Never used: the constructor allocates the buffer (A2). |
| `ax::ref` | type alias | `size_t`; `RefId` before `710c462`, the 2024 prototype's key type for `wait`/`notify`. Never used. |
| `State::NOW`, `SLEEPING`, `WAITING` | enum | Ranked by `compare`, never set (P29). |
| `LOGGING` | macro in `atomicx.cpp` (PC only) | Defined as 0, never read. |
| `#include <setjmp.h>` | header | Included twice. |

---

## 6. Validation evidence

All runs from 2026-10-13 to 2026-10-23: macOS with Apple clang 21 (the Makefile's `g++`); the ESP32 with
`arduino-cli`, core esp32 3.3.7 (xtensa GCC 14.2), `--fqbn esp32:esp32:esp32`; AVR with core arduino:avr 1.8.7
(avr-gcc 7.3). Each row names the code it ran on when that is not the code as built.

| What | Before | After |
|---|---|---|
| `thread_item` constructor with a forward-declared `thread_context` (A4) | does not compile: "cannot convert argument of incomplete type 'thread_context' to 'auto_obj_list &'" | compiles |
| Allocation failure on the PC: a thread asking for 2^58 words, 2^61 bytes (A2) | aborts: `std::bad_alloc`, exit 134 (the code of commit `8eea8f3`) | "Failed to allocate stack for thread item.", buffer null, `stackSize` 0, exit 0 |
| Parameter shadowing (A3): the same request with `new (std::nothrow)` and the parameter still named `stackSize` | the member `stackSize` keeps 2^61 after the failure | 0 |
| Header and source split (A1): the 15 moved bodies against Gustavo's working copy before the split | — | 15 of 15 identical after the uniform shift to namespace level (the constructor apart from A2 and A3). Against commit `8eea8f3`, 13 of 15: `start()` had changed before the move |
| Symbols after the split (A1, `nm`) | — | the 15 moved functions defined (`T`) in `atomicx.o` only; `main.o` defines none of them and refers (`U`) to the 4 it uses: the `thread_item` constructor and destructor, `getHeadItem` and `getNextItem` |
| The PC demo, before A6 (`fa67484`'s working copy) | — | 4 "Added…", 4 "Allocated stack…", 4 "Thread item at address…" and 4 "Removed…" lines; exit 0 |
| `sort` (§2.3, the two-parameter `sort` of 2026-10-13) | — | keys 3, 1, 2, 1 → 1, 1, 2, 3, the equal keys in construction order; tail correct |
| ESP32, the probe sketch below, `--warnings all` (A1 alone: the same code with the bodies in the header, then in the source) | 522,005 bytes of flash, 29,280 bytes of RAM, no warning from AtomicX | 522,189 bytes (+184), 29,280 bytes, no warning from AtomicX |
| ESP32, `simple.ino` with an empty `setup()` and `loop()` (A1 alone) | 265,568 bytes of flash, 22,060 bytes of RAM; 3 stream-library symbols in the image | 517,209 bytes (+251,641), 29,192 bytes (+7,132); no AtomicX code left in the image, 434 stream-library symbols, pulled in by the sketch object's references to `std::cout` and `std::cerr` |
| AVR (Nano): the sketch as it is | `fatal error: iostream: No such file or directory` | the same (P1) |
| Construction order (A5, R3.1) | `make build`: "field '_parent' will be initialized after base 'item'" (`-Werror`) | `make build` clean; the demo's output unchanged |
| Private data (A6, `fa67484`) | `make build` clean | fails: "private field '_l_stackPointer' is not used" and "private field '_parent' is not used" (`-Werror,-Wunused-private-field`); clean again from `1245c3b` (P20) |
| `start()` with three threads (the `start()` of `fa67484`) | — | one `run()` call, then `true`; a second `start()` gives `false` |
| Copying (R2.5, R2.6, R3.7) | — | a copied `item` unlinks the original (3 visible, then 2 while it still exists); a copied context keeps a pointer to a destroyed thread; copying a thread does not compile |
| A program that does not define `getTick()` (R4.1) | links (before the library called `getTick()`) | `Undefined symbols: ax::getTick()` |
| Tick wrap (A7): at now = 0xFFFFFFF8, X due 24 ticks ahead, past the wrap, and Y due; at now = 0x8, Q due 8 ticks ahead and P due 20 ticks ago, before the wrap | the naive `lastRunTime + nice <= now`: X Y and Q P, both wrong | the wrap-safe test: Y X and P Q; the same with the deadline keys (2026-10-23) |
| The schedule order (A7), §3.4's example: five threads at now = 1000 | — | C D B A E; the same with the deadline keys (2026-10-23) |
| Earliest deadline first (A7, 2026-10-23), Gustavo's dump: four `RUNNING` threads at now = 1773308405, in list order nice 2000, 1000, 500, 5000, none due | `93feabe`'s due before not due: 2000, 1000, 500, 5000, the list's order | 500, 5000, 1000, 2000 (deadlines 1773308796, 1773308885, 1773309357, 1773310405) |
| Equal deadlines (A7, 2026-10-23): A and B `READY`, both 100 + 10 | — | A B, the list's order (R3.18) |
| `NOW` ranked with `READY` (A7): W `WAITING` due, N `NOW` not due, R `READY` due, S `SLEEPING` not due | — | R N W S |
| Gustavo's case (A7): T `TERMINATED` due, X `RUNNING` not due, Y and Z `RUNNING` due | the two-level ready or not-ready rank: T Y Z X, `TERMINATED` first | the four ranks: Y Z X T; the same with the deadline keys (2026-10-23) |
| One thread of each state, all due (A7): K `TERMINATED`, W `WAITING`, S `SLEEPING`, U `RUNNING`, R `READY`, N `NOW` | the two-level rank: R N K W S U | the four ranks: R N U W S K |
| Strict weak ordering (A7): 2000 rounds of 6 random threads (all 6 states, due times around now); irreflexivity, asymmetry, transitivity and transitive ties over every pair and triple, and every sorted list | an interim rule ("true when the first is ready"): 4,065, 13,668 and 10,060 violations, 3,071 unsorted neighbours; §3.4's example sorted D C B A E | the four ranks: 0 violations. With the deadline keys (2026-10-23): 2000 rounds of 6 random threads, deadlines within 100 ticks of now: 0 violations of each rule, 0 unsorted neighbours |
| A `State` added without a case in `rank()` (A7) | — | does not compile: "enumeration value 'SUSPENDED' not handled in switch [-Werror,-Wswitch]" |
| `dump_data` (A10): 16 zero bytes; 23 mixed bytes over two lines; the printable range's edges; 0 bytes | — | 4 outputs equal to the expected text, byte for byte |
| `make build` as built (2026-10-23) | — | clean under `-Werror` (`atomicx.cpp` and `main.cpp`, exit 0) |
| The PC demo of 2026-10-22 (§3.5): threads of 250 bytes that yield once | — | exit 0; slices of 112, 240, 368 and 496 bytes for buffers of 250; 4 `longjmp`s, each followed by "Context restored"; 3 "finished" lines |
| The PC demo as built (2026-10-23), sampled for 8 s | — | runs until stopped; counters 100, 200 and 300 printed once each, 400 printed 14 times; slices of 112, 288, 464 and 640 bytes for buffers of 500; sleeps of 475 to 977 ms before a `longjmp`; 14 `longjmp`s, each followed by a restore in the same call; the first computed gap -1,769,946,455 ticks (P31) |
| The PC demo as built (2026-10-26), eight threads (§4), sampled for 10 s | — | `make build` clean under `-Werror`; runs until stopped; the 8 listed addresses, then counters 100, 200 and 300 printed once each and 400 to 465 (66 lines), all from `MySecThreadItem` threads; 73 "Stack saved…" lines, each naming the chosen head (P24); 65 "Context restored." lines, each followed by the same thread printing again |
| `ef5eeee` compiled alone (`-Werror`) | — | does not compile: "use of undeclared identifier 'set_jmp'; did you mean 'setjmp'?" and "use of undeclared identifier 'dummy'" |
| The kernel loop in `start()` (2026-10-28, not kept, A12): the eight-thread demo for 12 s; a harness of three threads that yield 3 to 5 times and end, and one with a 16-byte buffer, at `-O0`, `-O2` and `-Os` | — | 23, 12, 6 and 3 lines for nice 500, 1000, 2000 and 5000; every local kept across the switches, the threads ended one by one and `start()` returned; the 16-byte buffer stopped the kernel with the overflow message; slices of 129 to 161 bytes |
| The PC demo as built (2026-10-28), Gustavo's redesign (A12) | — | `make build` clean under `-Werror`; the 8 listed addresses, 8 "Stack saved…" lines, 100, 200, 300 and 400 printed once each, then "Main thread finished.", exit 0, 0.51 s in all; no overflow message (P27) |
| The PC demo under AddressSanitizer (`-fsanitize=address`) | — | stops at the first switch: "stack-buffer-overflow … READ of size 160" (2026-10-22) and "READ of size 192" (2026-10-23) in `thread_context::save`; "stack-use-after-scope … READ of size 416" (2026-10-28), also in `save`, called from `yield()`, at an address inside `start()`'s frame (P23) |
| `ssize_t` on the targets (A11) | — | defined on the PC and on the ESP32 (newlib: `typedef _ssize_t ssize_t;`); defined nowhere in the AVR toolchain's headers (avr-gcc 7.3) |
| ESP32, the probe sketch, code of 2026-10-14 (the two-parameter `compare`), `--warnings all` | — | 522,313 bytes of flash, 29,288 bytes of RAM, no warning from AtomicX |
| `sort` integrity (A14): 20,000 random lists of 0 to 14 items, keys 0 to 4, compared with `std::stable_sort`; forward and backward walks, `prev` links, null ends, `tail`, `count()` | the merge sort (`e6e9d97`): 0 corrupted, 0 out of order | the swap sort: 0 corrupted, 0 out of order |
| `swap` (A14): neighbours in both orders, one item between, far apart, head and tail in both orders, an item with itself, two items, a null item, items of two lists; then 20,000 rounds of 8 random swaps against a model | — | all cases right; 0 of 20,000 rounds wrong |
| Comparisons of one `sort` on 8 threads (A14): in order; one thread out of place; reversed | merge sort: 12; 14; 12 | swap sort: 7; 13; 28 |
| The schedule order through the swap sort (A7, A14): §3.4's example, `RUNNING` before `TERMINATED`, one thread of each state, the tick wrap | — | C D B A E; Y Z X T; R N U W S K; Y X |
| Where the demo's list breaks (P23), code of `e6e9d97` with a check after `restore` | — | the list is corrupt after the first `restore`; it wrote `[0x…df37, 0x…dfb7)`, which holds the `next` (`0x…dfa8`) and `prev` (`0x…dfb0`) of the thread object at `0x…dfa0` |
| The slice range fix (P23, 2026-10-29): the working copy with a check after every `restore` (the list walked from the head, every `prev` and `tail` checked), the demo for 12 s | `save` copies `[0x…1f27, 0x…1fa7)` and `[0x…1f27, 0x…1fd7)`, above the top (another run, so other addresses); 1 restore, the list corrupt after it; counters 100, 200, 300, 400 and 401, then nothing more | `save` copies `[0x…9e77, 0x…9f27)` and `[0x…9ea7, 0x…9f27)`, below the top (176 and 128 bytes); 112 restores, the list intact after each, no overflow message; counters 100 to 139, 200 to 216, 300 to 310 and 400 to 404, each going up by one; `make build` clean under `-Werror` |
| AddressSanitizer with the slice range fix (2026-10-29) | "stack-use-after-scope … READ of size 416" in `save`, at `start()`'s `dummy` | "stack-buffer-overflow … READ of size 416" in `save`, at the byte after `yield()`'s `dummy`, where the slice starts; its own hint: "this may be a false positive if your program uses some custom stack unwind mechanism" |
| `TRACE(…)` (A15, 2026-10-31): a test of `TRACE("plain " << 42 << std::endl << std::flush);`, `TRACE("side effect " << side());`, a message with commas, `TRACE(not_declared_anywhere << std::cout);` and `if (c) TRACE(…); else …`, `-Wall -Wextra -Werror`; then the library | `TRACE << …` with `(void)0`: without `TRACE_ON`, "invalid operands to binary expression ('void' and 'const char[7]')"; `make build` fails the same way at `yield()`'s `TRACE` | PC without `TRACE_ON` (no `<iostream>` included): nothing printed, `side()` called 0 times, the undeclared identifier compiles, the `else` taken once; with it: `[TRACE] t2.cpp:19 - plain 42` and the other lines, `side()` called once, `if` and `else` once each. avr-g++ 7.3 (`-mmcu=atmega328p`) without `TRACE_ON`: compiles and links; with it: "iostream: No such file or directory" (P1). A `static` function used only in traces: "defined but not used" without `TRACE_ON`. `make build` and `-DTRACE_ON` clean; `yield()`'s call preprocessed: `do { } while (0);` |
| `TRACE=1` in the Makefile (A15, 2026-10-31): `make -n` of `build`, `build TRACE=1`, `build TRACE=1 SYMBOLS=1`, `nano`, `nano TRACE=1`, `nano_flash TRACE=1`, `nodemcu`, `nodemcu TRACE=1` | first cut, native only: `-DTRACE_ON` on the 2 compile lines of the two `TRACE=1` builds, nothing for the Arduino targets | `-DTRACE_ON` on the 2 compile lines of the two `TRACE=1` builds; `arduino-cli compile --upload --build-property "compiler.cpp.extra_flags=-DTRACE_ON" --fqbn …` for the three Arduino targets with `TRACE=1`, no property without it; `build` has no flag |
| The demo without and with `TRACE=1` (A15, 2026-10-31), 6 s each | — | `make build` and `make build TRACE=1` clean under `-Werror`; 44 lines with 0 `[TRACE]` and 0 `[YIELD]` lines, then 215 lines with 171 `[TRACE]` lines (`[TRACE] atomicx/atomicx.cpp:340 - [YIELD] Stack saved for thread at address: …`); the same counters in the same order both ways: 100, 200, 300, 400, 101 to 106, 201, 202, 301, 302, … |
| `dump_data` through `TRACE` (A10, A15, 2026-10-31), with `TRACE_ON`, against the code before the change | — | 16 zero bytes, 23 mixed bytes with a label, the printable edges 0x1f, 0x20, 0x7e, 0x7f, 0 bytes with a label, 8 bytes, 49 random bytes with a label: 12 lines, each with the prefix, equal byte for byte to the old output once it is removed; all 256 byte values: equal (0x20 to 0x7e is `isprint()`'s range) |
| Code left without `TRACE_ON` (A15, 2026-10-31), `atomicx.cpp` at `-Os` | `dump_data` with `isprint()`: a 30-instruction loop that still calls the library for every byte | `dump_data`: `ret`; `dump_threads`: one load of `_context` (it is `volatile`) and `ret`; 216 instructions in `dump_data` with `TRACE_ON` |
| The line end in the macro (A15, 2026-10-31): the PC demo with `TRACE=1` for 6 s, against the run before the change, addresses and line numbers masked; `dump_data` against the original | — | 215 lines, 171 `[TRACE]`, identical; without `TRACE=1` 44 lines, 0 `[TRACE]`; `dump_data` equal on the 6 inputs and all 256 byte values; at `-Os` without `TRACE_ON` still a single `ret` |
| `serial_stream` on AVR (A15, 2026-10-31): a Nano sketch with `atomicx.cpp`'s `TRACE` block (its lines 50 to 70, copied) and traces of a `const char*`, a `char[]`, a `const` and a plain struct pointer, a `volatile char*`, `int`, `uint32_t`, `size_t`, `char` and `long`; `arduino-cli`, `arduino:avr:nano` | — | with `compiler.cpp.extra_flags=-DTRACE_ON`: compiles, 2,524 bytes of flash, 510 bytes of RAM, `[TRACE] ` in the image; without it: 1,438 bytes, 184 bytes, no `[TRACE] ` (not run on a board) |
| `arduino/simple` with and without `TRACE_ON` (A15, 2026-10-31) | — | ESP32 (`esp32:esp32:esp32`): compiles and links both ways, 541,845 and 540,917 bytes of flash, 29,304 bytes of RAM, `[TRACE] ` in the image only with it (P3 fixed). Nano: "atomicx.h:33:10: fatal error: iostream: No such file or directory" both ways (P1) |
| The Nano build of `arduino/simple` (A16, 2026-10-31), `arduino-cli compile --fqbn arduino:avr:nano --warnings all` | "atomicx.h:33:10: fatal error: iostream: No such file or directory"; with that include removed: "'cerr' is not a member of 'std'" and "'endl' is not a member of 'std'" (2 lines each), "'ssize_t' was not declared in this scope" | compiles and links: 5,850 bytes of flash, 484 bytes of RAM (1,564 free); with `TRACE_ON` 6,438 and 774; 0 warnings from AtomicX or the sketch (4 from the core's own `new.cpp`) |
| The ESP32 build of `arduino/simple` after A16 (2026-10-31), `--warnings all` | 541,845 bytes of flash and 29,304 bytes of RAM with `TRACE_ON`, 540,917 and 29,304 without (A15's row) | 287,252 and 22,164 with `TRACE_ON`, 286,312 and 22,164 without; 3 warnings from AtomicX (P37) |
| Sizes on AVR (A16, 2026-10-31): `-fstack-usage` with `-fno-lto`; `sizeof` through a template that does not compile | — | static frames: `start()` 9 bytes, `MyThreadItem::run()` and `MySecThreadItem::run()` 2 (`dump()` inlined), `yield()` 13, `sort` 12, `compare` 14, `swap` 6, `dump_data` 85, `setup()` 460; `sizeof(MySecThreadItem)` 55, `sizeof(jmp_buf)` 23 |
| The sleep across the wrap (A16, 2026-10-31): the old `ssize_t` gap and the new one, side by side | due in 300: 300; due now: 0; 200 overdue: 0; due after the wrap, now before it: 384; due before the wrap, now after it: 4,294,967,152; deadline and now both after the wrap: 4,294,967,536 | 300; 0; 0; 384; 0; 240 |
| The PC demo with the new sleep (A16, 2026-10-31), 12 s, against the same code with the old sleep, two runs each (`main.cpp`'s counters now start at 1000 to 8000) | lines per printing thread: 39, 19, 11, 3 and 40, 17, 11, 5 | 37, 19, 9, 5 and 40, 17, 11, 5; the second run's first 16 counters equal to both old runs: the differences are timing, not the sleep |
| `LOG_ERROR` on the PC (A16, 2026-10-31): a thread asking for 2^61 bytes | — | "Failed to allocate stack for thread item." on `stderr`, the program goes on, exit 0 (the same text as before) |

The probe sketch is `arduino/simple/simple.ino` with this added (written for the two-parameter `sort` of the time):

```cpp
ax::thread_context ctx;

struct probe_item : ax::thread_item
{
    probe_item(ax::thread_context& c) : thread_item(c, 64) {}
    void run() override {}
};

probe_item probe_a(ctx);
probe_item probe_b(ctx);

bool keep_order(const ax::auto_obj_list::item& a, const ax::auto_obj_list::item& b)
{
    (void)a;
    (void)b;
    return false;
}

void setup()
{
    ctx.sort(keep_order);
    Serial.println((unsigned)ctx.count());
    ctx.start();
}

void loop()
{
}
```

There is no automated test in the repository yet: these runs were made by hand, outside it (P13).

---

## 7. Open problems

Each problem was found while writing this file; its evidence column says how it was checked (a run, the compiler,
the history, or reading the code).

| # | Problem | Evidence | Consequence |
|---|---|---|---|
| P1 | *Fixed on 2026-10-31 (A16)*: the library wrote to `std::cout` and `std::cerr`, and the header included `<iostream>`. Now the header includes no stream, the traces print only with `TRACE=1` (A15), and the errors go to `std::cerr` on the PC and `Serial` on Arduino. | avr-gcc: `atomicx.h: fatal error: iostream: No such file or directory`; on the ESP32 the stream library cost 251,641 bytes of flash and 7,132 bytes of RAM (§6, the empty sketch). Since 2026-10-31 the Nano build links, and the ESP32 build is 254,605 bytes of flash and 7,140 bytes of RAM smaller (§6). | Nothing built for AVR (the Nano), and the ESP32 paid a quarter of a megabyte for logging. |
| P2 | *Fixed on 2026-10-31 (A16)*: `atomicx.cpp` includes `<iostream>` only when `ARDUINO` is not defined, and on Arduino it got it through the header. Nothing in `atomicx.cpp` uses a stream on Arduino any more. | The include block of `atomicx.cpp`; the ESP32 and Nano builds (§6). | Taking `<iostream>` out of the header would have broken every Arduino build while `make build` stayed green. |
| P3 | *Fixed in `e6e9d97`* (`setup()` and `loop()` added back; *measured* on 2026-10-31: the ESP32 build links, §6): `arduino/simple/simple.ino` had no `setup()` and no `loop()` (removed in `8eea8f3`). | ESP32 link: `undefined reference to setup()` and `loop()`. | The sketch did not link. |
| P4 | `item`, and every class derived from it except `thread_item`, can be copied: the implicit copy constructor copies the links without registering the copy, and the copy's destructor unlinks the original. | clang: "definition of implicit copy constructor for 'item' is deprecated because it has a user-provided destructor"; *measured*: 3 items visible, 2 after the copy's destruction, the original still alive. | An object copied by mistake silently disappears from its list. Threads are protected only by accident (R3.7). |
| P5 | `thread_item` has a virtual `run()` but no virtual destructor, and neither has `item`. | clang `-Wnon-virtual-dtor`. | Deleting a thread through a base pointer is undefined behaviour. |
| P6 | *Fixed in `1245c3b`* (`lastRunTime = 0`, `nice = 1`): `st_data::lastRunTime` and `nice` had no initializer. | The declaration of `st_data`. | They were indeterminate for a thread on the stack or the heap, and 0 for a global thread. |
| P7 | A thread whose buffer allocation failed stays registered, with no buffer. | A2, R3.3. | `yield()` would copy a slice into a null buffer; on AVR, address 0 holds the processor's registers. |
| P8 | The scheduler is not complete: there is no kernel loop, and the switch of §3.5 does not resume threads. | §3.5, *measured*. | The library does not multitask yet. |
| P9 | The "Allocated stack" line prints the buffer's address as `1`. | The demo's output. Before C++23, `operator<<` has no overload for a `volatile` pointer, so the pointer converts to `bool`. | A misleading log. |
| P10 | `getNextItem(current)` and `getPrevItem(current)` read their argument, not `this`. | Their definitions. | `a->getNextItem(b)` returns `b`'s neighbour: an unusual interface. |
| P11 | A plain `auto_obj_list::item` can be registered in a `thread_context`. | `ax::auto_obj_list::item x(ctx);` compiles and is counted. | `start()`, `compare` and `yield()` would treat it as a `thread_item`: undefined behaviour. |
| P12 | `main.cpp`'s `FAKE_TIMER` branch does not compile, and its `sleepTicks` never returns for any `nSleep` but 0. | With `-DFAKE_TIMER`: "use of undeclared identifier 'nCounter'"; `while (nSleep);`. | That branch is unusable. |
| P13 | No test target. (The CI part is *fixed on 2026-10-31*, A17: the workflow was GitHub's autotools starter, `./configure`, `make check`, `make distcheck`, none of which exists, and every run since `8607977` failed at `./configure`; the last, 41239665284 on `de25d6a`, "./configure: No such file or directory", exit 127.) | No `make test`. | D3's "tests that fail before and pass after" cannot live in the repository yet; the CI's demo run only informs. |
| P14 | No license is settled. | `atomicx.h` and `atomicx.cpp` say MIT (since `42b2515`); the Makefile's header copies the GNU GPL v3 title and preamble and points to a LICENSE file; no LICENSE or COPYING file exists anywhere in the history. atomicx v1 has an MIT LICENSE file. | Users cannot know which license applies. |
| P15 | `make document` runs `Doxygen` with no Doxyfile; `README.md` is a stub whose one command uses the path `simple`, which is `arduino/simple` from the repository's root. | The Makefile; `README.md`. | No generated documentation; the README's command fails as written. |
| P16 | The stack buffer comes from the heap (`new`). | A2. The 2024 prototype takes buffers from the program (`VMEM`); v1 takes them from the program, or self-managed from the heap, where `Yield()` frees the buffer and `malloc`s a larger one whenever the slice outgrows it (§3.6). | On an AVR, heap use at run time risks fragmentation; where the buffers come from is open. |
| P17 | A1 made the constructors, the destructors, `add`, `remove` and `start` out-of-line functions that refer to `std::cout`. Since 2026-10-31, without `TRACE=1`, only the `thread_item` constructor and `yield()` refer to a stream, `std::cerr` (not re-measured on the ESP32). | *measured* (§6): +184 bytes with the probe sketch; +251,641 bytes of flash and +7,132 bytes of RAM with an empty sketch. | A program that includes AtomicX and uses none of it pays for the stream library; solving P1 removes the cost. Since 2026-10-31 nothing refers to a stream on Arduino (A16): the ESP32 build of `arduino/simple` without `TRACE_ON` went from 540,917 to 286,312 bytes of flash and from 29,304 to 22,164 bytes of RAM (§6). |
| P18 | `.vscode/settings.json` is tracked although `.gitignore` ignores `.vscode`; it holds machine-specific paths. | `git ls-files`; the file. | Editor settings of one machine travel with the repository. |
| P19 | The reasons for A5 and A6, and the alternatives Gustavo rejected, are not recorded. | A5, A6. | What `volatile` on `add`, `remove` and `parent` must guarantee is unclear (it gives no atomicity, R2.4). |
| P20 | *Fixed in `1245c3b`*: `make build` failed since A6, because `_l_stackPointer` and `_parent` were private and never used. | `-Werror,-Wunused-private-field` (§6). | The PC demo did not build. |
| P21 | `auto_obj_list`, and so `thread_context`, can be copied (R2.6). | *measured*: a context passed by value compiles without a warning; after a copy, destroying a thread leaves the copy's `head` pointing at it. | A context copied by mistake holds dangling pointers. |
| P22 | Threads with the same rank and the same deadline keep the list's order. (Ordering by deadline is built since 2026-10-23, and reading now once per sort since `1245c3b`: A7, R3.16, R3.17.) Proposed, waiting for Gustavo: breaking the tie by the least recently run. | A7, R3.18. | Equal threads are not rotated: of two threads with the same deadline, the first in list order always comes first. |
| P23 | *Fixed on 2026-10-29*: `save` and `restore` copy the `size` bytes below `stackTop` (R3.12). They were given `_stk_top` as the start of the slice, so they copy `[_stk_top, _stk_top + size)`, the bytes above the threads' stack; the slice is `[_stk_bottom, _stk_top)`. | AddressSanitizer: "stack-buffer-overflow … READ of size 160" (2026-10-22) and 192 (2026-10-23) in `save`, inside `start()`'s frame; "stack-use-after-scope … READ of size 416" (2026-10-28) (§6). On 2026-10-29, a check after `restore` found the list corrupt after the first one: it had written back a thread object's `next` and `prev` (§6). | `save` stores the wrong bytes, and `restore` would overwrite `start()`'s and `main()`'s frames. |
| P24 | `yield()` saves registers with `setjmp` into the next thread's `_register_context`, not its own; since `e6e9d97` the `longjmp` after the sleep targets that same `jmp_buf`, so it lands in the same `yield()` call. The resumed thread continues only because the restore then rewrites this call's frames with its saved frames, at the same addresses. | `yield()`'s code; *measured* until 2026-10-26: each `longjmp` on that `jmp_buf` was followed by a restore in the same call; since 2026-10-29 the demo's threads take turns that way (§6). | A paused thread's own registers are never saved: a thread whose frames under `start()` differ from the resumer's (another call depth, another optimisation level) would resume with the wrong registers. |
| P25 | *Fixed in `e6e9d97`*: a new thread's `run()` was called from inside the yielding thread's `yield()`; it is called from `start()` since. | *measured*: slices of 112, 240, 368 and 496 bytes (2026-10-22), of 112, 288, 464 and 640 bytes (2026-10-23), a fixed step more for each thread started; 128 and 176 bytes on 2026-10-29, for every thread of each class (§6). | Threads ran nested: each slice held the frames of every thread started before it, and a finished thread returned into the `yield()` of the thread that started it. |
| P26 | After the `longjmp`, the restore reads `this` and `_context._current` from the frame it is about to overwrite, and the stamp after it, `current->_data.lastRunTime = getTick()`, reads `current` from the overwritten frame. | `yield()`'s code; at `-O0` both live in the frame. | After the copy they hold the resumed thread's saved values: the stamp may go to the thread the resumed one chose when it paused. A restore must read only data outside the stack until the copy is done (§3.6, proposed step 4). |
| P27 | `_stackSize` (`stackSize` until 2026-10-28) is `stack_size * sizeof(size_t)` while the buffer holds `stack_size` bytes. Since 2026-10-28 `yield()` compares the slice with it. | The constructor; *measured*: slices of 368 and 496 bytes for 250-byte buffers (2026-10-22), of 640 bytes for 500-byte buffers (2026-10-23); no overflow message for 500-byte buffers on 2026-10-28, where the limit is 4,000 bytes. | The check lets `save` write up to 8 times past the end of the buffer on a 64-bit PC. On AVR, where `size_t` has 2 bytes, the limit is twice the buffer: 128 bytes for the sketch's 64 (A16). |
| P28 | *Fixed on 2026-10-29 (A14)*: the doc comment above `sort` in `atomicx.h` described `cmp` as `bool cmp(const item& a, const item& b)`. | The header. | The comment does not describe the three-parameter `sort` (D4). |
| P29 | `NOW`, `SLEEPING` and `WAITING` are ranked by `compare` but never set. | §5. | Their place in the schedule order is defined, their meaning is not. |
| P30 | *Fixed on 2026-10-31 (A16)*: `yield()` computed the time to sleep as `(ssize_t)lastRunTime + (ssize_t)nice - getTick()`: not wrap-safe, unlike the schedule order (R3.16), and `ssize_t` is POSIX, not C++11. | `yield()`'s code; the AVR toolchain's headers define no `ssize_t` (§6). The PC demo's `getTick()` is epoch milliseconds cut to 32 bits (about 1.77e9 on 2026-10-23), so its tick wraps for real every 49.7 days. *Measured* on 2026-10-31: a deadline before the wrap with now after it gave a sleep of 4,294,967,152 ticks; 0 with the fix (§6). | On the PC (64-bit `ssize_t`), when a thread's last run was before the wrap and now after it, the program would sleep about 49.7 days. It did not compile on AVR. |
| P31 | *Fixed in `ef5eeee`*: `start()` did not stamp the first thread's `lastRunTime`, which stayed 0. | `start()`'s code; *measured* before: the first computed gap was -1,769,946,455 ticks (§6). | The first thread looked long overdue until it was chosen. |
| P32 | *Fixed in `e6e9d97`*: when `yield()` chose a `RUNNING` thread, it slept until that thread was due and then `longjmp`ed to `_top_context`, so `start()` returned and the system ended. A `RUNNING` head is now resumed by a `longjmp` to its `_register_context` (P24). | `yield()`'s code (2026-10-28); *measured* then: the demo ended after about 0.5 s, each thread having run once (§6). | No thread ran twice. `start()` still returns on an overflow, on a head in another state, or when a thread's `run()` returns (R3.10). |
| P33 | `start()` sets `_stk_top` to the address of a `volatile char` declared in an inner block, whose lifetime has ended when it is used. Until `e6e9d97` the restore's size used `thread_item::_stk_top`, never set (null); it uses `_context._stk_top` since. | `start()`'s and `yield()`'s code (2026-10-28); AddressSanitizer: "stack-use-after-scope" in `save` (§6). | The top mark is an address the compiler may reuse. |
| P34 | A12's reference, "my original design from the atomicx 1", is not recorded; §3.6 cites v1.3.0's `Start()` as the loop that starts and resumes threads. | A12, §3.6. | Which v1 design `yield()` follows, and so what §3.6 should describe, is unclear. |
| P35 | *Fixed on 2026-10-31*: this specification was not synced with `e6e9d97` ("saving", 2026-10-28). | `git show e6e9d97`. | §3.3, §3.5, P24, P25 and P32 described the code before it. |
| P36 | The Nano build has not been run on a board, nor in a simulator (none is installed here). The slice size on AVR is *estimated* from `-fstack-usage` without LTO; the real build uses LTO. | A16; §6. | Whether the threads switch on the Nano, and whether 64-byte stacks are enough, is unknown until it runs; `Free memory` and the overflow message would tell. |
| P37 | The ESP32 core compiles as C++20 or newer, where `_count++` and `_count--` on a `volatile` member (`add`, `remove`) are deprecated, and `setNice`'s `(volatile ax::time)` cast is ignored: 3 warnings with `--warnings all`. | The ESP32 build log (2026-10-31, §6). | No error today; a later standard may reject them. The project's rule is C++11 (R1.3). |
---

## 8. Glossary

- **Context** — an `ax::thread_context`: the threads that take turns on one processor.
- **Context switch** — saving one thread's state and restoring another's, so that the second goes on where it
  stopped.
- **Cooperative multithreading** — threads take turns on one processor, and each one gives the processor back by
  itself; nothing preempts it.
- **Deadline** — when a thread becomes due: `lastRunTime + nice`; the second key of the schedule order (§3.4).
- **Due** — a thread whose deadline is not after now, tested wrap-safe (§3.4).
- **Intrusive list** — a linked list whose links are members of the listed objects; it never allocates.
- **Kernel** — in atomicx v1, the code in `Start()` that picks the next thread and switches to it; AtomicX 2 has none
  yet (§3.6).
- **Nice** — how many ticks after its last run a thread becomes due, set with `setNice()` (AtomicX 2); in atomicx v1,
  how many ticks a thread sleeps when it yields without saying how long.
- **ODR (one definition rule)** — a non-inline function must be defined exactly once in a program; R1.2 keeps it.
- **Rank** — the first key of the schedule order, from a thread's state: `NOW` and `READY`, `RUNNING`, `SLEEPING` and
  `WAITING`, `TERMINATED` (§3.4).
- **Schedule order** — the order `thread_item::compare` gives a context: by rank, then by deadline, the earliest
  first, ties in list order (§3.4).
- **Scheduler** — the code that chooses which thread runs next and switches to it; not complete yet (§3.5, §3.6, P8).
- **Self-registering object** — an object whose constructor adds it to a list and whose destructor removes it.
- **setjmp / longjmp** — the C library pair that saves the processor's registers at a point (`setjmp`) and later jumps
  back to it (`longjmp`); atomicx v1 switches threads with them, and AtomicX 2's `yield()` uses them (§3.5).
- **Stable sort** — a sort that keeps equal elements in the order they had.
- **Stack buffer** — the memory a thread owns for a copy of its stack (`_v_stackPointer`, `stack_size` bytes).
- **Stack slice** — the part of the stack a thread is using when it pauses: from the top mark down to the frame of the
  call that paused.
- **Strict weak ordering** — the rules of a "less than": never `a < a`; if `a < b` then not `b < a`; transitive; and
  "neither is less" behaves as equality.
- **Thread** — an object of a class derived from `ax::thread_item`; its `run()` is the code it executes (§3.1).
- **Tick** — the unit of `ax::time`, chosen by the program (§4).
- **Unity build** — compiling several source files as one by including them as text, as the Arduino sketch does with
  `atomicx.cpp` (R1.2).
- **Work in progress** — code that exists but does not do its job yet; described in this file, not normative.
- **Yield** — a thread handing the processor back by itself so that another can run; atomicx v1's `Yield()`, and
  AtomicX 2's `thread_item::yield()` (§3.5).
- **Zero stack displacement** — atomicx v1's term: the stack pointer never moves into a thread's buffer; the buffer
  only holds a copy of the slice while the thread is paused.
