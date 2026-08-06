// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// server.cpp - gRPC server implementation with full service lifecycle
//
/// @brief Manages the gRPC server lifecycle with proper service ownership.

#include "server/server.hpp"
#include "server/service_impl.hpp"
#include <iostream>

namespace cortexforge
{

Server::Server(const std::string& address)
    : address_(address)
    , service_(std::make_shared<CortexForgeServiceImpl>())
{
}

grpc::Status Server::Start()
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
    builder.RegisterService(service_.get());

    // Set max message size for large model/inference data
    builder.SetMaxReceiveMessageSize(100 * 1024 * 1024);  // 100 MB
    builder.SetMaxSendMessageSize(100 * 1024 * 1024);     // 100 MB

    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();

    if (!server_)
    {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to build and start gRPC server");
    }

    running_ = true;
    std::cout << "Cortex Forge Server v0.1.0 listening on " << address_ << "\n";
    std::cout << "  Accelerators: GPU, DLA0, DLA1, PVA\n";
    std::cout << "  Capabilities: tensorrt, cuda, dla, pva, hot-swap, fault-recovery\n";
    return grpc::Status::OK;
}

void Server::Shutdown()
{
    if (!running_)
        return;

    running_ = false;
    server_->Shutdown();
    cq_->Shutdown();
    std::cout << "Cortex Forge Server shut down gracefully.\n";
}

bool Server::IsRunning() const
{
    return running_;
}

} // namespace cortexforge
