// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Cortex Forge Contributors
//
// server.cpp - gRPC server implementation
//
/// @brief Implements the gRPC server lifecycle.

#include "server/server.hpp"
#include "server/service_impl.hpp"
#include <iostream>

namespace cortexforge {

Server::Server(const std::string& address)
    : address_(address) {}

grpc::Status Server::Start() {
    auto service = std::make_shared<CortexForgeServiceImpl>();

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());

    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();

    if (!server_) {
        return grpc::Status(grpc::StatusCode::INTERNAL,
                           "Failed to build and start gRPC server");
    }

    running_ = true;
    std::cout << "gRPC server listening on " << address_ << "\n";
    return grpc::Status::OK;
}

void Server::Shutdown() {
    if (!running_) return;

    running_ = false;
    server_->Shutdown();
    cq_->Shutdown();
    std::cout << "Server shut down gracefully.\n";
}

bool Server::IsRunning() const {
    return running_;
}

} // namespace cortexforge
