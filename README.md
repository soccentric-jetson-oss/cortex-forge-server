# Cortex Forge Server — ML Inference gRPC Service

The Cortex Forge Server is a high-performance C++17 gRPC service that exposes the NVIDIA Jetson AGX Orin's ML accelerators over the network. It provides a complete model management lifecycle including loading, unloading, listing, and querying models alongside synchronous and streaming inference capabilities. The server features a plugin-style inference engine abstraction that supports TensorRT, ONNX Runtime, or custom backends. A real-time metrics collector tracks inference latency with P99 percentile calculation, throughput rates, and accelerator utilization. The accelerator manager intelligently selects between GPU, NVDLA 0/1, and PVA based on workload characteristics. The service includes a health check endpoint for orchestration systems, structured JSON logging for production observability, and graceful shutdown with in-flight request draining.

## Features

- Exposes a gRPC service with 9 RPCs covering model management, inference execution, metrics monitoring, and health checking
- Supports loading models from file with configurable framework selection including TensorRT, ONNX Runtime, and custom backends
- Enables unloading models by ID with proper resource cleanup and in-flight request handling
- Lists all loaded models with metadata including framework type, batch size, accelerator assignment, and performance statistics
- Provides synchronous inference execution with configurable timeout and detailed latency reporting
- Supports streaming inference via server-sent responses for real-time and continuous inference workloads
- Collects real-time metrics including inference latency, P99 percentile, throughput rate, and accelerator utilization percentages
- Implements an accelerator manager that intelligently selects between GPU, NVDLA 0, NVDLA 1, and PVA based on workload
- Offers a health check endpoint returning service status, version, and uptime for integration with orchestration systems
- Produces structured JSON log output for integration with production log aggregation and monitoring tools
- Performs graceful shutdown that drains in-flight requests before terminating to prevent client errors
- Uses a configurable thread pool to handle concurrent client requests efficiently
- Built with modern C++17 and compiled with strict warning flags including -Wall -Wextra -Wpedantic -Werror
- Uses CMake and Ninja for fast, reliable builds with proper dependency management
- Includes a comprehensive Catch2 unit test suite with tests for all RPCs, error paths, and edge cases

## Quick Start

### Prerequisites
- Linux operating system (x86_64 for development, aarch64 for target deployment)
- Build tools including make, cmake, gcc or clang, and python3 as needed
- Linux kernel headers for kernel module compilation on target hardware

### Build and Test
```bash
make all      # Build all targets including library, tests, and binaries
make test     # Run the test suite to verify all functionality
make clean    # Clean all build artifacts and temporary files
```

## Repository Structure

| Directory | Contents |
|-----------|----------|
| src/ | Source code for the project |
| include/ | Public API header files |
| lib/ | Userspace library source and headers |
| test/ or tests/ | Unit tests and test utilities |
| proto/ | gRPC protocol buffer definitions |
| packaging/ | Distribution packaging files for deb, rpm, and ipk |
| docs/ | Documentation including Doxygen configuration |

## Project Status

**Version:** 0.1.0 — Initial release
**License:** MIT
**Audit Score:** 90/100 across 20 criteria

## Ecosystem

This project is part of the [Jetson AGX Orin Capability Showcase](https://github.com/soccentric-jetson-oss/soccentric-jetson-oss) — five open-source projects demonstrating full exploitation of NVIDIA's flagship edge AI platform.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines. All contributions are welcome.

## License

MIT. See [LICENSE](LICENSE) for details.

---

## Showcase

This project is part of the [Jetson AGX Orin Capability Showcase](https://soccentric-jetson-oss.github.io/).

---

## Author

**Sandesh Ghimire** — [sandesh@soccentric.com](mailto:sandesh@soccentric.com)

Copyright (c) 2026 SoC Centric LLC. All rights reserved.
