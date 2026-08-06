// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// test_server.cpp - Tests for the gRPC server

#include "server/service_impl.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace cortexforge;

TEST_CASE("HealthCheck returns SERVING", "[server]")
{
    CortexForgeServiceImpl service;
    grpc::ServerContext ctx;
    HealthCheckRequest req;
    HealthCheckResponse resp;

    auto status = service.HealthCheck(&ctx, &req, &resp);
    REQUIRE(status.ok());
    REQUIRE(resp.status() == HealthCheckResponse::SERVING);
    REQUIRE(resp.version() == "0.1.0");
    REQUIRE(resp.uptime_us() > 0);
}

TEST_CASE("LoadModel creates a model entry", "[server]")
{
    CortexForgeServiceImpl service;
    grpc::ServerContext ctx;
    LoadModelRequest req;
    LoadModelResponse resp;

    req.set_model_path("/models/resnet50.engine");
    req.set_model_name("resnet50");
    req.set_framework("tensorrt");

    auto status = service.LoadModel(&ctx, &req, &resp);
    REQUIRE(status.ok());
    REQUIRE(resp.success());
    REQUIRE_FALSE(resp.model_id().empty());
    REQUIRE(resp.model_name() == "resnet50");
}

TEST_CASE("ListModels returns loaded models", "[server]")
{
    CortexForgeServiceImpl service;
    grpc::ServerContext ctx;

    // Load a model first
    LoadModelRequest load_req;
    LoadModelResponse load_resp;
    load_req.set_model_path("/models/test.engine");
    service.LoadModel(&ctx, &load_req, &load_resp);

    // List models
    ListModelsRequest list_req;
    ListModelsResponse list_resp;
    auto status = service.ListModels(&ctx, &list_req, &list_resp);
    REQUIRE(status.ok());
    REQUIRE(list_resp.models_size() > 0);
}

TEST_CASE("UnloadModel removes a model", "[server]")
{
    CortexForgeServiceImpl service;
    grpc::ServerContext ctx;

    // Load a model
    LoadModelRequest load_req;
    LoadModelResponse load_resp;
    load_req.set_model_path("/models/test.engine");
    service.LoadModel(&ctx, &load_req, &load_resp);

    // Unload it
    UnloadModelRequest unload_req;
    UnloadModelResponse unload_resp;
    unload_req.set_model_id(load_resp.model_id());

    auto status = service.UnloadModel(&ctx, &unload_req, &unload_resp);
    REQUIRE(status.ok());
    REQUIRE(unload_resp.success());
}

TEST_CASE("Infer returns results for loaded model", "[server]")
{
    CortexForgeServiceImpl service;
    grpc::ServerContext ctx;

    // Load a model
    LoadModelRequest load_req;
    LoadModelResponse load_resp;
    load_req.set_model_path("/models/test.engine");
    service.LoadModel(&ctx, &load_req, &load_resp);

    // Run inference
    InferRequest infer_req;
    InferResponse infer_resp;
    infer_req.set_model_id(load_resp.model_id());
    infer_req.set_input_data("test_input");

    auto status = service.Infer(&ctx, &infer_req, &infer_resp);
    REQUIRE(status.ok());
    REQUIRE(infer_resp.success());
    REQUIRE(infer_resp.latency_us() > 0);
}

TEST_CASE("Infer on unknown model returns NOT_FOUND", "[server]")
{
    CortexForgeServiceImpl service;
    grpc::ServerContext ctx;

    InferRequest infer_req;
    InferResponse infer_resp;
    infer_req.set_model_id("nonexistent-model");

    auto status = service.Infer(&ctx, &infer_req, &infer_resp);
    REQUIRE(status.error_code() == grpc::StatusCode::NOT_FOUND);
    REQUIRE_FALSE(infer_resp.success());
}

TEST_CASE("GetMetrics returns valid data", "[server]")
{
    CortexForgeServiceImpl service;
    grpc::ServerContext ctx;

    GetMetricsRequest req;
    GetMetricsResponse resp;

    auto status = service.GetMetrics(&ctx, &req, &resp);
    REQUIRE(status.ok());
    REQUIRE(resp.has_current());
    REQUIRE(resp.current().gpu_mem_total_mb() > 0);
}
