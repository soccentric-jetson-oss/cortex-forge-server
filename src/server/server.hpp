// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Cortex Forge Contributors
//
// server.hpp - gRPC server wrapper for Cortex Forge
//
/// @brief Manages the gRPC server lifecycle: start, shutdown, and signal handling.

#pragma once

#include <string>
#include <memory>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

namespace cortexforge {

/// @brief Wraps a gRPC server with Cortex Forge service implementation.
class Server {
public:
    /// @brief Construct a server bound to the given address.
    /// @param address Host:port string (e.g., "0.0.0.0:50051").
    explicit Server(const std::string& address);

    /// @brief Start the gRPC server. Blocks until the server is ready.
    /// @return OK on success, error status otherwise.
    grpc::Status Start();

    /// @brief Gracefully shut down the server, draining in-flight requests.
    void Shutdown();

    /// @brief Check if the server is currently running.
    /// @return true if the server is active.
    [[nodiscard]] bool IsRunning() const;

private:
    std::string address_;
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    bool running_{false};
};

} // namespace cortexforge
