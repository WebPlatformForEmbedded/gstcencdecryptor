/*
 * Copyright RDK Management
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation, version 2
 * of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#pragma once

#include "Constants.h"
#include <iostream>
#include <iomanip>

namespace CENCDecryptor {

class Trace {
public:
    template <typename... Args>
    static void log(Args... args)
    {
        std::stringstream ss;
        append(ss, args...);
        std::cout << Constants::LogPrefix << ss.str() << "\n";
    }

    template <typename... Args>
    static void error(Args... args)
    {
        std::stringstream ss;
        append(ss, args...);
        std::cerr << Constants::ErrorPrefix << ss.str() << "\n";
    }

    static std::string uint8_to_string(const uint8_t* array, const uint32_t size)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (int index = 0; index < size; ++index) {
            ss << std::hex << std::setw(2) << static_cast<int>(array[index]);
        }
        return ss.str();
    }

private:
    template <typename T>
    static void append(std::stringstream& output, T first)
    {
        output << first;
    }

    template <typename T, typename... Args>
    static void append(std::stringstream& output, T first, Args... args)
    {
        output << first;
        append(output, args...);
    }
};
}
