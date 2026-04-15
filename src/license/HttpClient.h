#pragma once
#include <string>
#include <Windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

namespace Http
{
    struct Response
    {
        int         statusCode = 0;
        std::string body;
        bool        success    = false;
    };

    // Parse URL into host and path
    inline bool ParseUrl(const std::string& url, std::wstring& host, std::wstring& path, bool& useSSL)
    {
        useSSL = (url.find("https://") == 0);
        size_t start = url.find("://");
        if (start == std::string::npos) return false;
        start += 3;

        size_t pathStart = url.find('/', start);
        std::string hostStr = (pathStart == std::string::npos) ? url.substr(start) : url.substr(start, pathStart - start);
        std::string pathStr = (pathStart == std::string::npos) ? "/" : url.substr(pathStart);

        host.assign(hostStr.begin(), hostStr.end());
        path.assign(pathStr.begin(), pathStr.end());
        return true;
    }

    // Send HTTP request (POST or GET)
    inline Response Request(const std::string& method, const std::string& url,
                            const std::string& jsonBody = "",
                            const std::string& authToken = "")
    {
        Response resp;
        std::wstring host, path;
        bool useSSL = true;

        if (!ParseUrl(url, host, path, useSSL))
            return resp;

        HINTERNET hSession = WinHttpOpen(L"ShiftHub/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return resp;

        INTERNET_PORT port = useSSL ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return resp; }

        std::wstring wMethod(method.begin(), method.end());
        DWORD flags = useSSL ? WINHTTP_FLAG_SECURE : 0;

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, wMethod.c_str(), path.c_str(),
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return resp; }

        // Headers
        std::wstring headers = L"Content-Type: application/json\r\n";
        if (!authToken.empty())
        {
            std::string authHeader = "Authorization: Bearer " + authToken + "\r\n";
            std::wstring wAuth(authHeader.begin(), authHeader.end());
            headers += wAuth;
        }

        WinHttpAddRequestHeaders(hRequest, headers.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

        // Send
        BOOL bResult = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            jsonBody.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)jsonBody.c_str(),
            (DWORD)jsonBody.size(), (DWORD)jsonBody.size(), 0);

        if (!bResult)
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return resp;
        }

        WinHttpReceiveResponse(hRequest, NULL);

        // Status code
        DWORD dwStatusCode = 0, dwSize = sizeof(dwStatusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
        resp.statusCode = (int)dwStatusCode;

        // Read body
        DWORD dwBytesAvailable = 0;
        do
        {
            WinHttpQueryDataAvailable(hRequest, &dwBytesAvailable);
            if (dwBytesAvailable == 0) break;

            std::vector<char> buf(dwBytesAvailable + 1, 0);
            DWORD dwBytesRead = 0;
            WinHttpReadData(hRequest, buf.data(), dwBytesAvailable, &dwBytesRead);
            resp.body.append(buf.data(), dwBytesRead);
        } while (dwBytesAvailable > 0);

        resp.success = (resp.statusCode >= 200 && resp.statusCode < 300);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return resp;
    }

    inline Response Post(const std::string& url, const std::string& json, const std::string& token = "")
    {
        return Request("POST", url, json, token);
    }

    inline Response Get(const std::string& url, const std::string& token = "")
    {
        return Request("GET", url, "", token);
    }
}
