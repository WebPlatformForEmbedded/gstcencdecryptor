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

namespace WPEFramework {
namespace CENCDecryptor {
    namespace OCDM {

        Exchanger::Exchanger()
            : _reqTrigger(false, true)
            , _resReceived(false, true)
            , _challenger(*this, _resReceived)
            , _queue(5) // TODO: magic constant
            , _consumer(_challenger, _queue)
        {
        }

        Core::ProxyType<Web::Response> Exchanger::Element()
        {
            return Core::ProxyType<Web::Response>::Create();
        }

        uint32_t Exchanger::Submit(Core::ProxyType<Web::Request> request,
            Core::ProxyType<IExchange::ICallback> onResponse,
            uint32_t waitTime)
        {
            bool isOverfilled = _queue.Post(LicenseRequest({ request, onResponse, waitTime }));
            return isOverfilled ? Core::ERROR_UNAVAILABLE : Core::ERROR_NONE;
        }

        uint32_t Exchanger::Revoke(Core::ProxyType<IExchange::ICallback> onResponse)
        {
            // TODO:
            return Core::ERROR_UNAVAILABLE;
        }

        Exchanger::Challenger::Challenger(Exchanger& parent, Core::Event& resReceived)
            : Exchanger::Challenger::WebLinkClass(2,
                parent,
                false,
                Core::NodeId(),
                Core::NodeId(),
                2048,
                2048)
            , _resReceived(resReceived)
        {
        }

        void Exchanger::Challenger::Send(const Core::ProxyType<Web::Request>& request, uint32_t timeout)
        {
            _response.Destroy();
            _request = request;

            Core::NodeId remoteNode(_request->Host.Value().c_str(), 80, Core::NodeId::TYPE_IPV4);
            if (remoteNode.IsValid() == false) {
                TRACE_L1("Connection to %s unavailable", _request->Host.Value().c_str());
            } else {

                Link().RemoteNode(remoteNode);
                Link().LocalNode(remoteNode.AnyInterface());
                uint32_t result = Open(0);

                if (result != Core::ERROR_NONE) {
                    _resReceived.Lock(timeout);
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
            element->Body<Web::TextBody>(Core::ProxyType<Web::TextBody>::Create());
        }

        void Exchanger::Challenger::Received(Core::ProxyType<Web::Response>& res)
        {
            _response = res;
            _resReceived.SetEvent();
        }

        void Exchanger::Challenger::Send(const Core::ProxyType<Web::Request>& req)
        {
            ASSERT(req == _request);
        }

        void Exchanger::Challenger::StateChange()
        {
            if (IsOpen()) {
                Submit(_request);
            }
        }

        Exchanger::QueueWorker::QueueWorker(Challenger& challenger, Core::QueueType<LicenseRequest>& queue)
            : _challenger(challenger)
            , _queue(queue)
        {
            this->Run();
        }

        uint32_t Exchanger::QueueWorker::Worker()
        {
            LicenseRequest requestData;
            bool extracted = _queue.Extract(requestData, Core::infinite);

            if (extracted) {
                _challenger.Send(requestData.licenseRequest, requestData.timeout);

                requestData.licenseHandler->Response(requestData.licenseRequest,
                    _challenger.Response());
            }

            return Core::ERROR_NONE;
        }

        Exchanger::QueueWorker::~QueueWorker()
        {
            this->Stop();
            TRACE_L1("Stopped queue worker");
        }
    }
}
}