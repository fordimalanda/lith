# LITH - Light HTTP Server

LITH is a minimalist, high-performance, multi-threaded HTTP server implemented in standard C11. Engineered to be exceptionally lightweight, modular, and portable, it provides native support for Windows (via MinGW-w64 and Winsock2), macOS, and Linux systems. It is designed with robust network lifecycle management, native multi-threading, and zero external software dependencies.

## Key Features

- **Multi-threaded Architecture**: Utilizes POSIX threads (`pthreads`) to handle multiple simultaneous client connections efficiently, preventing the core network listener from blocking.
- **I/O Engine (v1.0.5 POST Support)**: Features advanced network stream processing to fully parse incoming HTTP `POST` requests, including chunked or fragmented payloads using precise `Content-Length` validation.
- **Modular Directory Architecture**: Separates the core networking engine from configuration management and server utility modules.
- **External Configuration Engine**: Dynamically loads runtime settings from a `lith.conf` configuration file without requiring recompilation.
- **Dynamic MIME Type Resolution**: Automatically maps file extensions to their corresponding IANA media types for proper browser rendering.
- **Unified Security & Path Filters**: Protects against directory traversal attacks by detecting and rejecting malicious relative path patterns such as `..`.
- **Graceful Port Guard**: Detects socket binding conflicts during startup and exits gracefully with informative logs.
- **Unified Error Canvas Layouts**: Serves clean, dark-themed HTML error pages for common HTTP status codes (`400`, `403`, `404`, and `500`).
- **Zero Dependencies**: Built exclusively on standard operating system APIs, ensuring a minimal binary footprint.

---

## Quick Start

### 1. Configuration (`lith.conf`)

LITH loads its runtime configuration from a file named `lith.conf` located in the application's root directory.

```ini
# Network listening port
PORT=8090

# Directory containing static web assets
PUBLIC_DIR=public
```

### 2. Deployment

Place your web assets (`.html`, `.css`, `.js`, `.png`, `.jpg`, `.svg`, `.json`, etc.) inside the configured `public/` directory.

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
├── bin/              # Output directory for compiled executable binaries
├── include/          # Header files, macros, and cross-platform abstractions
├── public/           # Static web root
├── src/
│   ├── server/       # Configuration parser and server utilities
│   └── ...           # Core HTTP server implementation
├── lith.conf         # Runtime configuration
└── Makefile          # Cross-platform GNU Make build script
```

---

## Technical Specifications

| Parameter | Default Value | Description |
|-----------|--------------:|-------------|
| `LITH_VERSION` | `1.0.5` | Current stable release version |
| `BUFFER_SIZE` | `4096 bytes` | Standard socket read/write buffer size |
| `BACKLOG` | `10` | Maximum number of pending client connections |
| `Maximum Buffer` | `16 KB` | Maximum buffer size used for HTTP header and body parsing |

## License

This software is distributed under the terms of the [FomaDev Public License (FPL)](LICENSE). Under this license, personal and educational use of compiled binaries is free, including within corporate environments. Commercial republication, direct source-code integration into competitive proprietary tools, or maintaining standalone independent forks is strictly restricted without explicit commercial authorization. For complete licensing terms, consult the accompanying [LICENSE](LICENSE) file.