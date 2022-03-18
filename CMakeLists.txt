# Specify the minimum version.
cmake_minimum_required(VERSION 3.9)

# Specify the project info.
project(cancomm LANGUAGES C VERSION 1.0.0 DESCRIPTION "SocketCAN communication library")

# For access to standard installation directory variables (CMAKE_INSTALL_xDIR).
include(GNUInstallDirs)

# Declare the library target.
add_library(${PROJECT_NAME} SHARED
    source/cancomm.c
    source/cancomm.h
)

# Set the version property.
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

# Set the shared object version property to the project's major version. This results
# in a cancomm.so.1 symlink to cancomm.so.1.0.0 in the example case of version 1.0.0.
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

# Set the public header property to the one with the actual API.
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER source/cancomm.h)

# Configure the directories to search for header files.
target_include_directories(${PROJECT_NAME} PRIVATE source)

# Create the pkg-config file from the template.
configure_file(source/${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)

# Set library shared object and API header file to install.
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Set pkg-config file to install.
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

