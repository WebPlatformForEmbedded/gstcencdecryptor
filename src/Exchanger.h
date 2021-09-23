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

#include "IExchange.h"
#include <plugins/System.h>

namespace WPEFramework {
namespace CENCDecryptor {
    namespace OCDM {

        class Exchanger : public IExchange {
        public:
            Exchanger();
            Exchanger(const Exchanger&) = delete;
            Exchanger& operator=(const Exchanger&) = delete;

            ~Exchanger() override { _queue.Disable(); };

            uint32_t Submit(Core::ProxyType<Web::Request>,
                Core::ProxyType<IExchange::ICallback>, uint32_t waitTime) override;

            uint32_t Revoke(Core::ProxyType<IExchange::ICallback>) override;
            Core::ProxyType<Web::Response> Element();

        private:
            class Challenger : public Web::WebLinkType<
                                   Core::SocketStream,
                                   Web::Response,
                                   Web::Request,
                                   Exchanger&> {

                using WebLinkClass = Web::WebLinkType<
                    Core::SocketStream,
                    Web::Response,
                    Web::Request,
                    Exchanger&>;

            public:
                Challenger() = delete;
                explicit Challenger(Exchanger& parent, Core::Event& resReceived);

                void Send(const Core::ProxyType<Web::Request>&, uint32_t timeout);
                Core::ProxyType<Web::Response> Response();

            private:
                void LinkBody(Core::ProxyType<Web::Response>& element) override;
                void Received(Core::ProxyType<Web::Response>& text) override;
                void Send(const Core::ProxyType<Web::Request>& text) override;
                void StateChange() override;

                Core::Event& _resReceived;
                Core::ProxyType<Web::Response> _response;
                Core::ProxyType<Web::Request> _request;
            };

            struct LicenseRequest {
            public:
                Core::ProxyType<Web::Request> licenseRequest;
                Core::ProxyType<IExchange::ICallback> licenseHandler;
                uint32_t timeout;
            };

            class QueueWorker : public Core::Thread {
            public:
                QueueWorker() = delete;
                QueueWorker& operator=(const QueueWorker&) = delete;
                QueueWorker(const QueueWorker&) = delete;

                QueueWorker(Challenger&, Core::QueueType<LicenseRequest>&);
                ~QueueWorker();

                uint32_t Worker() override;

            private:
                Challenger& _challenger;
                Core::QueueType<LicenseRequest>& _queue;
            };

        private:
            Core::Event _reqTrigger;
            Core::Event _resReceived;
            Challenger _challenger;
            Core::QueueType<LicenseRequest> _queue;
            QueueWorker _consumer;
        };
    }

    std::unique_ptr<CENCDecryptor::IExchange> CENCDecryptor::IExchange::Create()
    {
        return std::unique_ptr<CENCDecryptor::IExchange>(new CENCDecryptor::OCDM::Exchanger());
    }
}
}
