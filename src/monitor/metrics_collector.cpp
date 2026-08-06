// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// metrics_collector.cpp - Metrics collector implementation

#include "monitor/metrics_collector.hpp"
#include <algorithm>
#include <numeric>

namespace cortexforge
{

MetricsCollector::MetricsCollector() : window_start_(std::chrono::steady_clock::now())
{
}

void MetricsCollector::RecordInference(uint64_t latency_us)
{
    std::lock_guard<std::mutex> lock(mutex_);

    total_inferences_++;
    sum_latency_us_ += static_cast<double>(latency_us);
    window_count_++;

    recent_latencies_.push_back(latency_us);
    if (recent_latencies_.size() > kMaxSamples)
    {
        recent_latencies_.erase(recent_latencies_.begin());
    }
}

uint64_t MetricsCollector::TotalInferences() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return total_inferences_;
}

double MetricsCollector::AvgLatencyUs() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (total_inferences_ == 0)
        return 0.0;
    return sum_latency_us_ / static_cast<double>(total_inferences_);
}

double MetricsCollector::P99LatencyUs() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (recent_latencies_.empty())
        return 0.0;

    auto sorted = recent_latencies_;
    std::sort(sorted.begin(), sorted.end());

    size_t idx = static_cast<size_t>(0.99 * static_cast<double>(sorted.size()));
    if (idx >= sorted.size())
        idx = sorted.size() - 1;

    return static_cast<double>(sorted[idx]);
}

uint64_t MetricsCollector::InferencesPerSecond() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();
    auto elapsed_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - window_start_).count());

    if (elapsed_us == 0)
        return 0;

    return (window_count_ * 1000000ULL) / elapsed_us;
}

void MetricsCollector::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    total_inferences_ = 0;
    sum_latency_us_ = 0.0;
    recent_latencies_.clear();
    window_start_ = std::chrono::steady_clock::now();
    window_count_ = 0;
}

} // namespace cortexforge
