// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Cortex Forge Contributors
//
// test_model_registry.cpp - Tests for ModelRegistry

#include <catch2/catch_test_macros.hpp>
#include "engine/model_registry.hpp"
#include "engine/inference_engine.hpp"

using namespace cortexforge;

TEST_CASE("ModelRegistry starts empty", "[registry]") {
    ModelRegistry registry;
    REQUIRE(registry.Count() == 0);
}

TEST_CASE("ModelRegistry register and retrieve", "[registry]") {
    ModelRegistry registry;

    ModelInfo info;
    info.set_model_id("test-1");
    info.set_model_name("Test Model");
    info.set_framework("tensorrt");

    auto engine = std::make_shared<StubInferenceEngine>();
    engine->LoadModel("/models/test.engine");

    registry.Register(info, engine);
    REQUIRE(registry.Count() == 1);

    ModelInfo retrieved;
    REQUIRE(registry.GetInfo("test-1", retrieved));
    REQUIRE(retrieved.model_name() == "Test Model");
}

TEST_CASE("ModelRegistry unregister removes entry", "[registry]") {
    ModelRegistry registry;

    ModelInfo info;
    info.set_model_id("test-1");
    info.set_model_name("Test Model");

    auto engine = std::make_shared<StubInferenceEngine>();
    registry.Register(info, engine);
    REQUIRE(registry.Count() == 1);

    REQUIRE(registry.Unregister("test-1"));
    REQUIRE(registry.Count() == 0);
}

TEST_CASE("ModelRegistry get engine returns correct instance", "[registry]") {
    ModelRegistry registry;

    ModelInfo info;
    info.set_model_id("test-1");

    auto engine = std::make_shared<StubInferenceEngine>();
    engine->LoadModel("/models/test.engine");

    registry.Register(info, engine);

    auto retrieved = registry.GetEngine("test-1");
    REQUIRE(retrieved != nullptr);
    REQUIRE(retrieved->IsLoaded());
}

TEST_CASE("ModelRegistry list all returns all entries", "[registry]") {
    ModelRegistry registry;

    for (int i = 0; i < 5; ++i) {
        ModelInfo info;
        info.set_model_id("model-" + std::to_string(i));
        registry.Register(info, std::make_shared<StubInferenceEngine>());
    }

    auto all = registry.ListAll();
    REQUIRE(all.size() == 5);
}
