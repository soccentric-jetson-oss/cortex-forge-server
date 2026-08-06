// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// accelerator_manager.hpp - Accelerator manager
//
/// @brief Manages accelerator selection and metrics collection.

#pragma once

#include "accelerator/accelerator_iface.hpp"
#include <memory>
#include <random>

namespace cortexforge {

/// @brief Manages accelerator selection and metrics collection.
///
/// In production, this reads metrics from the cortex-forge-driver
/// via /dev/cortex-forge* ioctls and sysfs. In development, it
/// simulates accelerator metrics.
class AcceleratorManager {
public:
    AcceleratorManager();

    /// @brief Get current accelerator utilization metrics.
    AcceleratorMetrics GetMetrics() const;

    /// @brief Select the best accelerator for a given model.
    /// @param preferred Preferred accelerator type (or AUTO).
    /// @return Selected accelerator type.
    AcceleratorType SelectAccelerator(AcceleratorType preferred) const;

    /// @brief Check if a specific accelerator is available.
    bool IsAcceleratorAvailable(AcceleratorType type) const;

private:
    mutable std::mt19937 rng_;
};

} // namespace cortexforge
