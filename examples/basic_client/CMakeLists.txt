# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Cortex Forge Contributors

add_executable(cortex-forge-client
    main.cpp
)

target_link_libraries(cortex-forge-client
    PRIVATE
        cortex-forge-server-core
)

target_strict_warnings(cortex-forge-client)
