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

find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST 
    REQUIRED 
        gstreamer-1.0>=1.4
        gstreamer-sdp-1.0>=1.4
        gstreamer-video-1.0>=1.4
        gstreamer-app-1.0>=1.4
        gstreamer-base-1.0>=1.4)

add_library(${PROJECT_NAME} SHARED "")

set(TARGET ${PROJECT_NAME})
set(IMPLEMENTATION_NAME ${TARGET}_implementation)
set(COMMON_INCLUDES
        ${GST_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set(COMMON_LIBRARIES 
        ${GST_LIBRARIES}
        ${CURL_LIBRARIES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${COMMON_INCLUDES})

target_sources(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/GstCencDecrypt.cpp)

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/${GSTCENCDECRYPT_IMPLEMENTATION})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${GSTCENCDECRYPT_IMPLEMENTATION})
else ()
    message(FATAL_ERROR "Invalid value \'${GSTCENCDECRYPT_IMPLEMENTATION}\' for GSTCENCDECRYPT_IMPLEMENTATION.")
endif ()

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${COMMON_LIBRARIES}
        ${IMPLEMENTATION_NAME})

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/gstreamer-1.0)
