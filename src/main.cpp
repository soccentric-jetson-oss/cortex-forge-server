// SPDX-License-Identifier: MIT
// Copyright (c) 2026 SoC Centric LLC
//
// main.cpp - Entry point for the Cortex Forge gRPC server
//
// Starts the gRPC server, loads configuration, and handles signals
// for graceful shutdown.

#include "server/server.hpp"
#include <thread>
#include <iostream>
#include <csignal>
#include <atomic>

static std::atomic<bool> g_running{true};

static void signal_handler(int) {
    g_running.store(false);
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::string server_address = "127.0.0.1:50051";

    // Parse command-line args
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--address" && i + 1 < argc) {
            server_address = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Cortex Forge Server v0.1.0\n"
                      << "Usage: " << argv[0] << " [--address <host:port>]\n";
            return 0;
        }
    }

    std::cout << "Cortex Forge Server v0.1.0\n";
    std::cout << "Listening on " << server_address << "\n";

    cortexforge::Server server(server_address);
    auto status = server.Start();
    if (!status.ok()) {
        std::cerr << "Failed to start server: " << status.error_message() << "\n";
        return 1;
    }

    std::cout << "Server started. Press Ctrl+C to stop.\n";

    // Wait for shutdown signal
    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Shutting down...\n";
    server.Shutdown();
    std::cout << "Server stopped.\n";

    return 0;
}
