cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/core/system_manager"
    "${CMAKE_CURRENT_LIST_DIR}/core/app_framework"
    "${CMAKE_CURRENT_LIST_DIR}/core/event_hub"
    "${CMAKE_CURRENT_LIST_DIR}/products/speaker"
    "${CMAKE_CURRENT_LIST_DIR}/products/speaker/apps/home"
    "${CMAKE_CURRENT_LIST_DIR}/services/display"
    "${CMAKE_CURRENT_LIST_DIR}/services/audio"
    "${CMAKE_CURRENT_LIST_DIR}/services/input"
    "${CMAKE_CURRENT_LIST_DIR}/services/sensor"
    "${CMAKE_CURRENT_LIST_DIR}/services/storage"
    "${CMAKE_CURRENT_LIST_DIR}/ports/display_port"
    "${CMAKE_CURRENT_LIST_DIR}/ports/lvgl_port"
    "${CMAKE_CURRENT_LIST_DIR}/drivers/bus/spi_master"
    "${CMAKE_CURRENT_LIST_DIR}/drivers/display/st7305"
    "${CMAKE_CURRENT_LIST_DIR}/bsp/waveshare_esp32_s3_rlcd_4_2"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(RLCD-Esp32-Brookesia)
