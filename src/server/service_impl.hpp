// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// service_impl.hpp - Ultra feature-rich gRPC service implementation
//
/// @brief Implements the full CortexForge gRPC service with model lifecycle,
/// multi-accelerator inference, streaming, telemetry, power/thermal monitoring,
/// and fault recovery.

#pragma once

#include <cortex_forge.grpc.pb.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace cortexforge
{

/// @brief Internal model state with full metrics tracking.
struct InternalModelState {
    ModelMetadata metadata;
    std::vector<uint64_t> latency_samples;  // Rolling window of latencies
    uint64_t total_errors{0};
    uint64_t last_inference_us{0};
    bool in_error_state{false};
    std::string error_description;
    uint64_t load_timestamp_us{0};
    std::mutex model_mutex;
};

/// @brief Fault event record.
struct FaultEventRecord {
    uint64_t timestamp_us;
    std::string component;
    std::string fault_type;
    std::string description;
    std::string severity;
    bool recovered;
    uint64_t recovery_time_us;
    std::string affected_model;
};

/// @brief Ultra feature-rich implementation of the CortexForge gRPC service.
class CortexForgeServiceImpl final : public CortexForge::Service
{
  public:
    CortexForgeServiceImpl();
    ~CortexForgeServiceImpl();

    // ── Model Lifecycle Management ─────────────────────────────────────────
    grpc::Status LoadModel(grpc::ServerContext* context, const LoadModelRequest* request,
                           LoadModelResponse* response) override;
    grpc::Status UnloadModel(grpc::ServerContext* context, const UnloadModelRequest* request,
                             UnloadModelResponse* response) override;
    grpc::Status ListModels(grpc::ServerContext* context, const ListModelsRequest* request,
                            ListModelsResponse* response) override;
    grpc::Status GetModelInfo(grpc::ServerContext* context, const GetModelInfoRequest* request,
                              GetModelInfoResponse* response) override;
    grpc::Status HotSwapModel(grpc::ServerContext* context, const HotSwapModelRequest* request,
                              HotSwapModelResponse* response) override;

    // ── Inference ──────────────────────────────────────────────────────────
    grpc::Status Infer(grpc::ServerContext* context, const InferRequest* request,
                       InferResponse* response) override;
    grpc::Status InferStream(grpc::ServerContext* context, const InferRequest* request,
                             grpc::ServerWriter<InferResponse>* writer) override;
    grpc::Status BatchInfer(grpc::ServerContext* context, const BatchInferRequest* request,
                            BatchInferResponse* response) override;

    // ── Accelerator Management ─────────────────────────────────────────────
    grpc::Status GetAcceleratorStatus(grpc::ServerContext* context,
                                      const GetAcceleratorStatusRequest* request,
                                      GetAcceleratorStatusResponse* response) override;
    grpc::Status AssignAccelerator(grpc::ServerContext* context,
                                   const AssignAcceleratorRequest* request,
                                   AssignAcceleratorResponse* response) override;

    // ── Monitoring & Telemetry ────────────────────────────────────────────
    grpc::Status GetMetrics(grpc::ServerContext* context, const GetMetricsRequest* request,
                            GetMetricsResponse* response) override;
    grpc::Status WatchMetrics(grpc::ServerContext* context, const GetMetricsRequest* request,
                              grpc::ServerWriter<MetricsSnapshot>* writer) override;
    grpc::Status GetPowerTelemetry(grpc::ServerContext* context,
                                   const GetPowerTelemetryRequest* request,
                                   GetPowerTelemetryResponse* response) override;
    grpc::Status GetThermalTelemetry(grpc::ServerContext* context,
                                     const GetThermalTelemetryRequest* request,
                                     GetThermalTelemetryResponse* response) override;

    // ── Fault Detection & Recovery ─────────────────────────────────────────
    grpc::Status HealthCheck(grpc::ServerContext* context, const HealthCheckRequest* request,
                             HealthCheckResponse* response) override;
    grpc::Status GetFaultLog(grpc::ServerContext* context, const GetFaultLogRequest* request,
                             GetFaultLogResponse* response) override;
    grpc::Status ResetAccelerator(grpc::ServerContext* context,
                                  const ResetAcceleratorRequest* request,
                                  ResetAcceleratorResponse* response) override;

    // ── System ────────────────────────────────────────────────────────────
    grpc::Status GetSystemInfo(grpc::ServerContext* context,
                              const GetSystemInfoRequest* request,
                              GetSystemInfoResponse* response) override;
    grpc::Status ShutdownServer(grpc::ServerContext* context,
                                const ShutdownServerRequest* request,
                                ShutdownServerResponse* response) override;

  private:
    // Thread-safe model registry
    std::mutex models_mutex_;
    std::unordered_map<std::string, std::unique_ptr<InternalModelState>> models_;
    std::atomic<uint64_t> next_model_id_{1};
    std::atomic<uint64_t> total_inferences_{0};
    std::atomic<uint64_t> total_errors_{0};
    uint64_t start_time_us_;

    // Accelerator state
    struct AccelState {
        std::string name;
        std::string type;
        double utilization;
        uint64_t freq_hz;
        int32_t temp_celsius;
        uint64_t mem_total;
        uint64_t mem_used;
        double power_watts;
        std::string status;
        uint64_t total_tasks;
        uint64_t failed_tasks;
        std::vector<std::string> loaded_models;
    };
    std::mutex accel_mutex_;
    std::map<std::string, AccelState> accelerators_;

    // Fault log
    std::mutex fault_mutex_;
    std::deque<FaultEventRecord> fault_log_;
    std::atomic<uint64_t> unrecovered_faults_{0};

    // Metrics history
    std::mutex history_mutex_;
    std::deque<MetricsSnapshot> metrics_history_;
    static constexpr size_t MAX_HISTORY = 3600; // 1 hour at 1 sample/sec

    // RNG for simulated metrics
    std::mt19937 rng_;
    std::mutex rng_mutex_;

    // Background metrics updater
    std::thread metrics_thread_;
    std::atomic<bool> running_{true};
    void MetricsUpdateLoop();

    // Helpers
    std::string GenerateModelId();
    uint64_t NowUs() const;
    double SimulateLatencyUs(const std::string& accelerator);
    void RecordFault(const std::string& component, const std::string& fault_type,
                     const std::string& description, const std::string& severity,
                     const std::string& affected_model = "");
    void UpdateLatencyStats(InternalModelState& state, uint64_t latency_us);
    MetricsSnapshot BuildCurrentSnapshot();
    AcceleratorStatus BuildAcceleratorStatus(const AccelState& accel);
};

} // namespace cortexforge
