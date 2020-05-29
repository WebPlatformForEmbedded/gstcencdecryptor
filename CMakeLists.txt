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

set(GST_CENC_PLUGIN_NAME gstdashdecrypt)

find_package(PkgConfig REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)

pkg_check_modules(GST 
    REQUIRED 
        gstreamer-1.0>=1.4
        gstreamer-sdp-1.0>=1.4
        gstreamer-video-1.0>=1.4
        gstreamer-app-1.0>=1.4
        gstreamer-base-1.0>=1.4)

add_library(${GST_CENC_PLUGIN_NAME} SHARED "")

target_include_directories(${GST_CENC_PLUGIN_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${GST_INCLUDE_DIRS})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ocdm)

target_sources(${GST_CENC_PLUGIN_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/GstCencDecrypt.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/EncryptedBuffer.cpp)

target_link_libraries(${GST_CENC_PLUGIN_NAME}
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins 
        ${GST_LIBRARIES}
        gstdashdecrypt_ocdm)

install(TARGETS ${GST_CENC_PLUGIN_NAME}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/gstreamer-1.0)
     
