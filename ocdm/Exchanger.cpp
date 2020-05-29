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

#include "Exchanger.h"
#include <interfaces/ITimeSync.h>
#include <plugins/System.h>

namespace WPEFramework {
namespace CENCDecryptor {
    namespace OCDM {

        Core::ProxyPoolType<Web::TextBody>
            Exchanger::Challenger::_bodyFactory(2);

        Exchanger::Exchanger(const std::string& url)
            : _url(url)
            , _reqTrigger(false, true)
            , _resReceived(false, true)
            , _challenger(*this, _resReceived)
            , _reqOverwatch()
        {
        }

        Core::ProxyType<Web::Response> Exchanger::Element()
        {
            return (PluginHost::Factories::Instance().Response());
        }

        uint32_t Exchanger::Submit(Core::ProxyType<Web::Request> request,
            Core::ProxyType<IExchange::ICallback> onResponse, uint32_t waitTime)
        {
            LicenseRequestData reqData({ request, onResponse, waitTime, _url });

            _reqOverwatch = Core::ProxyType<Core::IDispatch>(
                Core::ProxyType<Overwatch>::Create(_challenger, reqData));

            if (_reqOverwatch != nullptr) {
                Core::IWorkerPool::Instance().Schedule(Core::Time::Now(), _reqOverwatch);
                return Core::ERROR_INPROGRESS;
            } else {
                return Core::ERROR_UNAVAILABLE;
            }
        }

        uint32_t Exchanger::Revoke(Core::ProxyType<IExchange::ICallback> onResponse)
        {
            // TODO:
            return Core::ERROR_UNAVAILABLE;
        }

        Exchanger::Challenger::Challenger(Exchanger& parent, Core::Event& resReceived)
            : Exchanger::Challenger::WebLinkClass(2, parent, false, Core::NodeId(), Core::NodeId(), 2048, 2048)
            , _resReceived(resReceived)
        {
        }

        void Exchanger::Challenger::Send(const Core::ProxyType<Web::Request>& request, const Core::URL& url)
        {
            _request = request;
            _request->Path = '/' + url.Path().Value();
            _request->Host = url.Host().Value();

            Core::NodeId remoteNode(url.Host().Value().c_str(), 80, Core::NodeId::TYPE_IPV4);
            if (remoteNode.IsValid() == false) {
                TRACE_L1("Connection to %s unavailable", url.Host().Value().c_str());
            } else {
                Link().RemoteNode(remoteNode);
                Link().LocalNode(remoteNode.AnyInterface());
                uint32_t result = Open(0);
                if (result != Core::ERROR_NONE) {
                    _resReceived.Lock(Core::infinite);
                } else {
                    TRACE_L1("Failed to open the connection to LA server: <%d>", result);
                }
            }
        }

        Core::ProxyType<Web::Response> Exchanger::Challenger::Response()
        {
            return _response;
        }

        void Exchanger::Challenger::LinkBody(Core::ProxyType<Web::Response>& element)
        {
            element->Body<Web::TextBody>(_bodyFactory.Element());
        }

        void Exchanger::Challenger::Received(Core::ProxyType<Web::Response>& res)
        {
            _response = res;
            std::string str;
            _response->ToString(str);
            _resReceived.SetEvent();
        }

        void Exchanger::Challenger::Send(const Core::ProxyType<Web::Request>& req)
        {
            ASSERT(req == _request);
        }

        void Exchanger::Challenger::StateChange()
        {
            std::string str;
            _request->ToString(str);
            if (IsOpen()) {
                Submit(_request);
            }
        }

        Exchanger::Overwatch::Overwatch(Challenger& challenger, const LicenseRequestData& requestData)
            : _challenger(challenger)
            , _requestData(requestData)
        {
        }

        void Exchanger::Overwatch::Dispatch()
        {
            _challenger.Send(_requestData.licenseRequest, this->_requestData.url);

            this->_requestData.licenseHandler->Response(this->_requestData.licenseRequest,
                this->_challenger.Response());
        }
    }
}
}