# Cortex Forge Server

C++17 gRPC server for the NVIDIA Jetson AGX Orin ML accelerators. Part of the
[Cortex Forge](https://github.com/soccentric-jetson-oss/cortex-forge) project.

## Architecture

```
┌──────────────┐     gRPC      ┌──────────────────────────────────┐
│   GUI/Client │ ◄──────────► │      Cortex Forge Server       │
│  (Python)    │    :50051    │  ┌──────────┐  ┌────────────┐  │
└──────────────┘              │  │ Service  │  │   Model    │  │
                              │  │  Impl    │──│  Registry  │  │
                              │  └──────────┘  └──────┬─────┘  │
                              │         │              │        │
                              │  ┌──────┴──────┐  ┌───┴──────┐ │
                              │  │ Accelerator │  │Inference │ │
                              │  │  Manager    │  │  Engine  │ │
                              │  └──────┬──────┘  └─────┬────┘ │
                              │         │                │     │
                              │  ┌──────┴────────────────┴───┐ │
                              │  │   Metrics Collector       │ │
                              │  └───────────────────────────┘ │
                              └────────────────────────────────┘
                                        │
                              ┌─────────┴──────────┐
                              │ cortex-forge-driver │
                              │ (/dev/cortex-forge*)│
                              └────────────────────┘
```

## gRPC Service

| RPC | Description |
|-----|-------------|
| `LoadModel` | Load a model from file |
| `UnloadModel` | Unload a loaded model |
| `ListModels` | List all loaded models |
| `GetModelInfo` | Get details about a specific model |
| `Infer` | Run synchronous inference |
| `InferStream` | Run streaming inference |
| `GetMetrics` | Get current accelerator metrics |
| `WatchMetrics` | Stream real-time accelerator metrics |
| `HealthCheck` | Server health probe |

## Quick Start

```bash
# Prerequisites: CMake, Ninja, gRPC, Protobuf, fmt
# Build
make all

# Run the server
make run

# Run tests
make test

# Run the example client (in another terminal)
./build/examples/basic_client/cortex-forge-client
```

## Dependencies

- gRPC >= 1.50
- Protobuf >= 3.21
- CMake >= 3.20
- Ninja
- fmt library
- Catch2 (dev, fetched automatically)

## Interface with Driver

This server communicates with the `cortex-forge-driver` kernel module via:
- `/dev/cortex-forge*` char devices for accelerator task submission
- `ioctl()` interface for DLA/PVA task management
- `sysfs` for accelerator status monitoring

## License

MIT

## 🌐 Ecosystem Website
Visit the [Jetson AGX Orin Capability Showcase](https://github.com/soccentric-jetson-oss/soccentric-jetson-oss) for an overview of all projects.
