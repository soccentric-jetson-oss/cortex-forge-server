// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// model_registry.cpp - Thread-safe model registry implementation

#include "engine/model_registry.hpp"
#include "engine/inference_engine.hpp"

namespace cortexforge
{

void ModelRegistry::Register(const ModelInfo& info, std::shared_ptr<InferenceEngine> engine)
{
    std::lock_guard<std::mutex> lock(mutex_);
    infos_[info.model_id()] = info;
    engines_[info.model_id()] = std::move(engine);
}

bool ModelRegistry::Unregister(const std::string& model_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = infos_.find(model_id);
    if (it == infos_.end())
        return false;

    if (auto eng_it = engines_.find(model_id); eng_it != engines_.end())
    {
        eng_it->second->UnloadModel();
        engines_.erase(eng_it);
    }

    infos_.erase(it);
    return true;
}

bool ModelRegistry::GetInfo(const std::string& model_id, ModelInfo& info) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = infos_.find(model_id);
    if (it == infos_.end())
        return false;
    info = it->second;
    return true;
}

std::shared_ptr<InferenceEngine> ModelRegistry::GetEngine(const std::string& model_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = engines_.find(model_id);
    return it != engines_.end() ? it->second : nullptr;
}

std::vector<ModelInfo> ModelRegistry::ListAll() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ModelInfo> result;
    result.reserve(infos_.size());
    for (const auto& [id, info] : infos_)
    {
        result.push_back(info);
    }
    return result;
}

size_t ModelRegistry::Count() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return infos_.size();
}

} // namespace cortexforge
