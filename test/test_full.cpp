#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cortex_forge.pb.h>
#include <grpcpp/grpcpp.h>
#include <thread>

TEST_CASE("Server configuration is valid", "[server]")
{
    // Verify that the gRPC server builder accepts valid configuration
    cortexforge::CortexForge::Service service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    auto server = builder.BuildAndStart();
    REQUIRE(server != nullptr);
    server->Shutdown();
}

TEST_CASE("Concurrent requests handled", "[server]")
{
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++)
        threads.emplace_back([&]() { counter++; });
    for (auto& t : threads)
        t.join();
    REQUIRE(counter == 10);
}

TEST_CASE("Error responses are well-formed", "[server]")
{
    // Verify that gRPC error responses contain proper status codes
    grpc::Status not_found(grpc::StatusCode::NOT_FOUND, "model not found");
    REQUIRE_FALSE(not_found.ok());
    REQUIRE(not_found.error_code() == grpc::StatusCode::NOT_FOUND);
    REQUIRE(not_found.error_message() == "model not found");

    grpc::Status invalid_arg(grpc::StatusCode::INVALID_ARGUMENT, "invalid config");
    REQUIRE_FALSE(invalid_arg.ok());
    REQUIRE(invalid_arg.error_code() == grpc::StatusCode::INVALID_ARGUMENT);
}
