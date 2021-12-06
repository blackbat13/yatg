cmake_minimum_required(VERSION 3.13...3.19 FATAL_ERROR)
project(libyatg VERSION 0.1.3 LANGUAGES CXX)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

##############################################
# Create target and set properties

add_library(YATG src/yatg.cpp)

target_include_directories(YATG
        PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
        )

target_compile_options(YATG PRIVATE -Werror)
target_compile_features(YATG PRIVATE cxx_std_11)

##############################################
# Installation instructions

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/YATG)

install(TARGETS YATG
        EXPORT YATG-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT YATG-targets
        FILE
            YATGTargets.cmake
        NAMESPACE
            YATG::
        DESTINATION
            ${INSTALL_CONFIGDIR}
        )

# Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/YATGConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/YATGConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/YATGConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
        )

# Install the config, configversion and custom find modules
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/YATGConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/YATGConfigVersion.cmake
        DESTINATION ${INSTALL_CONFIGDIR}
        )

##############################################
## Exporting from the build tree

export(EXPORT YATG-targets
        FILE ${CMAKE_CURRENT_BINARY_DIR}/YATGTargets.cmake
        NAMESPACE YATG::)

# Register package in the User Package Registry
export(PACKAGE YATG)