// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Cortex Forge Contributors
//
// metrics_collector.hpp - Metrics collection for inference and accelerators
//
/// @brief Collects and aggregates inference and accelerator metrics.

#pragma once

#include <cstdint>
#include <mutex>
#include <vector>
#include <chrono>

namespace cortexforge {

/// @brief Collects and aggregates inference latency and throughput metrics.
///
/// Maintains a sliding window of latency samples for percentile calculations.
/// Thread-safe for concurrent access from multiple inference threads.
class MetricsCollector {
public:
    MetricsCollector();

    /// @brief Record an inference completion.
    /// @param latency_us Inference latency in microseconds.
    void RecordInference(uint64_t latency_us);

    /// @brief Get the total number of inferences recorded.
    uint64_t TotalInferences() const;

    /// @brief Get the average inference latency in microseconds.
    double AvgLatencyUs() const;

    /// @brief Get the P99 latency in microseconds.
    double P99LatencyUs() const;

    /// @brief Get the current inferences-per-second rate.
    uint64_t InferencesPerSecond() const;

    /// @brief Reset all metrics.
    void Reset();

private:
    mutable std::mutex mutex_;
    uint64_t total_inferences_{0};
    double sum_latency_us_{0.0};
    std::vector<uint64_t> recent_latencies_;
    std::chrono::steady_clock::time_point window_start_;
    uint64_t window_count_{0};

    static constexpr size_t kMaxSamples = 10000;
};

} // namespace cortexforge
