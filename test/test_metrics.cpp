// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Cortex Forge Contributors
//
// test_metrics.cpp - Tests for MetricsCollector

#include <catch2/catch_test_macros.hpp>
#include "monitor/metrics_collector.hpp"
#include <thread>

using namespace cortexforge;

TEST_CASE("MetricsCollector starts at zero", "[metrics]") {
    MetricsCollector collector;
    REQUIRE(collector.TotalInferences() == 0);
    REQUIRE(collector.AvgLatencyUs() == 0.0);
    REQUIRE(collector.P99LatencyUs() == 0.0);
}

TEST_CASE("MetricsCollector records inferences", "[metrics]") {
    MetricsCollector collector;

    collector.RecordInference(1000);
    collector.RecordInference(2000);
    collector.RecordInference(3000);

    REQUIRE(collector.TotalInferences() == 3);
    REQUIRE(collector.AvgLatencyUs() == 2000.0);
}

TEST_CASE("MetricsCollector P99 calculation", "[metrics]") {
    MetricsCollector collector;

    // Record 100 samples with known distribution
    for (int i = 0; i < 100; ++i) {
        collector.RecordInference(static_cast<uint64_t>(1000 + i * 10));
    }

    auto p99 = collector.P99LatencyUs();
    REQUIRE(p99 > 0.0);
    // P99 should be near the 99th percentile
    REQUIRE(p99 >= 1900.0);
}

TEST_CASE("MetricsCollector reset clears data", "[metrics]") {
    MetricsCollector collector;

    collector.RecordInference(1000);
    collector.RecordInference(2000);
    REQUIRE(collector.TotalInferences() == 2);

    collector.Reset();
    REQUIRE(collector.TotalInferences() == 0);
    REQUIRE(collector.AvgLatencyUs() == 0.0);
}

TEST_CASE("MetricsCollector thread safety", "[metrics]") {
    MetricsCollector collector;
    std::vector<std::thread> threads;

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&collector]() {
            for (int i = 0; i < 100; ++i) {
                collector.RecordInference(static_cast<uint64_t>(500 + i));
            }
        });
    }

    for (auto& t : threads) t.join();

    REQUIRE(collector.TotalInferences() == 400);
    REQUIRE(collector.AvgLatencyUs() > 0.0);
}
