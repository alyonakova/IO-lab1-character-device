cmake_minimum_required(VERSION 2.6)

project("ch_drv")

#
# Find kernel headers directory
#

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(KernelHeaders REQUIRED)
include_directories(${KERNELHEADERS_INCLUDE_DIRS})

#
# Target: Kernel module file (.ko)
#

include("cmake/BuildKernelModule.cmake")
add_module("${PROJECT_NAME}" "${PROJECT_SOURCE_DIR}/src/ch_drv.c" "${KERNELHEADERS_DIR}")

#
# Dummy target for CLion
#
# This fake target is needed just to setup syntax highlighting
# and code completion in the CLion IDE. Do not build this target.
#

set(CLION_FAKE_TARGET "dummy_target_for_clion_${PROJECT_NAME}")
add_library("${CLION_FAKE_TARGET}" src/ch_drv.c)

# Do not build this target by default:
set_target_properties("${CLION_FAKE_TARGET}" PROPERTIES EXCLUDE_FROM_ALL 1)
set_target_properties("${CLION_FAKE_TARGET}" PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD 1)

# Allow resolution of kernel-level procedures and macros:
target_compile_definitions("${CLION_FAKE_TARGET}" PRIVATE __KERNEL__)
target_compile_definitions("${CLION_FAKE_TARGET}" PRIVATE MODULE)

# Keep KBUILD_MODNAME empty to not break `pr_fmt' macro:
target_compile_definitions("${CLION_FAKE_TARGET}" PRIVATE KBUILD_MODNAME=)
