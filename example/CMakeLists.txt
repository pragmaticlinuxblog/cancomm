# Specify the minimum version.
cmake_minimum_required(VERSION 3.9)

# Specify the project info.
project(example VERSION 1.0.0 DESCRIPTION "SocketCAN communication library example")

# Finds the pkg-config executable adds the pkg_check_modules() function, among others.
find_package(PkgConfig REQUIRED)

# Create imported target with name PkgConfig::CANCOMM
pkg_check_modules(CANCOMM REQUIRED IMPORTED_TARGET cancomm)

# Declare the executable target.
add_executable(${PROJECT_NAME} example.c)

# Specify the libraries to use when linking the target.
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::CANCOMM)

