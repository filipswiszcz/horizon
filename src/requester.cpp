#include "requester.h"

requester_status requester::send(const char *url, const char *headers, const char *data, const u32 data_size) noexcept {
    if (url == nullptr || headers == nullptr || data == nullptr || data_size == 0) {
        return requester_status::INVALID_ARG;
    }

#ifdef _WIN32
    return _send_win32(url, headers, data, data_size);
#else
    return _send_linux(url, headers, data, data_size);
#endif
}

#ifdef _WIN32

#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

requester_status requester::_send_win32(const char *url, const char *headers, const char *data, const u32 data_size) noexcept {
    if (url == nullptr || headers == nullptr || data == nullptr || data_size == 0) {
        return requester_status::INVALID_ARG;
    }

    wchar_t url_wide[2048]{}, headers_wide[256]{};
    const i32 url_len = MultiByteToWideChar(CP_UTF8, 0, url, -1, url_wide, static_cast<i32>(sizeof(url_wide) / sizeof(url_wide[0])));
    if (url_len == 0) {
        return requester_status::INVALID_ARG;
    }

    const i32 headers_len = MultiByteToWideChar(CP_UTF8, 0, headers, -1, headers_wide, static_cast<i32>(sizeof(headers_wide) / sizeof(headers_wide[0])));
    if (headers_len == 0) {
        return requester_status::INVALID_ARG;
    }

    wchar_t host[512]{}, url_path[2048]{};
    URL_COMPONENTS url_parts{};
    url_parts.dwStructSize = sizeof(url_parts);
    url_parts.lpszHostName = host;
    url_parts.dwHostNameLength = static_cast<DWORD>(sizeof(host) / sizeof(host[0]));
    url_parts.lpszUrlPath = url_path;
    url_parts.dwUrlPathLength = static_cast<DWORD>(sizeof(url_path) / sizeof(url_path[0]));

    if (!WinHttpCrackUrl(url_wide, 0, 0, &url_parts)) {
        return requester_status::INVALID_ARG;
    }

    if (url_parts.nScheme != INTERNET_SCHEME_HTTPS) {
        return requester_status::INVALID_ARG;
    }

    HINTERNET session = WinHttpOpen(L"HOR/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == nullptr) {
        return requester_status::INIT_ERROR;
    }

    HINTERNET connection = WinHttpConnect(session, host, url_parts.nPort, 0);
    if (connection == nullptr) {
        (void) WinHttpCloseHandle(session);
        return requester_status::INIT_ERROR;
    }

    HINTERNET request = WinHttpOpenRequest(connection, L"POST", url_path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (request == nullptr) {
        (void) WinHttpCloseHandle(connection);
        (void) WinHttpCloseHandle(session);
        return requester_status::INIT_ERROR;
    }

    const BOOL is_sent = WinHttpSendRequest(request, headers_wide, static_cast<DWORD>(-1L), const_cast<char*>(data), data_size, data_size, 0);
    if (!is_sent || !WinHttpReceiveResponse(request, nullptr)) {
        (void) WinHttpCloseHandle(request);
        (void) WinHttpCloseHandle(connection);
        (void) WinHttpCloseHandle(session);
        return requester_status::HTTP_ERROR;
    }

    DWORD status_code = 0, status_size = sizeof(status_code);
    const BOOL has_status = WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &status_size, WINHTTP_NO_HEADER_INDEX);

    (void) WinHttpCloseHandle(request);
    (void) WinHttpCloseHandle(connection);
    (void) WinHttpCloseHandle(session);

    if (!has_status || status_code < 200 || status_code >= 300) {
        return requester_status::HTTP_ERROR;
    }

    return requester_status::SUCCESS;
}

#else

#include <curl/curl.h>

requester_status requester::_send_linux(const char *url, const char *headers, const char *data, const u32 data_size) noexcept {
    if (url == nullptr || headers == nullptr || data == nullptr || data_size == 0) {
        return requester_status::INVALID_ARG;
    }

    if (std::strncmp(url, "https://", 8) != 0) {
        return requester_status::INVALID_ARG;
    }

    CURL *session = curl_easy_init();
    if (session == nullptr) {
        return requester_status::INIT_ERROR;
    }

    curl_slist *headers_formatted = curl_slist_append(nullptr, headers);
    if (headers_formatted == nullptr) {
        curl_easy_cleanup(session);
        return requester_status::INIT_ERROR;
    }

    using write_fn_t = size_t(*) (char*, size_t, size_t, void*);
    const write_fn_t discard_body = +[](char*, size_t elem_size, size_t elem_count, void*) noexcept -> size_t {
        return elem_size * elem_count;
    };

    CURLcode result = curl_easy_setopt(session, CURLOPT_URL, url);
    if (result == CURLE_OK) {
        result = curl_easy_setopt(session, CURLOPT_USERAGENT, "HOR/1.0");
    }
    if (result == CURLE_OK) {
        result = curl_easy_setopt(session, CURLOPT_HTTPHEADER, headers_formatted);
    }
    if (result == CURLE_OK) {
        result = curl_easy_setopt(session, CURLOPT_POST, 1);
    }
    if (result == CURLE_OK) {
        result = curl_easy_setopt(session, CURLOPT_POSTFIELDS, data);
    }
    if (result == CURLE_OK) {
        result = curl_easy_setopt(session, CURLOPT_POSTFIELDSIZE, static_cast<long>(data_size));
    }
    if (result == CURLE_OK) {
        result = curl_easy_setopt(session, CURLOPT_PROTOCOLS_STR, "https");
    }
    if (result == CURLE_OK) {
        result = curl_easy_setopt(session, CURLOPT_WRITEFUNCTION, discard_body);
    }
    if (result == CURLE_OK) {
        result = curl_easy_setopt(session, CURLOPT_NOSIGNAL, 1);
    }
    if (result == CURLE_OK) {
        result = curl_easy_setopt(session, CURLOPT_TIMEOUT, 30);
    }

    if (result != CURLE_OK) {
        curl_slist_free_all(headers_formatted);
        curl_easy_cleanup(session);
        return requester_status::INIT_ERROR;
    }

    result = curl_easy_perform(session);
    long status_code = 0;
    if (result == CURLE_OK) {
        result = curl_easy_getinfo(session, CURLINFO_RESPONSE_CODE, &status_code);
    }

    curl_slist_free_all(headers_formatted);
    curl_easy_cleanup(session);

    if (result != CURL_OK || status_code < 200 || status_code >= 300) {
        return requester_status::HTTP_ERROR;
    }

    return requester_status::SUCCESS;
}
#endif