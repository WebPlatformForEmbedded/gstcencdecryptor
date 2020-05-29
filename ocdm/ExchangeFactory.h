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

#include "Exchanger.h"
#include "IExchangeFactory.h"

namespace WPEFramework {
namespace CENCDecryptor {
    namespace OCDM {

        class ExchangeFactory : public IExchangeFactory {
        public:
            static std::unique_ptr<IExchangeFactory> Create()
            {
                return std::unique_ptr<IExchangeFactory>(new ExchangeFactory());
            };

            std::unique_ptr<IExchange> CreateExchange(const std::string& url) override
            {
                return std::unique_ptr<IExchange>(new Exchanger(url));
            };

            ~ExchangeFactory() override{};
        };

    }
}
}