// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// accelerator_iface.hpp - Accelerator interface abstraction
//
/// @brief Defines the interface for accelerator backends (GPU, DLA, PVA).

#pragma once

#include <cstdint>
#include <string>

namespace cortexforge
{

/// @brief Accelerator type identifiers.
enum class AcceleratorType
{
    GPU,
    DLA0,
    DLA1,
    PVA,
    AUTO
};

/// @brief Convert accelerator type to string.
inline std::string AcceleratorTypeToString(AcceleratorType type)
{
    switch (type)
    {
        case AcceleratorType::GPU:
            return "gpu";
        case AcceleratorType::DLA0:
            return "dla0";
        case AcceleratorType::DLA1:
            return "dla1";
        case AcceleratorType::PVA:
            return "pva";
        case AcceleratorType::AUTO:
            return "auto";
    }
    return "unknown";
}

/// @brief Convert string to accelerator type.
inline AcceleratorType StringToAcceleratorType(const std::string& str)
{
    if (str == "gpu")
        return AcceleratorType::GPU;
    if (str == "dla0")
        return AcceleratorType::DLA0;
    if (str == "dla1")
        return AcceleratorType::DLA1;
    if (str == "pva")
        return AcceleratorType::PVA;
    return AcceleratorType::AUTO;
}

/// @brief Accelerator utilization snapshot.
struct AcceleratorMetrics
{
    double gpu_util_percent{0.0};
    double dla0_util_percent{0.0};
    double dla1_util_percent{0.0};
    double pva_util_percent{0.0};
    uint64_t gpu_mem_total_mb{0};
    uint64_t gpu_mem_used_mb{0};
};

} // namespace cortexforge
