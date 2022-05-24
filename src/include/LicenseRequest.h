#include <string>
#include <thread>
#include <vector>
#include "Tracing.h"

#include <curl/curl.h>

namespace CENCDecryptor {
class LicenseRequest {
public:
    using LicenseResponseCallback = std::function<void(uint32_t code, const std::string& body)>;

    LicenseRequest() = delete;

    LicenseRequest(const std::string& url,
        const std::vector<uint8_t>& body,
        LicenseResponseCallback callback)
        : _url(url)
        , _body(body)
        , _hs(nullptr)
        , _callback(callback)
    {
    }

    LicenseRequest(const std::string& url,
        const std::vector<uint8_t>& body,
        std::vector<std::string> headers,
        LicenseResponseCallback callback)
        : _url(url)
        , _body(body)
        , _hs(nullptr)
        , _callback(callback)
    {
        for (auto& header : headers) {
            _hs = curl_slist_append(_hs, header.c_str());
        }
    }

    LicenseRequest(LicenseRequest&) = default;
    LicenseRequest& operator=(const LicenseRequest&) = default;
    LicenseRequest(LicenseRequest&&) = default;
    LicenseRequest& operator=(LicenseRequest&&) = default;

    void Submit()
    {
        _requestThread = std::thread([&]() {
            _curlHandle = curl_easy_init();
            if (_curlHandle) {
                std::string outputBuffer;
                curl_easy_setopt(_curlHandle, CURLOPT_URL, _url.c_str());
                curl_easy_setopt(_curlHandle, CURLOPT_POSTFIELDS, _body.data());
                curl_easy_setopt(_curlHandle, CURLOPT_POSTFIELDSIZE, _body.size());
                curl_easy_setopt(_curlHandle, CURLOPT_HTTPHEADER, _hs);
                curl_easy_setopt(_curlHandle, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
                curl_easy_setopt(_curlHandle, CURLOPT_WRITEDATA, &outputBuffer);
                curl_easy_perform(_curlHandle);

                long responseCode = 0;
                curl_easy_getinfo(_curlHandle, CURLINFO_RESPONSE_CODE, &responseCode);

                _callback(responseCode, outputBuffer);

                curl_easy_cleanup(_curlHandle);
            } else {
                Trace::error("Could not initialize libcurl.");
            }
        });
    }

    void Revoke()
    {
    }

    ~LicenseRequest()
    {
        _requestThread.join();
    };

private:
    static size_t CurlWriteCallback(char* contents, size_t size, size_t nmemb, void* userp)
    {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    std::string _url;
    std::vector<uint8_t> _body;
    curl_slist* _hs;
    LicenseResponseCallback _callback;

    std::thread _requestThread;
    CURL* _curlHandle;
};
}

