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

namespace WPEFramework {
namespace CENCDecryptor {

    class Exchanger : public IExchange {
    public:
        explicit Exchanger(const std::string& url);
        Exchanger() = delete;
        Exchanger(const Exchanger&) = delete;
        Exchanger& operator=(const Exchanger&) = delete;

        ~Exchanger() override
        {
            // _challenger.
        };

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

            void Send(const Core::ProxyType<Web::Request>&, const Core::URL&);
            Core::ProxyType<Web::Response> Response();

        private:
            void LinkBody(Core::ProxyType<Web::Response>& element) override;
            void Received(Core::ProxyType<Web::Response>& text) override;
            void Send(const Core::ProxyType<Web::Request>& text) override;
            void StateChange() override;

            Core::Event& _resReceived;
            Core::ProxyType<Web::Response> _response;
            Core::ProxyType<Web::Request> _request;
            static Core::ProxyPoolType<Web::TextBody> _bodyFactory;
        };

        struct LicenseRequestData {
            Core::ProxyType<Web::Request> licenseRequest;
            Core::ProxyType<IExchange::ICallback> licenseHandler;
            uint32_t timeout;
            Core::URL url;
        };

        class Overwatch : public Core::IDispatch {
        public:
            Overwatch() = delete;
            Overwatch(Challenger& challenger,
                const LicenseRequestData& reqData);

            void Dispatch() override;

        private:
            Challenger& _challenger;
            LicenseRequestData _requestData;
        };

    private:
        Core::URL _url;
        Core::Event _reqTrigger;
        Core::Event _resReceived;
        Challenger _challenger;
        LicenseRequestData _requestData;
        Core::ProxyType<Core::IDispatch> _reqOverwatch;
    };
}
}
