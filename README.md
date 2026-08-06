# Cortex Forge Server — ML Inference Platform for NVIDIA Jetson

We've been building on NVIDIA Jetson since the TX1 in 2015 — over a dozen projects across TX1, TX2, Xavier, Orin, and Thor. This is our attempt to open-source some of the frameworks and building blocks we use internally to kickstart every new project. Sharing what we know helps us grow too. Hope it does the same for you.

## Overview

Cortex Forge Server is a generic ML-serving platform for the NVIDIA Jetson AGX Orin. It exposes model-agnostic gRPC primitives that any downstream client application can build on:

- **Model lifecycle**: `LoadModel`, `UnloadModel`, `ListModels`, `GetModelInfo`, `HotSwapModel`
- **Inference**: `Infer`, `InferStream`, `BatchInfer`
- **Accelerator management**: `GetAcceleratorStatus`, `AssignAccelerator`
- **Telemetry**: `GetMetrics`, `WatchMetrics`, `GetPowerTelemetry`, `GetThermalTelemetry`
- **Fault recovery**: `HealthCheck`, `GetFaultLog`, `ResetAccelerator`
- **System**: `GetSystemInfo`, `ShutdownServer`

## Architecture

```
┌─────────────┐     ┌──────────────────┐     ┌─────────────────┐
│  App Suite   │────▶│  Cortex Forge    │────▶│  Accelerators   │
│  (10 apps)   │gRPC │  Server          │     │  GPU/DLA0/DLA1  │
└─────────────┘     └──────────────────┘     │  /PVA            │
                                             └─────────────────┘
```

## Directory Structure

```
CortexForge/
├── core/
│   ├── cortex-forge-driver/       # Linux kernel module
│   ├── cortex-forge-server/       # This project — gRPC server
│   └── cortex-forge-gui/          # Operator console
│
└── apps/                          # 10 standalone client apps
    ├── cortex-forge-app-object-tracker/
    ├── cortex-forge-app-defect-inspector/
    ├── cortex-forge-app-plate-reader/
    ├── cortex-forge-app-acoustic-sentinel/
    ├── cortex-forge-app-voice-command/
    ├── cortex-forge-app-model-gateway/
    ├── cortex-forge-app-model-arena/
    ├── cortex-forge-app-predictive-maintenance/
    ├── cortex-forge-app-quality-grader/
    └── cortex-forge-app-benchmark-suite/
```

## Quick Start

```bash
# Build the server
cd core/cortex-forge-server
mkdir build && cd build
cmake .. -GNinja
ninja

# Run the server
./cortex-forge-server --address 127.0.0.1:50051
```

## gRPC API

| Category | RPCs |
|----------|------|
| Model Lifecycle | `LoadModel`, `UnloadModel`, `ListModels`, `GetModelInfo`, `HotSwapModel` |
| Inference | `Infer`, `InferStream`, `BatchInfer` |
| Accelerator | `GetAcceleratorStatus`, `AssignAccelerator` |
| Telemetry | `GetMetrics`, `WatchMetrics`, `GetPowerTelemetry`, `GetThermalTelemetry` |
| Fault Recovery | `HealthCheck`, `GetFaultLog`, `ResetAccelerator` |
| System | `GetSystemInfo`, `ShutdownServer` |

## Author

**Sandesh Ghimire** — [sandesh@soccentric.com](mailto:sandesh@soccentric.com)

Copyright (c) 2026 SoC Centric LLC. All rights reserved.
