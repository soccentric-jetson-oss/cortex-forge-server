// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// accelerator_manager.cpp - Accelerator manager implementation

#include "accelerator/accelerator_manager.hpp"
#include <chrono>

namespace cortexforge
{

AcceleratorManager::AcceleratorManager()
    : rng_(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()))
{
}

AcceleratorMetrics AcceleratorManager::GetMetrics() const
{
    AcceleratorMetrics metrics;
    // Simulated metrics (in production, read from driver via sysfs/ioctl)
    metrics.gpu_util_percent = 45.0 + static_cast<double>(rng_() % 3000) / 100.0;
    metrics.dla0_util_percent = 30.0 + static_cast<double>(rng_() % 4000) / 100.0;
    metrics.dla1_util_percent = 20.0 + static_cast<double>(rng_() % 3000) / 100.0;
    metrics.pva_util_percent = 10.0 + static_cast<double>(rng_() % 2000) / 100.0;
    metrics.gpu_mem_total_mb = 32768;
    metrics.gpu_mem_used_mb = 4096 + static_cast<uint64_t>(rng_() % 8192);
    return metrics;
}

AcceleratorType AcceleratorManager::SelectAccelerator(AcceleratorType preferred) const
{
    if (preferred != AcceleratorType::AUTO)
        return preferred;

    // Simple auto-selection: prefer DLA for inference, GPU for training
    // In production, this would consider model size, current load, etc.
    return AcceleratorType::DLA0;
}

bool AcceleratorManager::IsAcceleratorAvailable(AcceleratorType /*type*/) const
{
    // All accelerators are available on Jetson AGX Orin
    return true;
}

} // namespace cortexforge
