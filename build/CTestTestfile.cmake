# CMake generated Testfile for 
# Source directory: /home/sandesh/jetson/build/cortex-forge-server
# Build directory: /home/sandesh/jetson/build/cortex-forge-server/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[cortex-forge-server-test]=] "/home/sandesh/jetson/build/cortex-forge-server/build/cortex-forge-server-test")
set_tests_properties([=[cortex-forge-server-test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/sandesh/jetson/build/cortex-forge-server/CMakeLists.txt;136;add_test;/home/sandesh/jetson/build/cortex-forge-server/CMakeLists.txt;0;")
subdirs("_deps/catch2-build")
subdirs("examples")
