// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// model_registry.hpp - Thread-safe model registry
//
/// @brief Manages loaded models with thread-safe access.

#pragma once

#include <cortex_forge.pb.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace cortexforge
{

class InferenceEngine;

/// @brief Thread-safe registry of loaded models.
///
/// Maps model IDs to ModelInfo protobufs and inference engine instances.
class ModelRegistry
{
  public:
    ModelRegistry() = default;

    /// @brief Register a loaded model.
    /// @param info Model metadata.
    /// @param engine Inference engine instance.
    void Register(const ModelInfo& info, std::shared_ptr<InferenceEngine> engine);

    /// @brief Unregister and unload a model.
    /// @param model_id Unique model identifier.
    /// @return true if found and removed.
    bool Unregister(const std::string& model_id);

    /// @brief Get model info by ID.
    /// @param model_id Unique model identifier.
    /// @param[out] info Filled with model metadata if found.
    /// @return true if found.
    bool GetInfo(const std::string& model_id, ModelInfo& info) const;

    /// @brief Get inference engine by model ID.
    /// @param model_id Unique model identifier.
    /// @return Shared pointer to engine, or nullptr if not found.
    std::shared_ptr<InferenceEngine> GetEngine(const std::string& model_id) const;

    /// @brief List all registered models.
    /// @return Vector of model info.
    std::vector<ModelInfo> ListAll() const;

    /// @brief Get the number of registered models.
    size_t Count() const;

  private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ModelInfo> infos_;
    std::unordered_map<std::string, std::shared_ptr<InferenceEngine>> engines_;
};

} // namespace cortexforge
