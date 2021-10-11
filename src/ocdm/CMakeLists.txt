find_package(WPEFramework REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)

pkg_check_modules(PC_OCDM ocdm)
if(PC_OCDM_FOUND)
    set(OCDM_INCLUDE_DIRS ${PC_OCDM_INCLUDE_DIRS})
    set(OCDM_LIBRARIES ${PC_OCDM_LIBRARIES})
    set(OCDM_LIBRARY_DIRS ${PC_OCDM_LIBRARY_DIRS})
else()
    message(FATAL_ERROR)
endif()

add_library(${IMPLEMENTATION_NAME} STATIC "")

target_include_directories(${IMPLEMENTATION_NAME}
    PUBLIC
        ${COMMON_INCLUDES}
        ${OCDM_INCLUDE_DIRS})

target_sources(${IMPLEMENTATION_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/OpenCDMDecryptor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Module.cpp)
        

target_link_libraries(${IMPLEMENTATION_NAME}
    PUBLIC
        ${COMMON_LIBRARIES}
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${OCDM_LIBRARIES})
