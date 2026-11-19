# PlanJOS Architecture Specification

PlanJOS is a minimalist, single core, capability based microkernel OS designed for desktop computing, targeting ARM64 (da Raspberry Pi) first.

Where UNIX treats system resources as untyped byte streams, **PlanJOS treats everything as a typed interface**; it is an isolated graph of uspace services communicating over low latency, register switched Inter Process Communication (IPC). Filesystems, drivers, windowing, and audio live entirely in user space. We inted to omit SMP, network transparency, and runtime kernel allocation, PlanJOS prioritizes **minimalism, low latency, and verifiable correctness**.

---

## Formal Lineage & Core Invariants

Per the glorious, wonderful, immaculate model created by Chairwoman Charlotte, we will be translating its state-machine guarantees onto ARM64 hardware under five invariants:

| Invariant | Specification Rule |
|---|---|
| **Static Kernel Memory** | Zero runtime heap allocation. Kernel objects, handle tables, and page tables are preallocated in fixed boot time pools. |
| **Monotonic Object IDs** | Every kernel object receives a unique 64 bit integer ID (`>= 1`) from a strictly increasing counter. In v0, IDs are never reused. |
| **Atomic Syscalls** | Every syscall validates all handles, rights, and arguments before mutating any kernel state. Failed calls leave state untouched. |
| **O(1) Flat Capabilities** | Each process owns a fixed size handle array (`256` slots default), trading CNode tree indirection for single index lookup speed. |
| **Strict Slowpath, Direct Fastpath** | Slowpaths obey top down call layering (`Layer 5 -> Layer 0`). **Fastpaths** (synchronous IPC rendezvous and page fault handoffs) bypass runqueues and jump directly to `Layer 0` context switching. |

---

## Kernel Objects

The kernel tracks **7 object types** and enforces no memory quotas; resource policy belongs to uspace pagers.

| Object | Responsibility & State |
|---|---|
| **1. Process** | Protection and resource container. Holds a `VSpace` handle, a flat array of object handles, and a pager `Endpoint` handle. |
| **2. Thread** | Execution unit bound to an owning `Process`. Holds saved CPU context, fixed priority (`0–31`), state, and IPC message block. |
| **3. Frame** | Identity token representing one `4 KiB` page of physical memory (code, data, or MMIO buffers). |
| **4. Endpoint** | Synchronous rendezvous IPC port maintaining FIFO queues of waiting senders and receivers. |
| **5. Notification** | Asynchronous signaling object backed by an atomic pending bit word. |
| **6. VSpace** | Hardware address space root backed by boot preallocated ARM64 page tables. |
| **7. IRQ** | Hardware interrupt line object granted to the root task at boot; binds to a `Notification` for lock free driver delivery. |

### Thread States & Cooperative Scheduling

PlanJOS is **non preemptive (cooperative)**: the running thread occupies the single core until it blocks on IPC/notifications, yields, or exits. Only a `Running` thread may invoke syscalls (except during boot).

| State | Meaning |
|---|---|
| `Running` | Currently executing on the CPU. |
| `Ready` | Queued in a priority runqueue awaiting dispatch. |
| `BlockedSend` | Waiting in an `Endpoint` sender queue for a receiver. |
| `BlockedRecv` | Waiting in an `Endpoint` receiver queue for a sender. |
| `BlockedNotif` | Waiting on a `Notification` bitmask (with optional timeout). |
| `Dead` | Terminated; never scheduled again. |

### Capabilities & Rights

Processes never touch raw kernel pointers. Syscalls pass integer indices into the caller's `Process` handle table, where each occupied slot stores `(ObjId, RightsMask)`. Object creators automatically receive full rights on new objects, and `Grant` delegates a subset of the caller's rights to another process.

| Bit | Right | Validated On |
|---|---|---|
| `0` | **Read** | Frames / readable objects |
| `1` | **Write** | Frames, VSpaces, Process thread creation |
| `2` | **Execute** | Executable frame mappings |
| `3` | **Map** | Inserting a `Frame` into a `VSpace` |
| `4` | **Send** | Endpoints and Notifications |
| `5` | **Receive** | Endpoints, Notifications, and IRQ acknowledgments |

Irrelevant rights bits on an object type (e.g., `Read` on an `Endpoint`) are ignored.

---

## Layered Kernel Architecture

Build rules enforce layering at compile time: `Layer N` may only include headers from `Layer N`, `Layer N-1`, and shared `types.h`. Hardware access is restricted exclusively to `Layer 0`.

| Layer | Subsystem | Core Responsibilities |
|---|---|---|
| **Layer 0** | **Arch Primitives** | ARM64 CPU init, EL1 exception vector table, MMU & full-ASID TLB invalidation, interrupt masking, fault register reads, context switch, cycle counter, and idle loop. |
| **Layer 1** | **Kernel Storage** | Fixed size object pools, per process flat handle arrays, and physical `4 KiB` frame bitmap allocator. |
| **Layer 2** | **Virtual Memory** | Maps/unmaps `Frame` objects in a `VSpace` (verifying `Map` rights and alignment), updates PTE permissions, switches active `VSpace`, and reads CPU fault state. |
| **Layer 3** | **Scheduler** | Fixed-priority cooperative dispatcher (`32` FIFO runqueues + `32` bit runnable bitmap). Pops highest priority `Ready` thread or enters `Layer 0` idle. |
| **Layer 4** | **Messaging (IPC)** | Synchronous `Endpoint` rendezvous, `Notification` bit flips, atomic handle transfer, and forwarding page faults to the process pager `Endpoint`. |
| **Layer 5** | **Syscall Boundary** | Sole user mode trap entry. Validates syscall IDs, arguments, and handle rights before invoking lower layers. |

### Fixed IPC Message Format

Threads carry a fixed size message block inside kernel thread state eliminating variable length copies, user memory dereferences during IPC, and per word staging syscalls. Small payloads ride directly in ARM64 general registers (`x0–x7`).

| Field | Width / Count | Purpose |
|---|---|---|
| **Tag Word** | `1 x 64 bit` | Encodes target Interface ID, Method ID, active capability slot count (`0–4`), and data word count (`0–6`). |
| **Source Slots** | `4 x HandleIdx` | Caller handle indices to transfer. |
| **Dest Slots** | `4 x HandleIdx` | Target slots in the receiver's handle table. |
| **Rights Masks** | `4 x Rights` | Maximum rights granted on each transferred handle. |
| **Data Words** | `6 x 64 bit` | Inline typed method arguments / return values. |

If any transferred capability cannot fit in the destination handle table, the IPC aborts atomically with `DestinationTableFull` before mutating state.

---

## Syscall Spec

All object creation syscalls execute atomically without rescheduling.

| Category | Syscall | Semantics |
|---|---|---|
| **Bootstrap** | `CreateVSpace` | Allocates a `VSpace`. Allowed without a current thread *only* during Boot Phase 1 to create the root space. |
| **Bootstrap** | `Bootstrap` | One time root thread creation in root `VSpace` (`!booted`, no current thread). Grants all space rights, sets `Running`, marks `booted = true`. |
| **Lifecycle** | `CreateProcess` | Allocates an empty `Process` bound to a `VSpace` and returns a full rights handle. |
| **Lifecycle** | `SetPager` | Binds a fault handling `Endpoint` to a `Process`. |
| **Lifecycle** | `CreateThread` | Requires `Write` on target `Process`. Initializes PC, SP, argument, and priority; places new thread at back of `Ready` queue; caller continues. |
| **Lifecycle** | `Yield` | Moves caller to back of its priority `Ready` queue and dispatches front `Ready` thread (caller continues if queue is empty). |
| **Lifecycle** | `Exit` | Marks caller `Dead` and dispatches next `Ready` thread (or idles). |
| **Memory** | `AllocFrame` | Allocates a `4 KiB` physical `Frame` from the bitmap pool with full rights. |
| **Memory** | `MapFrame` | Requires `Map` on `Frame` and unmapped page aligned VA; inserts PTE into target `VSpace`. |
| **Memory** | `UnmapFrame` | Removes PTE at VA in target `VSpace` and invalidates TLB entry. |
| **Capabilities** | `Grant` | Widens target process's handle rights for an object by `caller_rights & requested_rights`. |
| **IPC** | `CreateEndpoint` | Allocates a synchronous `Endpoint` and returns a full rights handle. |
| **IPC** | `Send` / `Call` | Requires `Send`. **Fastpath** (receiver `Y` waiting): copies registers to `Y`, demotes caller to `Ready` (or `BlockedRecv` on `Call` with transient reply cap), and runs `Y` immediately. **Slowpath**: blocks caller in sender queue and dispatches next thread. |
| **IPC** | `Receive` | Requires `Receive`. **Fastpath** (sender `Y` waiting): copies `Y`'s registers to caller, moves `Y` to back of `Ready` queue, and **caller keeps running** (asymmetric with `Send`). **Slowpath**: blocks caller in receiver queue. |
| **IPC** | `Reply` | Consumes transient reply capability from `Call` to wake the waiting caller directly. |
| **Signaling** | `CreateNotif` | Allocates an asynchronous `Notification` object. |
| **Signaling** | `Signal` / `Wait` | `Signal` atomically ORs bits on a `Notification` (waking waiter if blocked); `Wait` blocks until non zero or timeout. |
| **Interrupts** | `BindIRQ` | Attaches an `IRQ` handle to a `Notification` so hardware interrupts flip its pending bit in `Layer 0`. |
| **Interrupts** | `AckIRQ` | Validates caller's `IRQ` handle and unmasks/acknowledges the hardware interrupt line. |

---

## Error Codes

Syscalls return `0` (or a positive handle index) on success and a negative status code on failure:

| Error Code | Trigger Condition |
|---|---|
| `InvalidCapability` | Handle index is out of bounds or references an empty slot. |
| `RightsViolation` | Handle lacks the required permission bit for the operation. |
| `NoObject` | Referenced object ID does not exist. |
| `NoMemory` | Static object pool or physical frame bitmap is exhausted. |
| `BadArgument` | Misaligned virtual address, invalid priority, or malformed tag. |
| `AlreadyExists` | Target virtual address or binding slot is already occupied. |
| `EmptySlot` | Source handle slot specified for transfer is empty. |
| `InvalidSlot` | Destination handle slot index is out of range. |
| `DestinationTableFull` | Receiver lacks free handle slots to accept IPC capability transfer. |
| `SizeMismatch` | Message tag word count exceeds architectural payload bounds. |
| `Timeout` | `Wait` on a `Notification` expired before a signal arrived. |
| `InvalidState` | Bootstrap invariant violated (e.g., calling `Bootstrap` after `booted == true`). |

---

## Boot Lifecycle

| Phase | State | Permitted Operations |
|---|---|---|
| **Phase 0** | Empty Kernel | Static pools and `Layer 0` ARM64 tables initialized; `booted = false`, `current = NULL`. |
| **Phase 1** | Construction | Kernel invokes `CreateVSpace` without a current thread to establish the root address space. |
| **Phase 2** | Bootstrap | `Bootstrap` spawns the root thread with full rights on root `VSpace` and initial `IRQ`/pool handles; sets `booted = true` permanently. |
| **Phase 3** | Operation | Root task constructs userspace drivers and services via standard syscalls. If all threads exit, the kernel enters `Layer 0` idle permanently. |

---

## Open Design Decisions

Lastly, we should def talk some more about design direction in any further version down the line. Again, vielen lieben dank to Charlotte for providing the original documentation, as well as going through the work to do research and create the initial model. — aidan
