# Dependencies

## Upstream Dependencies

This server depends on the **cortex-forge-driver** kernel module for:
- `/dev/cortex-forge*` char device access
- Accelerator task submission via ioctl
- Accelerator status via sysfs

## Build Dependencies

- CMake >= 3.20
- Ninja build system
- C++17 compiler (GCC >= 9, Clang >= 10)
- gRPC >= 1.50 (including grpc_cpp_plugin)
- Protobuf >= 3.21 (including protoc)
- fmt library
- Catch2 (auto-fetched by CMake for tests)
- Doxygen (optional, for docs)

## Runtime Dependencies

- Linux (x86_64 for dev, aarch64 for Jetson deployment)
- cortex-forge-driver kernel module loaded
- NVIDIA Jetson AGX Orin (for hardware acceleration)
