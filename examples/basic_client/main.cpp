// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Cortex Forge Contributors
//
// main.cpp - Basic gRPC client example for Cortex Forge Server

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <cortex_forge.grpc.pb.h>

int main(int /*argc*/, char** /*argv*/) {
    std::string server_address = "localhost:50051";

    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    auto stub = cortexforge::CortexForge::NewStub(channel);

    // Health check
    grpc::ClientContext health_ctx;
    cortexforge::HealthCheckRequest health_req;
    cortexforge::HealthCheckResponse health_resp;

    auto status = stub->HealthCheck(&health_ctx, health_req, &health_resp);
    if (status.ok()) {
        std::cout << "Server is " << (health_resp.status() == cortexforge::HealthCheckResponse::SERVING ? "SERVING" : "NOT SERVING")
                  << " (v" << health_resp.version() << ")\n";
    } else {
        std::cerr << "Health check failed: " << status.error_message() << "\n";
        return 1;
    }

    // Load a model
    grpc::ClientContext load_ctx;
    cortexforge::LoadModelRequest load_req;
    cortexforge::LoadModelResponse load_resp;

    load_req.set_model_path("/models/resnet50.engine");
    load_req.set_model_name("resnet50");
    load_req.set_framework("tensorrt");

    status = stub->LoadModel(&load_ctx, load_req, &load_resp);
    if (status.ok() && load_resp.success()) {
        std::cout << "Loaded model: " << load_resp.model_id() << " (" << load_resp.model_name() << ")\n";
    } else {
        std::cerr << "Failed to load model: " << load_resp.error_message() << "\n";
        return 1;
    }

    // Run inference
    grpc::ClientContext infer_ctx;
    cortexforge::InferRequest infer_req;
    cortexforge::InferResponse infer_resp;

    infer_req.set_model_id(load_resp.model_id());
    infer_req.set_input_data("sample_input_data");

    status = stub->Infer(&infer_ctx, infer_req, &infer_resp);
    if (status.ok() && infer_resp.success()) {
        std::cout << "Inference complete: " << infer_resp.latency_us() << " us\n";
    } else {
        std::cerr << "Inference failed: " << infer_resp.error_message() << "\n";
        return 1;
    }

    // Get metrics
    grpc::ClientContext metrics_ctx;
    cortexforge::GetMetricsRequest metrics_req;
    cortexforge::GetMetricsResponse metrics_resp;

    status = stub->GetMetrics(&metrics_ctx, metrics_req, &metrics_resp);
    if (status.ok()) {
        auto& m = metrics_resp.current();
        std::cout << "GPU util: " << m.gpu_util_percent() << "%\n";
        std::cout << "DLA0 util: " << m.dla0_util_percent() << "%\n";
        std::cout << "Total inferences: " << m.total_inferences() << "\n";
    }

    std::cout << "Done.\n";
    return 0;
}
