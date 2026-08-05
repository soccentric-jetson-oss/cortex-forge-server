// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Cortex Forge Contributors
//
// service_impl.hpp - gRPC service implementation for Cortex Forge
//
/// @brief Implements the CortexForge gRPC service defined in cortex_forge.proto.

#pragma once

#include <cortex_forge.grpc.pb.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

namespace cortexforge {

/// @brief Implements the CortexForge gRPC service.
///
/// Handles model loading/unloading, inference requests, and metrics
/// collection. Thread-safe: uses internal mutex for model registry.
class CortexForgeServiceImpl final : public CortexForge::Service {
public:
    CortexForgeServiceImpl();

    // Model management
    grpc::Status LoadModel(grpc::ServerContext* context,
                           const LoadModelRequest* request,
                           LoadModelResponse* response) override;

    grpc::Status UnloadModel(grpc::ServerContext* context,
                             const UnloadModelRequest* request,
                             UnloadModelResponse* response) override;

    grpc::Status ListModels(grpc::ServerContext* context,
                            const ListModelsRequest* request,
                            ListModelsResponse* response) override;

    grpc::Status GetModelInfo(grpc::ServerContext* context,
                              const GetModelInfoRequest* request,
                              GetModelInfoResponse* response) override;

    // Inference
    grpc::Status Infer(grpc::ServerContext* context,
                       const InferRequest* request,
                       InferResponse* response) override;

    grpc::Status InferStream(grpc::ServerContext* context,
                             const InferRequest* request,
                             grpc::ServerWriter<InferResponse>* writer) override;

    // Monitoring
    grpc::Status GetMetrics(grpc::ServerContext* context,
                            const GetMetricsRequest* request,
                            GetMetricsResponse* response) override;

    grpc::Status WatchMetrics(grpc::ServerContext* context,
                              const GetMetricsRequest* request,
                              grpc::ServerWriter<MetricsSnapshot>* writer) override;

    // Health
    grpc::Status HealthCheck(grpc::ServerContext* context,
                             const HealthCheckRequest* request,
                             HealthCheckResponse* response) override;

private:
    std::mutex mutex_;
    std::unordered_map<std::string, ModelInfo> models_;
    uint64_t start_time_us_;
    uint64_t total_inferences_{0};
    uint64_t next_model_id_{1};

    std::string GenerateModelId();
    uint64_t NowUs() const;
};

} // namespace cortexforge
