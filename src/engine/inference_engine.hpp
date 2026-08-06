// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// inference_engine.hpp - Abstract inference engine interface
//
/// @brief Defines the interface for inference backends (TensorRT, ONNX, etc.).

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cortex_forge.pb.h>

namespace cortexforge {

/// @brief Result of a single inference execution.
struct InferenceResult {
    bool success{false};
    std::vector<uint8_t> output_data;
    uint64_t latency_us{0};
    std::string error_message;
};

/// @brief Abstract base class for inference engines.
///
/// Concrete implementations support TensorRT engine plans, ONNX Runtime,
/// or custom backends. Each engine manages a single loaded model.
class InferenceEngine {
public:
    virtual ~InferenceEngine() = default;

    /// @brief Load a model from file.
    /// @param model_path Path to the model file.
    /// @return true on success.
    virtual bool LoadModel(const std::string& model_path) = 0;

    /// @brief Unload the model and free resources.
    virtual void UnloadModel() = 0;

    /// @brief Run inference on input data.
    /// @param input_data Raw input tensor bytes.
    /// @return InferenceResult with output data and latency.
    virtual InferenceResult Infer(const std::vector<uint8_t>& input_data) = 0;

    /// @brief Get the model's input tensor size in bytes.
    virtual size_t GetInputSize() const = 0;

    /// @brief Get the model's output tensor size in bytes.
    virtual size_t GetOutputSize() const = 0;

    /// @brief Check if a model is loaded.
    virtual bool IsLoaded() const = 0;
};

/// @brief Stub inference engine for development/testing.
///
/// Simulates model loading and inference with configurable latency.
/// In production, this would be replaced with TensorRT or ONNX Runtime.
class StubInferenceEngine : public InferenceEngine {
public:
    StubInferenceEngine() = default;

    bool LoadModel(const std::string& /*model_path*/) override {
        loaded_ = true;
        input_size_ = 1024 * 1024;
        output_size_ = 1024 * 1024;
        return true;
    }

    void UnloadModel() override {
        loaded_ = false;
    }

    InferenceResult Infer(const std::vector<uint8_t>& /*input_data*/) override {
        auto latency_ms = 1 + (rand() % 5);
        std::this_thread::sleep_for(std::chrono::milliseconds(latency_ms));

        InferenceResult result;
        result.success = true;
        result.output_data.resize(output_size_, 0x42);
        result.latency_us = static_cast<uint64_t>(latency_ms) * 1000;
        return result;
    }

    size_t GetInputSize() const override { return input_size_; }
    size_t GetOutputSize() const override { return output_size_; }
    bool IsLoaded() const override { return loaded_; }

private:
    bool loaded_{false};
    size_t input_size_{0};
    size_t output_size_{0};
};

} // namespace cortexforge

/* OpenCV integration for embedded optimization */
#ifdef HAS_OPENCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

class OpenCVOptimizedEngine : public InferenceEngine {
public:
    bool LoadModel(const std::string& path) override;
    void UnloadModel() override;
    InferenceResult Infer(const std::vector<uint8_t>& data) override;
    size_t GetInputSize() const override;
    size_t GetOutputSize() const override;
    bool IsLoaded() const override;
private:
    cv::Mat preprocess(const cv::Mat& input);
    bool loaded_{false};
};
#endif
