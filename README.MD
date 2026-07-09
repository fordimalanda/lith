# LITH - Light HTTP Server

LITH is a minimalist, high-performance, multi-threaded HTTP engine implemented in standard C11. Engineered to be exceptionally lightweight, modular, and portable, it provides native support for Windows (via MinGW-w64/MSYS2 and Winsock2), macOS, and Linux systems. It is designed with robust network lifecycle management, resource-efficient concurrency handlers, and zero external software dependencies.

## Key Features

- **Architectural Thread Pool**: Instantiates a fixed set of permanent, isolated worker threads upon boot. Eliminates runtime thread spawning/destruction overhead by utilizing a synchronized FIFO task queue managed via POSIX condition variables (`pthread_cond_t`) and mutexes (`pthread_mutex_t`).
- **High-Throughput Main Loop**: Features an optimized accept-loop that offloads incoming sockets instantly to the shared worker pool, freeing the core network thread to handle maximum incoming connection density per second without blocking on system calls.
- **Hybrid RAM Cache Subsystem (v1.0.9)**: Features a high-speed, thread-safe static RAM cache (`CacheEntry`) backed by readers-writer locks (`pthread_rwlock_t`). Supports an adaptive dual-mode: **Development Mode** (cache disabled for real-time asset modifications) and **Production Mode** with **Thread-Safe Hot-Reload** (automatically synchronizes RAM buffers if a disk file is updated without restarting the engine).
- **HTTP Keep-Alive Persistent Connections**: Keeps client sockets active for consecutive resource transfers within a secure window. Features automated connection recycling to maximize performance and minimize TCP handshake overhead.
- **Asynchronous Anti-Slowloris Guard (v1.0.9)**: Arms incoming sockets with a rigid 3-second network receive timeout (`SO_RCVTIMEO`) executed asynchronously by the allocated worker. Natively abstracted to support both UNIX `timeval` structures and Windows `DWORD` millisecond contexts.
- **Native Background Daemon Mode (Linux Only)**: Detaches the server process from the controlling terminal using a strict dual-`fork()` POSIX sequence. Isolates the process session (`setsid()`), resets the file creation mask (`umask(0)`), and seamlessly redirects standard streams to a continuous localized `lith.log` file.
- **Advanced I/O POST Payload Streaming**: Features automated network stream processing to fully parse incoming HTTP `POST` requests, handling chunked or fragmented bodies using precise `Content-Length` tracking and multi-stage buffer safety boundaries.
- **Thread-Safe Isolated Configuration**: Passes critical runtime data (such as the web root asset path) directly inside the thread pool context, completely removing reliance on shared static global variables or race-prone states.
- **External Configuration Engine**: Dynamically loads runtime environments from a `lith.conf` configuration file without requiring binary recompilation.
- **Dynamic MIME Type Resolution**: Automatically maps file extensions to their corresponding IANA media types (`.html`, `.css`, `.js`, `.json`, `.png`, etc.) for proper browser rendering.
- **Unified Security & Path Filters**: Protects against directory traversal vulnerabilities by parsing, validating, and rejecting malicious relative path patterns such as `..`.
- **Unified Error Canvas Layouts**: Serves clean, dark-themed HTML error pages for common HTTP status codes (`400`, `403`, `404`, `408`, and `500`), perfectly synchronized with connection persistence states.

---

## Quick Start

### 1. Build Compilation

LITH ships with a highly robust, cross-platform `Makefile` optimized for GNU Make. It automatically handles header dependency generation (`-MMD -MP`), links the native Winsock network library (`-lws2_32`) under Windows, compiles binary resources (`windres`), and adapts natively to both Linux/macOS and Windows environments.

To compile the project from scratch:

```bash
make clean
make
```

### 2. Configuration (`lith.conf`)

LITH loads its runtime configuration from a file named `lith.conf` located in the application's root directory.

```ini
# Network listening port
PORT=8001

# Directory containing static web assets
PUBLIC_DIR=views

# Cache strategy configuration (v1.0.9)
# 0 = DISABLED (Development Mode: Instant disk read on every request, perfect for design)
# 1 = ENABLED  (Production Mode: High-speed RAM caching with automated Hot-Reload)
USE_CACHE=0
```

### 3. Deployment & CLI Usage

Place your web assets inside the configured `public/` directory(e.g., `public/index.html`)..

LITH features a modern, explicit CLI command architecture. To execute the engine, you must use the `start` command routing path:

```bash
# Launch the engine in the foreground using standard configuration settings
# Launch the engine in the foreground using standard configuration settings
lith start

# Override the configured port dynamically via runtime arguments
lith start 8080

# Launch as a background daemon process detached from the terminal (Linux only)
lith start 8080 --daemon
```

**Note on Windows Portability**: If the `--daemon` or `-d` flag is invoked under Windows, LITH will catch the argument, log an informative warning indicating that background daemons require a UNIX environment, and gracefully fall back to foreground mode to maintain continuous execution.

To display context help or print the build version string:

```bash
lith --help
lith --version
```

---

## Technical Specifications
| Parameter | Default Value | Description |
|-----------|---------------|-------------|
| `LITH_VERSION` | `1.0.9` | Current stable release version |
| `BUFFER_SIZE` | `4096 bytes` | Standard socket read/write block buffer size |
| Maximum Buffer | `16 KB` | Maximum cumulative buffer space for HTTP headers and POST bodies |
| `BACKLOG` | `10` | Maximum number of pending client connections in the listen queue |
| Keep-Alive Timeout | `3 seconds` | `SO_RCVTIMEO` window before cleanly recycling a persistent socket |
| `MAX_KEEP_ALIVE_REQUESTS` | `100` | Maximum requests processed per single client TCP session |

## License

This software is distributed under the terms of the [FomaDev Public License (FPL)](LICENSE). Under this license, personal and educational use of compiled binaries is free, including within corporate environments. Commercial republication, direct source-code integration into competitive proprietary tools, or maintaining standalone independent forks is strictly restricted without explicit commercial authorization. For complete licensing terms, consult the accompanying [LICENSE](LICENSE) file.