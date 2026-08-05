# Cortex Forge Server {#mainpage}

## Overview

Cortex Forge Server is a C++ gRPC service that exposes the NVIDIA Jetson AGX Orin's
ML accelerators (NVDLA v2.0, PVA v2.0, GPU) over a network API. It provides model
loading, inference execution, and real-time accelerator monitoring.

## Architecture

```
┌──────────────┐     gRPC      ┌──────────────────────────────────┐
│   GUI/Client │ ◄──────────► │      Cortex Forge Server         │
│  (Python)    │    :50051    │                                  │
└──────────────┘              │  ┌──────────┐  ┌──────────────┐  │
                              │  │ Service  │  │    Model     │  │
                              │  │  Impl    │──│   Registry   │  │
                              │  └──────────┘  └──────┬───────┘  │
                              │         │              │          │
                              │  ┌──────┴──────┐  ┌───┴────────┐ │
                              │  │ Accelerator │  │ Inference  │ │
                              │  │  Manager    │  │   Engine   │ │
                              │  └──────┬──────┘  └──────┬─────┘ │
                              │         │                 │       │
                              │  ┌──────┴─────────────────┴─────┐ │
                              │  │     Metrics Collector        │ │
                              │  └────────────────────────────┘ │
                              └──────────────────────────────────┘
                                        │
                              ┌─────────┴──────────┐
                              │  cortex-forge-driver│
                              │  (/dev/cortex-forge*)│
                              └────────────────────┘
```

## Dependencies

- gRPC >= 1.50
- Protobuf >= 3.21
- CMake >= 3.20
- Ninja build system
- fmt library
- Catch2 (for tests)

## Quick Start

```bash
# Build
make all

# Run the server
make run

# Run tests
make test
```

## gRPC Service

See `proto/cortex_forge.proto` for the full service definition.

### Key RPCs

| RPC | Description |
|-----|-------------|
| LoadModel | Load a model from file |
| UnloadModel | Unload a loaded model |
| ListModels | List all loaded models |
| Infer | Run synchronous inference |
| InferStream | Run streaming inference |
| GetMetrics | Get current accelerator metrics |
| WatchMetrics | Stream accelerator metrics |
| HealthCheck | Server health probe |
