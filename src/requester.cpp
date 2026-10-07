#include "requester.h"

requester_status requester::send(const char *url, const char *data, const u32 data_size) noexcept {
    if (url == nullptr || data == nullptr || data_size == 0) {
        return requester_status::INVALID_ARG;
    }

#ifdef _WIN32
    return handle_win32(url, data, data_size);
#else
    return handle_linux(url, data, data_size);
#endif
}

#ifdef _WIN32

#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

requester_status requester::handle_win32(const char *url, const char *data, const u32 data_size) noexcept {
    wchar_t wide_url[2048]{};
    const i32 conv_url = MultiByteToWideChar(CP_UTF8, 0, url, -1, wide_url, static_cast<i32>(sizeof(wide_url) / sizeof(wide_url[0])));
    if (conv_url == 0) {
        return requester_status::INVALID_ARG;
    }

    wchar_t host[512]{}, path[2048]{};
    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    components.lpszHostName = host;
    components.dwHostNameLength = static_cast<DWORD>(sizeof(host) / sizeof(host[0]));
    components.lpszUrlPath = path;
    components.dwUrlPathLength = static_cast<DWORD>(sizeof(path) / sizeof(path[0]));

    if (!WinHttpCrackUrl(wide_url, 0, 0, &components)) {
        return requester_status::INVALID_ARG;
    }

    if (components.nScheme != INTERNET_SCHEME_HTTPS) {
        return requester_status::INVALID_ARG;
    }

    HINTERNET session = WinHttpOpen(L"HOR/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == nullptr) {
        return requester_status::INIT_ERROR;
    }

    HINTERNET connection = WinHttpConnect(session, host, components.nPort, 0);
    if (connection == nullptr) {
        WinHttpCloseHandle(session);
        return requester_status::INIT_ERROR;
    }

    HINTERNET open_request = WinHttpOpenRequest(connection, L"POST", path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (open_request == nullptr) {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return requester_status::INIT_ERROR;
    }

    const wchar_t headers[] = L"Content-Type: application/json\r\n";
    const BOOL sent_request = WinHttpSendRequest(open_request, headers, static_cast<DWORD>(-1L), const_cast<char*>(data), data_size, data_size, 0);
    if (!sent_request || !WinHttpReceiveResponse(open_request, nullptr)) {
        WinHttpCloseHandle(open_request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return requester_status::HTTP_ERROR;
    }

    DWORD status_code = 0, status_size = sizeof(status_code);
    if (!WinHttpQueryHeaders(open_request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &status_size, WINHTTP_NO_HEADER_INDEX)) {
        WinHttpCloseHandle(open_request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return requester_status::HTTP_ERROR;
    }

    WinHttpCloseHandle(open_request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    if (status_code < 200 || status_code >= 300) {
        return requester_status::HTTP_ERROR;
    }

    return requester_status::SUCCESS;
}
#endif

requester_status requester::handle_linux(const char *url, const char *data, const u32 data_size) noexcept {
    return requester_status::SUCCESS;
}