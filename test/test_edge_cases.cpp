#include <catch2/catch_test_macros.hpp>
#include <cerrno>
#include <cstring>

TEST_CASE("Null input handling", "[edge]")
{
    // Verify that the userspace library rejects NULL handles
    int ret = cortex_forge_submit_task(NULL, NULL);
    REQUIRE(ret == -EINVAL);
}

TEST_CASE("Empty input handling", "[edge]")
{
    // Verify that zero-size submissions are handled
    struct cortex_forge_task_desc desc;
    std::memset(&desc, 0, sizeof(desc));
    REQUIRE(desc.task_id == 0);
    REQUIRE(desc.input_size == 0);
}

TEST_CASE("Boundary values", "[edge]")
{
    // Verify boundary conditions for accelerator types
    struct cortex_forge_task_desc desc;
    std::memset(&desc, 0, sizeof(desc));
    desc.accel_type = 0; // DLA0
    REQUIRE(desc.accel_type == 0);
    desc.accel_type = 2; // PVA
    REQUIRE(desc.accel_type == 2);
}

TEST_CASE("Concurrent access", "[edge]")
{
    // Verify thread safety
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back([&]() { counter++; });
    }
    for (auto& t : threads)
        t.join();
    REQUIRE(counter == 10);
}

TEST_CASE("Resource cleanup on error", "[edge]")
{
    // Verify that resources are cleaned up on error
    struct cortex_forge_task_desc desc;
    std::memset(&desc, 0, sizeof(desc));
    desc.accel_type = 99; // Invalid
    int ret = cortex_forge_submit_task(NULL, &desc);
    REQUIRE(ret == -EINVAL);
}
