cmake_minimum_required(VERSION 3.18)

project(RTN_Kronop_Bridge VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(ANDROID)
    set(PLATFORM "ANDROID")
elseif(APPLE)
    set(PLATFORM "IOS")
else()
    set(PLATFORM "DESKTOP")
endif()

message(STATUS "Building RTN_Kronop_Bridge for platform: ${PLATFORM}")

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
endif()

# Platform-specific configurations
if(ANDROID)
    # Android-specific configurations
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DANDROID")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    
    # Android NDK libraries
    find_library(log-lib log)
    find_library(jnigraphics-lib jnigraphics)
    
elseif(APPLE)
    # iOS-specific configurations
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAPPLE -D_IOS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-arc")
    
    # iOS frameworks
    find_library(Foundation_FRAMEWORK Foundation)
    find_library(CoreFoundation_FRAMEWORK CoreFoundation)
    find_library(UIKit_FRAMEWORK UIKit)
    
else()
    # Desktop configurations
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDESKTOP")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Master_Control
    ${CMAKE_CURRENT_SOURCE_DIR}/../Thermal_Guard
    ${CMAKE_CURRENT_SOURCE_DIR}/../AI_Workers
    ${CMAKE_CURRENT_SOURCE_DIR}/../IO_Vault
    ${CMAKE_CURRENT_SOURCE_DIR}/../AI1/KRONOP_AI_V2_FINAL_BUILD/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../AI1/Kronop_Cleaner AI/src
)

# Source files
set(RTN_BRIDGE_SOURCES
    RTNVideoAI.cpp
    RTNVideoAI.hpp
)

# Core Video AI sources
set(VIDEO_AI_SOURCES
    ../Master_Control/Video_AI_Coordinator.cpp
    ../Master_Control/Video_AI_Coordinator.hpp
    ../Master_Control/Input_Collector/Input_Collector.cpp
    ../Master_Control/Input_Collector/Input_Collector.hpp
    ../Master_Control/Master_Splitter/Master_Splitter.cpp
    ../Master_Control/Master_Splitter/Master_Splitter.hpp
    ../Master_Control/Merge_Master/Merge_Master.cpp
    ../Master_Control/Merge_Master/Merge_Master.hpp
    ../Master_Control/Output_Dispatcher/Output_Dispatcher.cpp
    ../Master_Control/Output_Dispatcher/Output_Dispatcher.hpp
    ../Master_Control/Emergency_Shutdown.cpp
    ../Master_Control/Emergency_Shutdown.hpp
    ../Thermal_Guard/Thermal_Monitor.cpp
    ../Thermal_Guard/Thermal_Monitor.hpp
    ../Thermal_Guard/Emergency_Brake.cpp
    ../Thermal_Guard/Emergency_Brake.hpp
    ../AI_Workers/Worker_Manager.cpp
    ../AI_Workers/Worker_Manager.hpp
    ../IO_Vault/Shared_Memory_Manager.cpp
    ../IO_Vault/Shared_Memory_Manager.hpp
    ../AI1/KRONOP_AI_V2_FINAL_BUILD/src/Upscale_Core.cpp
    ../AI1/KRONOP_AI_V2_FINAL_BUILD/src/Upscale_Core.hpp
    ../AI1/KRONOP_AI_V2_FINAL_BUILD/src/Weights_Manager.hpp
    ../AI1/Kronop_JNI_Interface.cpp
    ../AI1/Kronop_Master_Bridge.hpp
)

# Combine all sources
set(ALL_SOURCES ${RTN_BRIDGE_SOURCES} ${VIDEO_AI_SOURCES})

# Create library
add_library(RTN_Kronop_Bridge SHARED ${ALL_SOURCES})

# Platform-specific linking
if(ANDROID)
    target_link_libraries(RTN_Kronop_Bridge
        ${log-lib}
        ${jnigraphics-lib}
        android
        EGL
        GLESv2
        vulkan
    )
    
    # Android-specific properties
    set_target_properties(RTN_Kronop_Bridge PROPERTIES
        ANDROID_STL c++_shared
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${ANDROID_ABI}
    )
    
elseif(APPLE)
    target_link_libraries(RTN_Kronop_Bridge
        ${Foundation_FRAMEWORK}
        ${CoreFoundation_FRAMEWORK}
        ${UIKit_FRAMEWORK}
        Foundation
        CoreFoundation
        UIKit
    )
    
    # iOS-specific properties
    set_target_properties(RTN_Kronop_Bridge PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER ""
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.kronop.rtnvideoai"
    )
    
else()
    # Desktop linking
    target_link_libraries(RTN_Kronop_Bridge
        pthread
        dl
    )
endif()

# JSI integration (React Native)
option(ENABLE_JSI "Enable React Native JSI integration" ON)
if(ENABLE_JSI)
    # Find React Native JSI
    find_package(fbjni REQUIRED)
    find_package(ReactAndroid REQUIRED)
    
    target_include_directories(RTN_Kronop_Bridge PRIVATE
        ${ReactAndroid_INCLUDE_DIR}
        ${fbjni_INCLUDE_DIR}
    )
    
    target_link_libraries(RTN_Kronop_Bridge
        ${ReactAndroid_LIBRARIES}
        ${fbjni_LIBRARIES}
    )
    
    target_compile_definitions(RTN_Kronop_Bridge PRIVATE JSI=1)
    
    message(STATUS "JSI integration enabled")
endif()

# Vulkan support (for GPU acceleration)
option(ENABLE_VULKAN "Enable Vulkan GPU acceleration" ON)
if(ENABLE_VULKAN)
    find_package(Vulkan REQUIRED)
    
    target_include_directories(RTN_Kronop_Bridge PRIVATE
        ${Vulkan_INCLUDE_DIRS}
    )
    
    target_link_libraries(RTN_Kronop_Bridge
        ${Vulkan_LIBRARIES}
    )
    
    target_compile_definitions(RTN_Kronop_Bridge PRIVATE VULKAN_ENABLED=1)
    
    message(STATUS "Vulkan support enabled")
endif()

# Debug configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(RTN_Kronop_Bridge PRIVATE DEBUG=1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
else()
    target_compile_definitions(RTN_Kronop_Bridge PRIVATE NDEBUG=1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
endif()

# Installation
install(TARGETS RTN_Kronop_Bridge
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(FILES
    RTNVideoAI.hpp
    DESTINATION include/RTN_Kronop_Bridge
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/RTN_Kronop_BridgeConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/RTN_Kronop_BridgeConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/RTN_Kronop_BridgeConfig.cmake"
    INSTALL_DESTINATION lib/cmake/RTN_Kronop_Bridge
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/RTN_Kronop_BridgeConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/RTN_Kronop_BridgeConfigVersion.cmake"
    DESTINATION lib/cmake/RTN_Kronop_Bridge
)

# Testing
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    
    # Test executable
    add_executable(RTN_Kronop_Bridge_Tests
        tests/test_main.cpp
        tests/test_rtn_video_ai.cpp
    )
    
    target_link_libraries(RTN_Kronop_Bridge_Tests
        RTN_Kronop_Bridge
        gtest
        gtest_main
    )
    
    add_test(NAME RTN_Bridge_Tests COMMAND RTN_Kronop_Bridge_Tests)
endif()

# Documentation
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
        set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
        
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
                       ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
        
        add_custom_target(docs
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "RTN_Kronop_Bridge Configuration Summary:")
message(STATUS "  Platform: ${PLATFORM}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  JSI Integration: ${ENABLE_JSI}")
message(STATUS "  Vulkan Support: ${ENABLE_VULKAN}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "  Build Docs: ${BUILD_DOCS}")
message(STATUS "")
