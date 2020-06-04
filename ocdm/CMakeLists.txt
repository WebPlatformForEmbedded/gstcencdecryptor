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


find_package(ocdm REQUIRED)
find_package(${NAMESPACE}Core REQUIRED)

set(GST_CENC_PLUGIN_IMPLEMENTATION gstcencdecrypt_ocdm)

add_library(${GST_CENC_PLUGIN_IMPLEMENTATION} STATIC "")

target_include_directories(${GST_CENC_PLUGIN_IMPLEMENTATION}
    PRIVATE
        ${GST_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_sources(${GST_CENC_PLUGIN_IMPLEMENTATION} 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Decryptor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Exchanger.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ResponseCallback.cpp)


target_link_libraries(${GST_CENC_PLUGIN_IMPLEMENTATION}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ocdm::ocdm)
