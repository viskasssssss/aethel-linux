![Aethel Logo](res/logo.png)

# Aethel Linux

**A Linux distribution built from scratch.**

Aethel Linux is an experimental Linux distribution focused on building a complete operating system environment from the ground up.

Instead of relying on an existing userspace, Aethel is built around its own shell, system utilities, userspace components, archive tools, package manager, and eventually its own graphical environment.

> **Status: Early development**

## Philosophy

Aethel is built step by step.

The goal is not to create the most feature-rich distribution as quickly as possible, but to understand and build the pieces that make a Linux system work.

Aethel intentionally keeps its userspace small, explicit, and close to the Linux kernel.

Much of the system is implemented directly on top of Linux system calls rather than relying on a traditional C library or large external frameworks.

## What is Aethel?

Aethel is built around a simple idea:

```text
Linux Kernel
     │
     ▼
Linux System Calls
     │
     ▼
Aethel Userspace
     │
     ├── Aethel Bash
     ├── System Utilities
     ├── Archive Support
     ├── YoYo Package Manager
     └── Applications
```

The Linux kernel provides the kernel layer, while Aethel builds its own environment on top of it.

## Aethel Bash

Aethel comes with its own shell — **Aethel Bash**.

It supports shell features such as:

* Pipes
* Command chaining
* Redirection
* Environment variables
* `PATH`
* Wildcards
* External ELF programs
* Process management
* Shell builtins

The shell communicates with Linux directly through Aethel's syscall interface.

## Built From Scratch

Aethel already includes several components implemented specifically for the project.

Among them are:

* Custom PID 1
* Custom syscall interface
* Custom shell
* Native system utilities
* ELF program execution
* `/proc` support
* TAR archive support
* GZIP decompression
* DEFLATE decompression
* Huffman decoding
* CRC32 verification

These components form the foundation for a larger Aethel userspace.

## YoYo

**YoYo** is Aethel's planned package manager.

It is being designed as the package and software management system for the distribution.

The project is also developing its own package repository infrastructure, allowing Aethel packages to eventually be distributed through a native ecosystem.

The goal is to make software management simple:

```bash
yoyo install <package>
yoyo remove <package>
yoyo update
```

YoYo is already under development.

## Architecture

Aethel currently uses the Linux kernel while providing its own userspace.

```text
┌──────────────────────────────┐
│          Applications        │
├──────────────────────────────┤
│       Aethel Shell / Tools   │
├──────────────────────────────┤
│        Aethel Userspace      │
├──────────────────────────────┤
│        Linux System Calls    │
├──────────────────────────────┤
│          Linux Kernel        │
├──────────────────────────────┤
│           Hardware           │
└──────────────────────────────┘
```

The architecture is intentionally kept understandable and modular, making it possible to experiment with individual parts of the system without hiding them behind large abstractions.

## Screenshots

![Aethel Linux](res/screenshot1.png)

## Building

### Requirements

* Linux or WSL
* GCC
* GNU Make
* Git
* QEMU
* cpio

### Clone

```bash
git clone https://github.com/viskasssssss/Aethel-Linux.git
cd Aethel-Linux
```

### Initialize the kernel

```bash
git submodule update --init --depth 1
```

### Build

```bash
make
```

### Run

```bash
make run
```

Aethel will boot inside QEMU using the Linux kernel and the generated initramfs.

## Project Structure

```text
Aethel-Linux/
├── build/          # Build artifacts
├── kernel/         # Linux kernel submodule
├── rootfs/         # Aethel root filesystem
├── src/            # Aethel userspace source code
└── Makefile        # Build system
```

## Roadmap

Aethel is still in early development. The system will grow gradually as new components are implemented.

* [x] Custom PID 1
* [x] Custom shell
* [x] Direct Linux syscall interface
* [x] Native system utilities
* [x] Environment variables
* [x] Shell expansion
* [x] Wildcard expansion
* [x] Pipelines and redirection
* [x] External ELF execution
* [x] `/proc` support
* [x] TAR support
* [x] GZIP / DEFLATE support
* [x] Archive extraction
* [ ] Dynamic memory allocation
* [ ] More system utilities
* [ ] Better filesystem support
* [ ] C library / userspace runtime
* [ ] Shared libraries
* [ ] Dynamic linking
* [ ] YoYo package manager
* [ ] Native package repositories
* [ ] Networking
* [ ] Graphical environment
* [ ] Desktop applications
* [ ] Native Aethel applications
* [ ] Complete desktop experience

The roadmap is intentionally flexible and will evolve with the project.

## Contributing

Aethel is an open-source project built around experimentation and learning.

You can:

* Fork the project
* Experiment with the userspace
* Add new utilities
* Improve existing components
* Work on new system components
* Suggest features
* Submit pull requests
* Create your own Aethel-based experiments

There are no strict rules about how a fork must work.

Aethel is both a Linux distribution and a playground for systems programming.

## License

Aethel Linux is licensed under the GNU General Public License v3.0 or later.

See the [LICENSE](LICENSE) file for the full license text.

---

**Aethel Linux — build the system, understand the system.**
