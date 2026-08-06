// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// service_impl.cpp - Ultra feature-rich gRPC service implementation
//
/// @brief Implements all CortexForge gRPC RPCs with full model lifecycle,
/// multi-accelerator inference, streaming, telemetry, and fault recovery.

#include "server/service_impl.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <thread>

namespace cortexforge
{

// ══════════════════════════════════════════════════════════════════════════
// Construction / Destruction
// ══════════════════════════════════════════════════════════════════════════

CortexForgeServiceImpl::CortexForgeServiceImpl()
    : start_time_us_(NowUs())
    , rng_(std::random_device{}())
{
    // Initialize simulated accelerators
    AccelState gpu;
    gpu.name = "GPU";
    gpu.type = "gpu";
    gpu.freq_hz = 1300000000;
    gpu.temp_celsius = 45;
    gpu.mem_total = 32768ULL * 1024 * 1024;
    gpu.mem_used = 4096ULL * 1024 * 1024;
    gpu.power_watts = 15.0;
    gpu.status = "online";
    gpu.total_tasks = 0;
    gpu.failed_tasks = 0;

    AccelState dla0;
    dla0.name = "DLA0";
    dla0.type = "dla";
    dla0.freq_hz = 1200000000;
    dla0.temp_celsius = 40;
    dla0.mem_total = 2048ULL * 1024 * 1024;
    dla0.mem_used = 256ULL * 1024 * 1024;
    dla0.power_watts = 5.0;
    dla0.status = "online";
    dla0.total_tasks = 0;
    dla0.failed_tasks = 0;

    AccelState dla1;
    dla1 = dla0;
    dla1.name = "DLA1";

    AccelState pva;
    pva.name = "PVA";
    pva.type = "pva";
    pva.freq_hz = 800000000;
    pva.temp_celsius = 38;
    pva.mem_total = 512ULL * 1024 * 1024;
    pva.mem_used = 64ULL * 1024 * 1024;
    pva.power_watts = 3.0;
    pva.status = "online";
    pva.total_tasks = 0;
    pva.failed_tasks = 0;

    accelerators_["GPU"] = gpu;
    accelerators_["DLA0"] = dla0;
    accelerators_["DLA1"] = dla1;
    accelerators_["PVA"] = pva;

    // Start background metrics updater
    metrics_thread_ = std::thread(&CortexForgeServiceImpl::MetricsUpdateLoop, this);
}

CortexForgeServiceImpl::~CortexForgeServiceImpl()
{
    running_ = false;
    if (metrics_thread_.joinable())
        metrics_thread_.join();
}

// ══════════════════════════════════════════════════════════════════════════
// Background Metrics Update Loop
// ══════════════════════════════════════════════════════════════════════════

void CortexForgeServiceImpl::MetricsUpdateLoop()
{
    while (running_)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Update accelerator metrics with simulated variation
        {
            std::lock_guard<std::mutex> lock(accel_mutex_);
            for (auto& [name, accel] : accelerators_)
            {
                std::lock_guard<std::mutex> rng_lock(rng_mutex_);
                double variation = (std::uniform_real_distribution<double>(-5.0, 5.0))(rng_);
                accel.utilization = std::clamp(accel.utilization + variation, 0.0, 100.0);
                accel.temp_celsius += static_cast<int32_t>(variation * 0.3);
                accel.temp_celsius = std::clamp(accel.temp_celsius, 30, 85);
                accel.power_watts += variation * 0.1;
                accel.power_watts = std::max(0.0, accel.power_watts);

                // Simulate occasional faults
                if (std::uniform_real_distribution<double>(0, 1)(rng_) < 0.001) // 0.1% chance
                {
                    accel.status = "error";
                    RecordFault(name, "thermal_throttle",
                                "Temperature spike detected on " + name,
                                "warning", "");
                }
                else if (accel.status == "error" &&
                         std::uniform_real_distribution<double>(0, 1)(rng_) < 0.1)
                {
                    accel.status = "online";
                }
            }
        }

        // Store metrics snapshot
        {
            std::lock_guard<std::mutex> lock(history_mutex_);
            metrics_history_.push_back(BuildCurrentSnapshot());
            if (metrics_history_.size() > MAX_HISTORY)
                metrics_history_.pop_front();
        }
    }
}

// ══════════════════════════════════════════════════════════════════════════
// Model Lifecycle Management
// ══════════════════════════════════════════════════════════════════════════

grpc::Status CortexForgeServiceImpl::LoadModel(grpc::ServerContext*,
                                               const LoadModelRequest* request,
                                               LoadModelResponse* response)
{
    auto load_start = NowUs();

    auto state = std::make_unique<InternalModelState>();
    auto& meta = state->metadata;

    auto model_id = GenerateModelId();
    meta.set_model_id(model_id);
    meta.set_model_name(request->model_name().empty() ? "model-" + model_id : request->model_name());
    meta.set_framework(request->framework().empty() ? "tensorrt" : request->framework());
    meta.set_batch_size(request->batch_size() > 0 ? request->batch_size() : 1);
    meta.set_model_type(request->model_type().empty() ? "detection" : request->model_type());
    meta.set_input_dtype(request->input_dtype().empty() ? "float32" : request->input_dtype());
    meta.set_input_channels(request->input_channels() > 0 ? request->input_channels() : 3);
    meta.set_input_width(request->input_width() > 0 ? request->input_width() : 640);
    meta.set_input_height(request->input_height() > 0 ? request->input_height() : 480);
    meta.set_input_fps(request->input_fps() > 0 ? request->input_fps() : 30);
    meta.set_loaded(true);
    meta.set_status("active");
    meta.set_load_timestamp_us(load_start);
    meta.set_total_inferences(0);
    meta.set_avg_latency_us(0);
    meta.set_throughput_fps(0);
    meta.set_error_count(0);
    meta.set_accuracy_metric(0.95);

    // Set input shape
    if (request->input_shape_size() > 0)
    {
        for (auto s : request->input_shape())
            meta.add_input_shape(s);
    }
    else
    {
        meta.add_input_shape(meta.input_channels());
        meta.add_input_shape(meta.input_height());
        meta.add_input_shape(meta.input_width());
    }

    // Assign accelerator
    std::string preferred = request->accelerator().empty() ? "auto" : request->accelerator();
    std::string assigned = preferred;

    if (preferred == "auto")
    {
        // Round-robin assignment
        std::lock_guard<std::mutex> lock(accel_mutex_);
        static int rr = 0;
        std::vector<std::string> accels = {"GPU", "DLA0", "DLA1", "PVA"};
        assigned = accels[rr % 4];
        rr++;
    }

    meta.set_accelerator(assigned);

    // Register with accelerator
    {
        std::lock_guard<std::mutex> lock(accel_mutex_);
        if (accelerators_.count(assigned))
        {
            accelerators_[assigned].loaded_models.push_back(model_id);
            accelerators_[assigned].total_tasks++;
        }
    }

    // Simulate load time
    uint64_t load_time = 500000 + (rand() % 1500000); // 0.5-2 seconds
    meta.set_load_time_us(load_time);

    state->load_timestamp_us = load_start;

    // Store model
    {
        std::lock_guard<std::mutex> lock(models_mutex_);
        models_[model_id] = std::move(state);
    }

    response->set_model_id(model_id);
    response->set_model_name(meta.model_name());
    response->set_success(true);
    response->set_load_time_us(load_time);
    response->set_assigned_accelerator(assigned);
    *response->mutable_metadata() = meta;

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::UnloadModel(grpc::ServerContext*,
                                                 const UnloadModelRequest* request,
                                                 UnloadModelResponse* response)
{
    std::lock_guard<std::mutex> lock(models_mutex_);

    auto it = models_.find(request->model_id());
    if (it == models_.end())
    {
        response->set_success(false);
        response->set_error_message("Model not found: " + request->model_id());
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Model not found");
    }

    auto& state = it->second;
    response->set_total_inferences(state->metadata.total_inferences());

    // Remove from accelerator
    {
        std::lock_guard<std::mutex> accel_lock(accel_mutex_);
        for (auto& [name, accel] : accelerators_)
        {
            auto& models = accel.loaded_models;
            models.erase(std::remove(models.begin(), models.end(), request->model_id()), models.end());
        }
    }

    models_.erase(it);
    response->set_success(true);
    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::ListModels(grpc::ServerContext*,
                                                const ListModelsRequest*,
                                                ListModelsResponse* response)
{
    std::lock_guard<std::mutex> lock(models_mutex_);

    uint32_t loaded = 0;
    for (const auto& [id, state] : models_)
    {
        if (state->metadata.loaded())
            loaded++;
        *response->add_models() = state->metadata;
    }

    response->set_total_count(static_cast<uint32_t>(models_.size()));
    response->set_loaded_count(loaded);
    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::GetModelInfo(grpc::ServerContext*,
                                                  const GetModelInfoRequest* request,
                                                  GetModelInfoResponse* response)
{
    std::lock_guard<std::mutex> lock(models_mutex_);

    auto it = models_.find(request->model_id());
    if (it == models_.end())
    {
        response->set_found(false);
        return grpc::Status::OK;
    }

    *response->mutable_model() = it->second->metadata;
    response->set_found(true);
    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::HotSwapModel(grpc::ServerContext*,
                                                  const HotSwapModelRequest* request,
                                                  HotSwapModelResponse* response)
{
    auto swap_start = NowUs();

    std::lock_guard<std::mutex> lock(models_mutex_);

    auto it = models_.find(request->model_id());
    if (it == models_.end())
    {
        response->set_success(false);
        response->set_error_message("Model not found: " + request->model_id());
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Model not found");
    }

    // Simulate hot-swap
    auto& old_state = it->second;
    std::string old_accel = old_state->metadata.accelerator();
    uint64_t old_inferences = old_state->metadata.total_inferences();

    // Create new model state
    auto new_state = std::make_unique<InternalModelState>();
    auto& new_meta = new_state->metadata;
    new_meta = old_state->metadata; // Copy old metadata
    new_meta.set_model_id(GenerateModelId());
    new_meta.set_model_name(request->new_model_path());
    new_meta.set_load_time_us(NowUs() - swap_start);
    new_meta.set_total_inferences(0);
    new_meta.set_avg_latency_us(0);
    new_meta.set_loaded(true);
    new_meta.set_status("active");

    new_state->load_timestamp_us = swap_start;

    // Replace
    std::string new_id = new_meta.model_id();
    models_[new_id] = std::move(new_state);
    models_.erase(it);

    uint64_t swap_time = NowUs() - swap_start;
    response->set_success(true);
    response->set_swap_time_us(swap_time);
    response->set_new_model_id(new_id);

    RecordFault("server", "hotswap", "Model " + request->model_id() + " hot-swapped to " + new_id,
                "info", request->model_id());

    return grpc::Status::OK;
}

// ══════════════════════════════════════════════════════════════════════════
// Inference
// ══════════════════════════════════════════════════════════════════════════

grpc::Status CortexForgeServiceImpl::Infer(grpc::ServerContext*,
                                          const InferRequest* request,
                                          InferResponse* response)
{
    InternalModelState* state = nullptr;
    {
        std::lock_guard<std::mutex> lock(models_mutex_);
        auto it = models_.find(request->model_id());
        if (it == models_.end())
        {
            response->set_success(false);
            response->set_error_message("Model not found: " + request->model_id());
            total_errors_++;
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "Model not found");
        }
        state = it->second.get();
    }

    std::lock_guard<std::mutex> model_lock(state->model_mutex);

    auto& meta = state->metadata;
    std::string accel = meta.accelerator();

    // Simulate inference with realistic latency
    uint64_t preprocess_us = static_cast<uint64_t>(100 + (rand() % 200));
    uint64_t inference_us = static_cast<uint64_t>(SimulateLatencyUs(accel));
    uint64_t postprocess_us = static_cast<uint64_t>(50 + (rand() % 100));
    uint64_t total_us = preprocess_us + inference_us + postprocess_us;

    // Simulate occasional errors
    bool has_error = false;
    if (state->in_error_state || (rand() % 10000 == 0)) // 0.01% random failure
    {
        has_error = true;
        state->total_errors++;
        total_errors_++;
        state->in_error_state = true;
        state->error_description = "Inference timeout on " + accel;

        RecordFault(accel, "inference_timeout",
                    "Inference on model " + request->model_id() + " timed out",
                    "warning", request->model_id());

        // Auto-recover after some errors
        if (state->total_errors > 5)
        {
            state->in_error_state = false;
            state->error_description.clear();
            RecordFault(accel, "auto_recovery",
                        "Model " + request->model_id() + " auto-recovered",
                        "info", request->model_id());
        }
    }

    if (has_error)
    {
        response->set_success(false);
        response->set_error_message(state->error_description);
        response->set_latency_us(total_us);
        response->set_model_id(request->model_id());
        return grpc::Status::OK;
    }

    // Update model metrics
    meta.set_total_inferences(meta.total_inferences() + 1);
    meta.set_last_inference_timestamp_us(NowUs());
    UpdateLatencyStats(*state, total_us);

    // Update accelerator metrics
    {
        std::lock_guard<std::mutex> accel_lock(accel_mutex_);
        if (accelerators_.count(accel))
        {
            accelerators_[accel].total_tasks++;
            accelerators_[accel].utilization = std::min(100.0, accelerators_[accel].utilization + 2.0);
        }
    }

    total_inferences_++;

    // Build response
    response->set_model_id(request->model_id());
    response->set_output_data("inference_output_" + request->model_id() + "_seq_" +
                              std::to_string(meta.total_inferences()));
    response->set_latency_us(total_us);
    response->set_preprocess_us(preprocess_us);
    response->set_inference_us(inference_us);
    response->set_postprocess_us(postprocess_us);
    response->set_success(true);
    response->set_accelerator_used(accel);
    response->set_sequence_number(meta.total_inferences());
    response->set_confidence(0.85 + (rand() % 150) / 1000.0);

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::InferStream(grpc::ServerContext* context,
                                                 const InferRequest* request,
                                                 grpc::ServerWriter<InferResponse>* writer)
{
    InternalModelState* state = nullptr;
    {
        std::lock_guard<std::mutex> lock(models_mutex_);
        auto it = models_.find(request->model_id());
        if (it == models_.end())
        {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "Model not found");
        }
        state = it->second.get();
    }

    uint64_t seq = 0;
    while (!context->IsCancelled())
    {
        InferResponse response;
        response.set_model_id(request->model_id());
        response.set_output_data("stream_output_" + std::to_string(seq));
        response.set_latency_us(static_cast<uint64_t>(SimulateLatencyUs(state->metadata.accelerator())));
        response.set_success(true);
        response.set_sequence_number(++seq);
        response.set_confidence(0.85 + (rand() % 150) / 1000.0);
        response.set_accelerator_used(state->metadata.accelerator());

        if (!writer->Write(response))
            break;

        // Simulate frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
    }

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::BatchInfer(grpc::ServerContext*,
                                                const BatchInferRequest* request,
                                                BatchInferResponse* response)
{
    auto batch_start = NowUs();

    InternalModelState* state = nullptr;
    {
        std::lock_guard<std::mutex> lock(models_mutex_);
        auto it = models_.find(request->model_id());
        if (it == models_.end())
        {
            response->set_success(false);
            response->set_error_message("Model not found");
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "Model not found");
        }
        state = it->second.get();
    }

    std::lock_guard<std::mutex> model_lock(state->model_mutex);

    for (int i = 0; i < request->inputs_size(); i++)
    {
        uint64_t latency = static_cast<uint64_t>(SimulateLatencyUs(state->metadata.accelerator()));
        response->add_outputs("batch_output_" + std::to_string(i));
        response->add_latencies_us(latency);

        state->metadata.set_total_inferences(state->metadata.total_inferences() + 1);
        total_inferences_++;
    }

    response->set_model_id(request->model_id());
    response->set_success(true);
    response->set_total_batch_time_us(NowUs() - batch_start);

    return grpc::Status::OK;
}

// ══════════════════════════════════════════════════════════════════════════
// Accelerator Management
// ══════════════════════════════════════════════════════════════════════════

grpc::Status CortexForgeServiceImpl::GetAcceleratorStatus(grpc::ServerContext*,
                                                          const GetAcceleratorStatusRequest* request,
                                                          GetAcceleratorStatusResponse* response)
{
    std::lock_guard<std::mutex> lock(accel_mutex_);

    for (const auto& [name, accel] : accelerators_)
    {
        if (!request->accelerator_name().empty() && request->accelerator_name() != name)
            continue;
        *response->add_accelerators() = BuildAcceleratorStatus(accel);
    }

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::AssignAccelerator(grpc::ServerContext*,
                                                       const AssignAcceleratorRequest* request,
                                                       AssignAcceleratorResponse* response)
{
    std::lock_guard<std::mutex> lock(models_mutex_);

    auto it = models_.find(request->model_id());
    if (it == models_.end())
    {
        response->set_success(false);
        response->set_error_message("Model not found");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Model not found");
    }

    std::string new_accel = request->accelerator();
    if (new_accel == "auto")
    {
        static int rr = 0;
        std::vector<std::string> accels = {"GPU", "DLA0", "DLA1", "PVA"};
        new_accel = accels[rr++ % 4];
    }

    std::string old_accel = it->second->metadata.accelerator();
    it->second->metadata.set_accelerator(new_accel);

    // Update accelerator registrations
    {
        std::lock_guard<std::mutex> accel_lock(accel_mutex_);
        for (auto& [name, accel_state] : accelerators_)
        {
            auto& models = accel_state.loaded_models;
            models.erase(std::remove(models.begin(), models.end(), request->model_id()), models.end());
        }
        if (accelerators_.count(new_accel))
            accelerators_[new_accel].loaded_models.push_back(request->model_id());
    }

    response->set_success(true);
    response->set_assigned_accelerator(new_accel);
    response->set_migration_time_us(100000 + (rand() % 200000));

    RecordFault("server", "migration",
                "Model " + request->model_id() + " migrated from " + old_accel + " to " + new_accel,
                "info", request->model_id());

    return grpc::Status::OK;
}

// ══════════════════════════════════════════════════════════════════════════
// Monitoring & Telemetry
// ══════════════════════════════════════════════════════════════════════════

grpc::Status CortexForgeServiceImpl::GetMetrics(grpc::ServerContext*,
                                                const GetMetricsRequest* request,
                                                GetMetricsResponse* response)
{
    *response->mutable_current() = BuildCurrentSnapshot();

    // Include history if requested
    if (request->history_seconds() > 0)
    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        size_t count = std::min(static_cast<size_t>(request->history_seconds()), metrics_history_.size());
        auto start = metrics_history_.end() - count;
        for (auto it = start; it != metrics_history_.end(); ++it)
            *response->add_history() = *it;
    }

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::WatchMetrics(grpc::ServerContext* context,
                                                  const GetMetricsRequest*,
                                                  grpc::ServerWriter<MetricsSnapshot>* writer)
{
    while (!context->IsCancelled())
    {
        auto snapshot = BuildCurrentSnapshot();
        if (!writer->Write(snapshot))
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::GetPowerTelemetry(grpc::ServerContext*,
                                                        const GetPowerTelemetryRequest*,
                                                        GetPowerTelemetryResponse* response)
{
    std::lock_guard<std::mutex> lock(accel_mutex_);

    auto add_rail = [&](const std::string& name, double power) {
        auto* rail = response->add_rails();
        rail->set_name(name);
        rail->set_voltage_v(12.0 + (rand() % 100) / 100.0);
        rail->set_current_a(power / 12.0);
        rail->set_power_watts(power);
        rail->set_min_power_watts(power * 0.5);
        rail->set_max_power_watts(power * 1.5);
    };

    double total = 0;
    for (const auto& [name, accel] : accelerators_)
    {
        add_rail(name, accel.power_watts);
        total += accel.power_watts;
    }

    response->set_total_power_watts(total);
    response->set_power_budget_watts(60.0);
    response->set_power_cap_watts(75.0);
    response->set_power_mode(2); // MAXN mode 2
    response->set_timestamp_us(NowUs());

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::GetThermalTelemetry(grpc::ServerContext*,
                                                        const GetThermalTelemetryRequest*,
                                                        GetThermalTelemetryResponse* response)
{
    std::lock_guard<std::mutex> lock(accel_mutex_);

    int32_t max_temp = 0;
    bool any_throttling = false;

    for (const auto& [name, accel] : accelerators_)
    {
        auto* zone = response->add_zones();
        zone->set_name(name);
        zone->set_temperature_celsius(accel.temp_celsius);
        zone->set_critical_temperature(90);
        zone->set_throttle_temperature(80);
        zone->set_throttling(accel.temp_celsius > 75);
        zone->set_throttle_percent(std::max(0, (accel.temp_celsius - 75) * 5));

        max_temp = std::max(max_temp, accel.temp_celsius);
        if (accel.temp_celsius > 75)
            any_throttling = true;
    }

    response->set_max_temperature_celsius(max_temp);
    response->set_any_throttling(any_throttling);
    response->set_timestamp_us(NowUs());

    return grpc::Status::OK;
}

// ══════════════════════════════════════════════════════════════════════════
// Fault Detection & Recovery
// ══════════════════════════════════════════════════════════════════════════

grpc::Status CortexForgeServiceImpl::HealthCheck(grpc::ServerContext*,
                                                 const HealthCheckRequest* request,
                                                 HealthCheckResponse* response)
{
    uint32_t active = 0, total = 0;
    {
        std::lock_guard<std::mutex> lock(models_mutex_);
        total = static_cast<uint32_t>(models_.size());
        for (const auto& [id, state] : models_)
        {
            if (state->metadata.loaded() && state->metadata.status() == "active")
                active++;
        }
    }

    // Determine overall status
    auto status = HealthCheckResponse::SERVING;
    std::vector<std::string> warnings;

    {
        std::lock_guard<std::mutex> lock(accel_mutex_);
        for (const auto& [name, accel] : accelerators_)
        {
            if (accel.status == "error")
            {
                status = HealthCheckResponse::DEGRADED;
                warnings.push_back(name + " is in error state");
            }
        }
    }

    if (unrecovered_faults_ > 0)
    {
        status = HealthCheckResponse::DEGRADED;
        warnings.push_back(std::to_string(unrecovered_faults_.load()) + " unrecovered faults");
    }

    response->set_status(status);
    response->set_version("0.1.0");
    response->set_uptime_us(NowUs() - start_time_us_);
    response->set_active_models(active);
    response->set_total_models(total);
    response->set_total_inferences(total_inferences_);
    response->set_total_errors(total_errors_);

    // Memory usage
    {
        std::lock_guard<std::mutex> lock(accel_mutex_);
        uint64_t total_mem = 0, used_mem = 0;
        for (const auto& [name, accel] : accelerators_)
        {
            total_mem += accel.mem_total;
            used_mem += accel.mem_used;
        }
        if (total_mem > 0)
            response->set_memory_usage_percent(100.0 * used_mem / total_mem);
    }

    for (const auto& w : warnings)
        response->add_warnings(w);

    if (request->detailed())
    {
        auto& comps = *response->mutable_component_status();
        comps["server"] = "online";
        comps["model_registry"] = "online";
        comps["metrics_collector"] = "online";
        {
            std::lock_guard<std::mutex> lock(accel_mutex_);
            for (const auto& [name, accel] : accelerators_)
                comps[name] = accel.status;
        }
    }

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::GetFaultLog(grpc::ServerContext*,
                                                 const GetFaultLogRequest* request,
                                                 GetFaultLogResponse* response)
{
    std::lock_guard<std::mutex> lock(fault_mutex_);

    uint32_t count = 0;
    uint32_t unrecovered = 0;

    for (const auto& event : fault_log_)
    {
        if (request->max_events() > 0 && count >= request->max_events())
            break;
        if (!request->severity_filter().empty() && event.severity != request->severity_filter())
            continue;
        if (!request->include_recovered() && event.recovered)
            continue;

        auto* e = response->add_events();
        e->set_timestamp_us(event.timestamp_us);
        e->set_component(event.component);
        e->set_fault_type(event.fault_type);
        e->set_description(event.description);
        e->set_severity(event.severity);
        e->set_recovered(event.recovered);
        e->set_recovery_time_us(event.recovery_time_us);
        e->set_affected_model(event.affected_model);

        count++;
        if (!event.recovered)
            unrecovered++;
    }

    response->set_total_count(count);
    response->set_unrecovered_count(unrecovered);

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::ResetAccelerator(grpc::ServerContext*,
                                                      const ResetAcceleratorRequest* request,
                                                      ResetAcceleratorResponse* response)
{
    auto reset_start = NowUs();

    std::lock_guard<std::mutex> lock(accel_mutex_);

    auto it = accelerators_.find(request->accelerator_name());
    if (it == accelerators_.end())
    {
        response->set_success(false);
        response->set_error_message("Accelerator not found: " + request->accelerator_name());
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Accelerator not found");
    }

    auto& accel = it->second;
    std::vector<std::string> affected = accel.loaded_models;

    // Simulate reset
    accel.status = "online";
    accel.utilization = 0;
    accel.temp_celsius = 35;
    accel.failed_tasks = 0;
    accel.power_watts = 1.0;

    if (!request->force())
    {
        // Re-register models
        for (const auto& model_id : affected)
        {
            std::lock_guard<std::mutex> model_lock(models_mutex_);
            if (models_.count(model_id))
                models_[model_id]->metadata.set_status("active");
        }
    }

    uint64_t reset_time = NowUs() - reset_start;
    response->set_success(true);
    response->set_reset_time_us(reset_time);
    for (const auto& m : affected)
        response->add_affected_models(m);

    RecordFault(request->accelerator_name(), "reset",
                "Accelerator " + request->accelerator_name() + " reset",
                "warning", "");

    return grpc::Status::OK;
}

// ══════════════════════════════════════════════════════════════════════════
// System
// ══════════════════════════════════════════════════════════════════════════

grpc::Status CortexForgeServiceImpl::GetSystemInfo(grpc::ServerContext*,
                                                   const GetSystemInfoRequest*,
                                                   GetSystemInfoResponse* response)
{
    auto* info = response->mutable_info();
    info->set_platform("Jetson AGX Orin (simulated)");
    info->set_serial_number("AGX-ORIN-SIM-001");
    info->set_l4t_version("35.4.1");
    info->set_kernel_version("5.15.136-tegra");
    info->set_jetpack_version("6.0");
    info->set_total_ram_bytes(32ULL * 1024 * 1024 * 1024);
    info->set_free_ram_bytes(24ULL * 1024 * 1024 * 1024);
    info->set_cpu_count(12);
    info->set_cpu_util_percent(23.5);
    info->set_disk_total_bytes(64ULL * 1024 * 1024 * 1024);
    info->set_disk_free_bytes(48ULL * 1024 * 1024 * 1024);
    info->set_uptime_seconds((NowUs() - start_time_us_) / 1000000);
    info->set_server_version("0.1.0");
    info->set_power_mode(2);

    info->add_available_accelerators("GPU (Ampere, 2048 CUDA cores, 64 Tensor cores)");
    info->add_available_accelerators("DLA0 (NVDLA v2.0)");
    info->add_available_accelerators("DLA1 (NVDLA v2.0)");
    info->add_available_accelerators("PVA (PVA v2.0)");

    info->add_capabilities("tensorrt");
    info->add_capabilities("cuda");
    info->add_capabilities("dla");
    info->add_capabilities("pva");
    info->add_capabilities("tensorrt_quantization");
    info->add_capabilities("multi_model_concurrency");
    info->add_capabilities("model_hot_swap");
    info->add_capabilities("fault_recovery");
    info->add_capabilities("power_telemetry");
    info->add_capabilities("thermal_telemetry");

    return grpc::Status::OK;
}

grpc::Status CortexForgeServiceImpl::ShutdownServer(grpc::ServerContext*,
                                                    const ShutdownServerRequest* request,
                                                    ShutdownServerResponse* response)
{
    if (request->delay_seconds() > 0)
    {
        std::this_thread::sleep_for(std::chrono::seconds(request->delay_seconds()));
    }

    response->set_success(true);
    response->set_message("Server shutting down" +
                          (request->reason().empty() ? "" : ": " + request->reason()));

    // Schedule shutdown in a separate thread
    if (request->force())
    {
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::exit(0);
        }).detach();
    }

    return grpc::Status::OK;
}

// ══════════════════════════════════════════════════════════════════════════
// Private Helpers
// ══════════════════════════════════════════════════════════════════════════

std::string CortexForgeServiceImpl::GenerateModelId()
{
    return "model-" + std::to_string(next_model_id_++);
}

uint64_t CortexForgeServiceImpl::NowUs() const
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

double CortexForgeServiceImpl::SimulateLatencyUs(const std::string& accelerator)
{
    std::lock_guard<std::mutex> lock(rng_mutex_);
    if (accelerator == "GPU")
        return 500 + std::exponential_distribution<double>(0.002)(rng_);
    else if (accelerator == "DLA0" || accelerator == "DLA1")
        return 800 + std::exponential_distribution<double>(0.001)(rng_);
    else if (accelerator == "PVA")
        return 1200 + std::exponential_distribution<double>(0.0008)(rng_);
    else
        return 1000 + (rand() % 1000);
}

void CortexForgeServiceImpl::RecordFault(const std::string& component,
                                         const std::string& fault_type,
                                         const std::string& description,
                                         const std::string& severity,
                                         const std::string& affected_model)
{
    std::lock_guard<std::mutex> lock(fault_mutex_);
    FaultEventRecord event;
    event.timestamp_us = NowUs();
    event.component = component;
    event.fault_type = fault_type;
    event.description = description;
    event.severity = severity;
    event.recovered = (severity != "critical");
    event.recovery_time_us = event.recovered ? 500000 + (rand() % 2000000) : 0;
    event.affected_model = affected_model;

    fault_log_.push_back(event);
    if (fault_log_.size() > 1000)
        fault_log_.pop_front();

    if (!event.recovered)
        unrecovered_faults_++;
}

void CortexForgeServiceImpl::UpdateLatencyStats(InternalModelState& state, uint64_t latency_us)
{
    auto& meta = state.metadata;
    auto& samples = state.latency_samples;

    samples.push_back(latency_us);
    if (samples.size() > 1000)
        samples.erase(samples.begin());

    // Update running stats
    uint64_t total = meta.total_inferences();
    double old_avg = meta.avg_latency_us();
    meta.set_avg_latency_us(old_avg + (latency_us - old_avg) / total);

    // Update min/max
    if (latency_us < meta.min_latency_us() || meta.min_latency_us() == 0)
        meta.set_min_latency_us(latency_us);
    if (latency_us > meta.max_latency_us())
        meta.set_max_latency_us(latency_us);

    // Calculate percentiles from samples
    if (samples.size() >= 10)
    {
        auto sorted = samples;
        std::sort(sorted.begin(), sorted.end());
        size_t n = sorted.size();
        meta.set_p50_latency_us(sorted[n * 50 / 100]);
        meta.set_p95_latency_us(sorted[n * 95 / 100]);
        meta.set_p99_latency_us(sorted[n * 99 / 100]);
    }

    // Throughput (inferences per second over last 10 samples)
    if (samples.size() >= 10)
    {
        auto last10 = std::vector<uint64_t>(samples.end() - 10, samples.end());
        double avg = std::accumulate(last10.begin(), last10.end(), 0.0) / last10.size();
        if (avg > 0)
            meta.set_throughput_fps(1000000.0 / avg);
    }
}

MetricsSnapshot CortexForgeServiceImpl::BuildCurrentSnapshot()
{
    MetricsSnapshot snap;
    snap.set_timestamp_us(NowUs());

    {
        std::lock_guard<std::mutex> lock(accel_mutex_);
        for (const auto& [name, accel] : accelerators_)
        {
            if (name == "GPU")
            {
                snap.set_gpu_util_percent(accel.utilization);
                snap.set_gpu_mem_total_mb(accel.mem_total / (1024 * 1024));
                snap.set_gpu_mem_used_mb(accel.mem_used / (1024 * 1024));
                snap.set_gpu_power_watts(accel.power_watts);
                snap.set_gpu_temperature_celsius(accel.temp_celsius);
            }
            else if (name == "DLA0")
                snap.set_dla0_util_percent(accel.utilization);
            else if (name == "DLA1")
                snap.set_dla1_util_percent(accel.utilization);
            else if (name == "PVA")
                snap.set_pva_util_percent(accel.utilization);
        }
    }

    snap.set_total_inferences(total_inferences_);
    snap.set_inferences_per_second(100 + (rand() % 200));

    // Aggregate latency stats across all models
    {
        std::lock_guard<std::mutex> lock(models_mutex_);
        double total_avg = 0;
        uint32_t count = 0;
        for (const auto& [id, state] : models_)
        {
            if (state->metadata.total_inferences() > 0)
            {
                total_avg += state->metadata.avg_latency_us();
                count++;
            }
            // Per-model metrics
            ModelMetrics mm;
            mm.set_total_inferences(state->metadata.total_inferences());
            mm.set_avg_latency_us(state->metadata.avg_latency_us());
            mm.set_p50_latency_us(state->metadata.p50_latency_us());
            mm.set_p95_latency_us(state->metadata.p95_latency_us());
            mm.set_p99_latency_us(state->metadata.p99_latency_us());
            mm.set_inferences_per_second(static_cast<uint64_t>(state->metadata.throughput_fps()));
            mm.set_errors(state->metadata.error_count());
            mm.set_accuracy(state->metadata.accuracy_metric());
            mm.set_accelerator(state->metadata.accelerator());
            (*snap.mutable_per_model_metrics())[id] = mm;
        }
        if (count > 0)
            snap.set_avg_latency_us(total_avg / count);
    }

    // Power & thermal
    snap.set_soc_power_watts(8.0 + (rand() % 30) / 10.0);
    snap.set_total_power_watts(snap.gpu_power_watts() + snap.soc_power_watts());
    snap.set_cpu_temperature_celsius(50 + (rand() % 15));
    snap.set_board_temperature_celsius(42 + (rand() % 10));
    snap.set_thermal_throttle_percent(rand() % 5);

    return snap;
}

AcceleratorStatus CortexForgeServiceImpl::BuildAcceleratorStatus(const AccelState& accel)
{
    AcceleratorStatus status;
    status.set_name(accel.name);
    status.set_type(accel.type);
    status.set_utilization_percent(accel.utilization);
    status.set_frequency_hz(accel.freq_hz);
    status.set_temperature_celsius(accel.temp_celsius);
    status.set_memory_total_bytes(accel.mem_total);
    status.set_memory_used_bytes(accel.mem_used);
    status.set_power_state(2);
    status.set_power_watts(accel.power_watts);
    status.set_status(accel.status);
    status.set_total_tasks(accel.total_tasks);
    status.set_active_tasks(accel.loaded_models.size());
    status.set_queued_tasks(0);
    status.set_failed_tasks(accel.failed_tasks);
    status.set_avg_task_latency_us(1000);
    status.set_uptime_ms((NowUs() - start_time_us_) / 1000);
    status.set_firmware_version("1.3.0");
    status.set_driver_version("0.1.0");
    for (const auto& m : accel.loaded_models)
        status.add_loaded_models(m);

    return status;
}

} // namespace cortexforge
