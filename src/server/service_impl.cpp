// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// service_impl.cpp - gRPC service implementation
//
/// @brief Implements all CortexForge gRPC RPCs.

#include "server/service_impl.hpp"
#include <chrono>
#include <random>
#include <thread>

namespace cortexforge
{

CortexForgeServiceImpl::CortexForgeServiceImpl() : start_time_us_(NowUs())
{
}

std::string CortexForgeServiceImpl::GenerateModelId()
{
    return "model-" + std::to_string(next_model_id_++);
}

uint64_t CortexForgeServiceImpl::NowUs() const
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                                     std::chrono::steady_clock::now().time_since_epoch())
                                     .count());
}

// ── Model Management ───────────────────────────────────────────────────────

grpc::Status CortexForgeServiceImpl::LoadModel(grpc::ServerContext* /*context*/,
                                               const LoadModelRequest* request,
                                               LoadModelResponse* response)
{

    auto load_start = NowUs();

    std::lock_guard<std::mutex> lock(mutex_);

    auto model_id = GenerateModelId();
    auto model_name = request->model_name().empty() ? "model-" + model_id : request->model_name();

    ModelInfo info;
    info.set_model_id(model_id);
    info.set_model_name(model_name);
    info.set_framework(request->framework().empty() ? "tensorrt" : request->framework());
    info.set_batch_size(request->batch_size() > 0 ? request->batch_size() : 1);
    info.set_accelerator(request->accelerator().empty() ? "auto" : request->accelerator());
    info.set_load_time_us(NowUs() - load_start);
    info.set_total_inferences(0);
    info.set_avg_latency_us(0);
    info.set_loaded(true);

    models_[model_id] = info;

    response->set_model_id(model_id);
    response->set_model_name(model_name);
    response->set_success(true);

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::UnloadModel(grpc::ServerContext* /*context*/,
                                                 const UnloadModelRequest* request,
                                                 UnloadModelResponse* response)
{

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = models_.find(request->model_id());
    if (it == models_.end())
    {
        response->set_success(false);
        response->set_error_message("Model not found: " + request->model_id());
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Model not found");
    }

    models_.erase(it);
    response->set_success(true);
    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::ListModels(grpc::ServerContext* /*context*/,
                                                const ListModelsRequest* /*request*/,
                                                ListModelsResponse* response)
{

    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& [id, info] : models_)
    {
        auto* model = response->add_models();
        *model = info;
    }

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::GetModelInfo(grpc::ServerContext* /*context*/,
                                                  const GetModelInfoRequest* request,
                                                  GetModelInfoResponse* response)
{

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = models_.find(request->model_id());
    if (it == models_.end())
    {
        response->set_found(false);
        return grpc::Status::OK;
    }

    *response->mutable_model() = it->second;
    response->set_found(true);
    return grpc::Status::OK;
}

// ── Inference ──────────────────────────────────────────────────────────────

grpc::Status CortexForgeServiceImpl::Infer(grpc::ServerContext* /*context*/,
                                           const InferRequest* request, InferResponse* response)
{

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = models_.find(request->model_id());
    if (it == models_.end())
    {
        response->set_success(false);
        response->set_error_message("Model not found: " + request->model_id());
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Model not found");
    }

    // Simulate inference latency (in production, this would run on the accelerator)
    auto& model = it->second;
    auto latency = static_cast<uint64_t>(1000 + (rand() % 500)); // 1-1.5ms simulated

    model.set_total_inferences(model.total_inferences() + 1);
    model.set_avg_latency_us(
        (model.avg_latency_us() * static_cast<double>(model.total_inferences() - 1ULL) +
         static_cast<double>(latency)) /
        static_cast<double>(model.total_inferences()));

    total_inferences_++;

    response->set_model_id(request->model_id());
    response->set_output_data("simulated_output_" + request->model_id());
    response->set_latency_us(latency);
    response->set_success(true);

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::InferStream(grpc::ServerContext* context,
                                                 const InferRequest* request,
                                                 grpc::ServerWriter<InferResponse>* writer)
{

    // Stream 10 simulated inference results
    for (int i = 0; i < 10; ++i)
    {
        if (context->IsCancelled())
            break;

        InferResponse response;
        response.set_model_id(request->model_id());
        response.set_output_data("stream_output_" + std::to_string(i));
        response.set_latency_us(500 + static_cast<uint64_t>(rand() % 200));
        response.set_success(true);

        if (!writer->Write(response))
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return grpc::Status::OK;
}

// ── Monitoring ────────────────────────────────────────────────────────────

grpc::Status CortexForgeServiceImpl::GetMetrics(grpc::ServerContext* /*context*/,
                                                const GetMetricsRequest* /*request*/,
                                                GetMetricsResponse* response)
{

    auto* current = response->mutable_current();
    current->set_gpu_util_percent(45.0 + static_cast<double>(rand() % 30));
    current->set_dla0_util_percent(30.0 + static_cast<double>(rand() % 40));
    current->set_dla1_util_percent(20.0 + static_cast<double>(rand() % 30));
    current->set_pva_util_percent(10.0 + static_cast<double>(rand() % 20));
    current->set_gpu_mem_total_mb(32768);
    current->set_gpu_mem_used_mb(4096 + static_cast<uint64_t>(rand() % 4096));
    current->set_total_inferences(total_inferences_);
    current->set_avg_latency_us(1200);
    current->set_p99_latency_us(2500);
    current->set_inferences_per_second(100 + static_cast<uint64_t>(rand() % 200));
    current->set_timestamp_us(NowUs());

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::WatchMetrics(grpc::ServerContext* context,
                                                  const GetMetricsRequest* /*request*/,
                                                  grpc::ServerWriter<MetricsSnapshot>* writer)
{

    // Stream metrics every 500ms
    while (!context->IsCancelled())
    {
        MetricsSnapshot snapshot;
        snapshot.set_gpu_util_percent(45.0 + static_cast<double>(rand() % 30));
        snapshot.set_dla0_util_percent(30.0 + static_cast<double>(rand() % 40));
        snapshot.set_dla1_util_percent(20.0 + static_cast<double>(rand() % 30));
        snapshot.set_pva_util_percent(10.0 + static_cast<double>(rand() % 20));
        snapshot.set_gpu_mem_total_mb(32768);
        snapshot.set_gpu_mem_used_mb(4096 + static_cast<uint64_t>(rand() % 4096));
        snapshot.set_total_inferences(total_inferences_);
        snapshot.set_avg_latency_us(1200);
        snapshot.set_p99_latency_us(2500);
        snapshot.set_inferences_per_second(100 + static_cast<uint64_t>(rand() % 200));
        snapshot.set_timestamp_us(NowUs());

        if (!writer->Write(snapshot))
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return grpc::Status::OK;
}

// ── Health ─────────────────────────────────────────────────────────────────

grpc::Status CortexForgeServiceImpl::HealthCheck(grpc::ServerContext* /*context*/,
                                                 const HealthCheckRequest* /*request*/,
                                                 HealthCheckResponse* response)
{

    response->set_status(HealthCheckResponse::SERVING);
    response->set_version("0.1.0");
    response->set_uptime_us(NowUs() - start_time_us_);

    return grpc::Status::OK;
}

} // namespace cortexforge
