cmake_minimum_required(VERSION 3.16)

# The actual application is the XiaoZhi project kept under main/. Expose its
# main component and managed components here so ESP-IDF builds from the outer
# workspace use the same source tree as xiaozhi.ps1.
set(XIAOZHI_PROJECT_DIR "${CMAKE_CURRENT_LIST_DIR}/main/xiaozhi-esp32-2.2.4")
file(GLOB IDF_COMPONENT_DIRS "$ENV{IDF_PATH}/components/*")
file(GLOB XIAOZHI_MANAGED_COMPONENT_DIRS "${XIAOZHI_PROJECT_DIR}/managed_components/*")
set(COMPONENT_DIRS
    "${XIAOZHI_PROJECT_DIR}/main"
    ${IDF_COMPONENT_DIRS}
    ${XIAOZHI_MANAGED_COMPONENT_DIRS}
)
set(SDKCONFIG "${XIAOZHI_PROJECT_DIR}/sdkconfig" CACHE FILEPATH "XiaoZhi sdkconfig")
set(PROJECT_VER "2.2.4")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xiaozhi)
