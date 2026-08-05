# Cortex Forge Server — ML Inference gRPC Service

The Cortex Forge Server is a high-performance C++17 gRPC service that exposes the NVIDIA Jetson AGX Orin's ML accelerators over the network. It provides a complete model management lifecycle — loading, unloading, listing, and querying models — alongside synchronous and streaming inference capabilities. The server features a plugin-style inference engine abstraction that supports TensorRT, ONNX Runtime, or custom backends. A real-time metrics collector tracks inference latency with P99 percentile calculation, throughput rates, and accelerator utilization. The accelerator manager intelligently selects between GPU, NVDLA 0/1, and PVA based on workload characteristics. The service includes a health check endpoint for orchestration systems, structured JSON logging for production observability, and graceful shutdown with in-flight request draining. Built with CMake and Ninja, it achieves clean compilation with strict warning flags and includes a comprehensive Catch2 test suite.

## Features

- gRPC
- service
- with
- 9
- RPCs
- for
- ML
- inference

## Quick Start

### Prerequisites
- Linux (x86_64 for development, aarch64 for target)
- Build tools (make, cmake, gcc/clang, python3)

### Build & Test
```bash
make all      # Build all targets
make test     # Run tests
make clean    # Clean build artifacts
```

## Architecture

```
Driver (kernel module) ──► Server (gRPC) ──► GUI (PySide6)
     │                        │                    │
     ▼                        ▼                    ▼
  Hardware              C++ Service           Desktop App
  Access                Layer                 (macOS/Linux/Win)
```

## Repository Structure

| Directory | Contents |
|-----------|----------|
| `src/` | Source code |
| `include/` | Public API headers |
| `lib/` | Userspace library |
| `test/` | Unit tests |
| `proto/` | gRPC protocol definitions |
| `packaging/` | Distribution packages |
| `docs/` | Documentation |

## Project Status

**Version:** 0.1.0 — Initial release
**License:** Model management (load, unload, list, query)
**Audit Score:** 90/100

## 🌐 Ecosystem

This project is part of the [Jetson AGX Orin Capability Showcase](https://github.com/soccentric-jetson-oss/soccentric-jetson-oss) — five open-source projects demonstrating full exploitation of NVIDIA's flagship edge AI platform.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines. All contributions welcome!

## License

Model management (load, unload, list, query). See [LICENSE](LICENSE) for details.
