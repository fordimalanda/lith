# LITH - Light HTTP Server

LITH is a minimalist, high-performance, multi-threaded HTTP server implemented in standard C11. Engineered to be exceptionally lightweight, modular, and portable, it provides native support for Windows (via w64devkit/MinGW-w64 and Winsock2), macOS, and Linux systems. It is designed with robust network lifecycle management, native multi-threading, and zero external software dependencies.

## Key Features

- **Multi-threaded Architecture**: Utilizes POSIX threads (`pthreads`) to handle multiple simultaneous client connections efficiently, preventing the core network listener from blocking.
- **HTTP Keep-Alive (v1.0.6 Persistent Connections)**: Features an intelligent network loop that keeps client sockets active for consecutive resource transfers. Includes case-insensitive header parsing and automated connection recycling to maximize performance and minimize TCP handshake overhead.
- **Advanced I/O Engine (POST Payload Streaming)**: Features automated network stream processing to fully parse incoming HTTP `POST` requests, including chunked or fragmented payloads using precise `Content-Length` validation and multi-stage buffer safety checks.
- **Modular Directory Architecture**: Separates the core networking engine, HTTP state parsing, and application routing from configuration management and utility modules.
- **External Configuration Engine**: Dynamically loads runtime settings from a `lith.conf` configuration file without requiring recompilation.
- **Dynamic MIME Type Resolution**: Automatically maps file extensions to their corresponding IANA media types (`.html`, `.css`, `.js`, `.json`, `.png`, etc.) for proper browser rendering.
- **Unified Security & Path Filters**: Protects against directory traversal attacks by detecting and rejecting malicious relative path patterns such as `..`.
- **Graceful Port Guard**: Detects socket binding conflicts during startup and exits gracefully with informative logs.
- **Unified Error Canvas Layouts**: Serves clean, dark-themed HTML error pages for common HTTP status codes (`400`, `403`, `404`, `408`, and `500`).
- **Zero Dependencies**: Built exclusively on standard operating system APIs, ensuring an ultra-minimal binary footprint.

---

## Quick Start

### 1. Build Compilation

LITH ships with a highly robust, cross-platform `Makefile` optimized for GNU Make. It automatically handles header dependency generation (`-MMD -MP`) and natively adapts to both Linux/macOS shells and Windows environments (like `w64devkit`).

To compile the project from scratch:

```bash
make clean
make
```

### 2. Configuration (`lith.conf`)

LITH loads its runtime configuration from a file named `lith.conf` located in the application's root directory.

```ini
# Network listening port
PORT=8090

# Directory containing static web assets
PUBLIC_DIR=public
```

### 3. Deployment

Place your web assets inside the configured `public/` directory.

Start the server:

```bash
bin/lith
```

Optionally, override the configured port at runtime:

```bash
bin/lith 8005
```

Open your browser and navigate to:

```text
http://localhost:8005
```

---

## Repository Architecture

```text
.
├── bin/                 # Output directory for compiled executable binaries
├── include/             # Header files, macros, and cross-platform abstractions
│   ├── http_parser.h    # HTTP Request structures, MIME mapping, and error pages
│   ├── http_router.h    # Application routing and static file rendering logic
│   ├── logger.h         # Centralized thread-safe logging interface
│   └── server.h         # Main socket listener and configuration structures
├── public/              # Static web root (HTML, CSS, JS, Assets)
├── src/
│   ├── server/          # Configuration parser and server utility modules
│   │   ├── config.c     # Runtime initialization logic (lith.conf reader)
│   │   └── utils.c      # Multiplatform OS abstractions and file system safety
│   ├── http_parser.c    # Case-insensitive protocol parser
│   ├── http_router.c    # Request handler, route dispatcher, and security validation
│   ├── logger.c         # Formatted terminal output subsystem
│   ├── main.c           # Application bootstrap and runtime arguments handler
│   ├── server.c         # TCP socket initialization (bind, listen, accept)
│   └── server_worker.c  # Persistent worker threads loop (Keep-Alive implementation)
├── lith.conf            # Runtime configuration file
└── Makefile             # Universal, silent multi-platform automated build script
```

---

## Technical Specifications

<table>
    <thead>
        <tr>
            <th>Parameter</th>
            <th>Default Value</th>
            <th>Description</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td><code>LITH_VERSION</code></td>
            <td><code>1.0.6</code></td>
            <td>Current stable release version</td>
        </tr>
        <tr>
            <td><code>BUFFER_SIZE</code></td>
            <td><code>4096 bytes</code></td>
            <td>Standard socket read/write block buffer size</td>
        </tr>
        <tr>
            <td>Maximum Buffer</td>
            <td><code>16 KB</code></td>
            <td>Maximum cumulative buffer space for HTTP headers and POST bodies</td>
        </tr>
        <tr>
            <td><code>BACKLOG</code></td>
            <td><code>10</code></td>
            <td>Maximum number of pending client connections in the listen queue</td>
        </tr>
        <tr>
            <td>Keep-Alive Timeout</td>
            <td><code>3 seconds</code></td>
            <td><code>SO_RCVTIMEO</code> window before cleanly recycling a persistent socket</td>
        </tr>
        <tr>
            <td><code>MAX_KEEP_ALIVE_REQUESTS</code></td>
            <td><code>100</code></td>
            <td>Maximum requests processed per single client TCP session</td>
        </tr>
    </tbody>
</table>

## License

This software is distributed under the terms of the [FomaDev Public License (FPL)](LICENSE). Under this license, personal and educational use of compiled binaries is free, including within corporate environments. Commercial republication, direct source-code integration into competitive proprietary tools, or maintaining standalone independent forks is strictly restricted without explicit commercial authorization. For complete licensing terms, consult the accompanying [LICENSE](LICENSE) file.