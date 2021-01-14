#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

get_filename_component(CHIP_ROOT ../../../../modules/lib/connectedhomeip REALPATH)
get_filename_component(COMMON_ROOT ../common REALPATH)

list(APPEND ZEPHYR_EXTRA_MODULES ${CHIP_ROOT}/config/nrfconnect/chip-module)
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(chip-light-switch)

target_include_directories(app PRIVATE src ${COMMON_ROOT}/src)

target_compile_options(app PRIVATE -Wno-deprecated-declarations)

target_sources(app PRIVATE
               src/app_task.cpp
               src/light_switch.cpp
               src/main.cpp
               ${COMMON_ROOT}/src/led_widget.cpp
               ${COMMON_ROOT}/src/thread_util.cpp)
