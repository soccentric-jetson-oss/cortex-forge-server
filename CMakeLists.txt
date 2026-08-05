# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Cortex Forge Contributors

cmake_minimum_required(VERSION 3.20)
project(cortex-forge-server VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── Options ────────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(ENABLE_DOXYGEN "Build Doxygen documentation" ON)

# ── Dependencies ───────────────────────────────────────────────────────────
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)

# ── Compiler warnings (per-target) ────────────────────────────────────────
include(cmake/CompilerWarnings.cmake)

# ── Protobuf / gRPC generation ─────────────────────────────────────────────
set(PROTO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/proto/cortex_forge.proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

set(PROTO_SRCS
    ${PROTO_GEN_DIR}/cortex_forge.pb.cc
    ${PROTO_GEN_DIR}/cortex_forge.grpc.pb.cc
)
set(PROTO_HDRS
    ${PROTO_GEN_DIR}/cortex_forge.pb.h
    ${PROTO_GEN_DIR}/cortex_forge.grpc.pb.h
)

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
    COMMAND protoc
        --proto_path ${CMAKE_CURRENT_SOURCE_DIR}/proto
        --cpp_out ${PROTO_GEN_DIR}
        --grpc_out ${PROTO_GEN_DIR}
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        ${PROTO_SRC}
    DEPENDS ${PROTO_SRC}
    COMMENT "Generating protobuf/gRPC sources"
)

add_library(cortex_forge_proto OBJECT ${PROTO_SRCS})
target_include_directories(cortex_forge_proto PUBLIC ${PROTO_GEN_DIR})
target_link_libraries(cortex_forge_proto PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)

# Suppress strict warnings for generated protobuf code
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cortex_forge_proto PRIVATE -Wno-sign-conversion -Wno-old-style-cast)
endif()

# ── Library: cortex-forge-server-core ──────────────────────────────────────
add_library(cortex-forge-server-core STATIC
    src/server/server.cpp
    src/server/service_impl.cpp
    src/engine/inference_engine.cpp
    src/engine/model_registry.cpp
    src/accelerator/accelerator_manager.cpp
    src/monitor/metrics_collector.cpp
)

target_include_directories(cortex-forge-server-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROTO_GEN_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/server
        ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/src/accelerator
        ${CMAKE_CURRENT_SOURCE_DIR}/src/monitor
)

target_link_libraries(cortex-forge-server-core
    PUBLIC
        cortex_forge_proto
        protobuf::libprotobuf
        gRPC::grpc++
        gRPC::grpc
        Threads::Threads
        fmt::fmt
)

target_strict_warnings(cortex-forge-server-core)

# ── Executable: cortex-forge-server ────────────────────────────────────────
add_executable(cortex-forge-server
    src/main.cpp
)

target_link_libraries(cortex-forge-server
    PRIVATE
        cortex-forge-server-core
)

target_strict_warnings(cortex-forge-server)

# ── Tests ──────────────────────────────────────────────────────────────────
if(BUILD_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(cortex-forge-server-test
        test/test_server.cpp
        test/test_model_registry.cpp
        test/test_metrics.cpp
    )

    target_link_libraries(cortex-forge-server-test
        PRIVATE
            cortex-forge-server-core
            Catch2::Catch2WithMain
    )

    target_strict_warnings(cortex-forge-server-test)

    include(CTest)
    add_test(NAME cortex-forge-server-test
             COMMAND cortex-forge-server-test)
endif()

# ── Examples ────────────────────────────────────────────────────────────────
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ── Doxygen ────────────────────────────────────────────────────────────────
if(ENABLE_DOXYGEN)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        configure_file(docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile @ONLY)
        add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile
            COMMENT "Generating Doxygen documentation"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endif()

# ── Install ────────────────────────────────────────────────────────────────
install(TARGETS cortex-forge-server
        RUNTIME DESTINATION bin)
install(FILES ${PROTO_HDRS}
        DESTINATION include/cortex-forge)
