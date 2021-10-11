# Copyright RDK Management
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation, version 2
# of the license.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
# Boston, MA 02110-1301, USA.

find_package(WPEFramework REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)
find_package(CURL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST 
    REQUIRED 
        gstreamer-1.0>=1.4
        gstreamer-sdp-1.0>=1.4
        gstreamer-video-1.0>=1.4
        gstreamer-app-1.0>=1.4
        gstreamer-base-1.0>=1.4)

pkg_check_modules(PC_OCDM ocdm)
if(PC_OCDM_FOUND)
    set(OCDM_INCLUDE_DIRS ${PC_OCDM_INCLUDE_DIRS})
    set(OCDM_LIBRARIES ${PC_OCDM_LIBRARIES})
    set(OCDM_LIBRARY_DIRS ${PC_OCDM_LIBRARY_DIRS})
else()
    message(FATAL_ERROR)
endif()

add_library(${PROJECT_NAME} SHARED "")

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${GST_INCLUDE_DIRS}
        ${OCDM_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS})

target_sources(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/GstCencDecrypt.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Decryptor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Module.cpp)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${GST_LIBRARIES}
        ${OCDM_LIBRARIES}
        ${CURL_LIBRARIES})

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/gstreamer-1.0)
