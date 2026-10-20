#include <cstdio>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

#include "gateway.h"

gateway_status gateway::initialize(const char *token) noexcept {
    if (token == nullptr) {
        return gateway_status::INIT_ERROR;
    }

    std::strncpy(this->token, token, sizeof(this->token) - 1);
    this->token[sizeof(this->token) - 1] = '\0';
    this->heartbeat = 0;
    this->is_conn = 0;
    this->is_awaiting_ack = 0;

    this->session = nullptr;
    this->conn = nullptr;
    this->request = nullptr;
    this->websocket = nullptr;

#ifdef _WIN32
    gateway_status gw_conn_status = this->_connect_win32();
    if (gw_conn_status != gateway_status::SUCCESS) {
        return gw_conn_status;
    }

    gateway_status gw_read_status = this->_read_frame();
    if (gw_read_status != gateway_status::SUCCESS) {
        this->terminate();
        return gateway_status::HANDSHAKE_ERROR;
    }

    const char *hb_interval = std::strstr(this->rec_buff, "\"heartbeat_interval\":");
    if (hb_interval == nullptr) {
        this->terminate();
        return gateway_status::HANDSHAKE_ERROR;
    }
    std::sscanf(hb_interval, "\"heartbeat_interval\":%u", &this->heartbeat);

    gateway_status gw_send_status = this->_send_identify();
    if (gw_send_status != gateway_status::SUCCESS) {
        this->terminate();
        return gateway_status::HANDSHAKE_ERROR;
    }

    this->is_conn = 1;
    this->last_heartbeat = std::chrono::steady_clock::now();
    
    return gateway_status::SUCCESS;
#else
    return this->_connect_linux();
#endif
}

gateway_status gateway::update(void) noexcept {
    if (this->websocket == nullptr) {
        return gateway_status::DISCONNECTED;
    }

    if (this->heartbeat > 0) {

        auto clock_now = std::chrono::steady_clock::now();
        auto clock_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock_now - this->last_heartbeat).count();

        if (clock_elapsed >= this->heartbeat) {
            gateway_status gw_send_hb_status = this->_send_heartbeat();
            if (gw_send_hb_status != gateway_status::SUCCESS) {
                this->is_conn = 0;
                return gw_send_hb_status;
            }
            this->last_heartbeat = clock_now;
            this->is_awaiting_ack = 1;
        }

        if (this->is_awaiting_ack) {
            gateway_status gw_read_status = this->_read_frame();
            if (gw_read_status == gateway_status::SUCCESS) {
                if (std::strstr(this->rec_buff, "\"op\":11") != nullptr) {
                    this->is_awaiting_ack = 0;
                }
            }
        }
    }

    return gateway_status::SUCCESS;

//     HINTERNET websocket_handle = static_cast<HINTERNET>(this->websocket);
//     WINHTTP_WEB_SOCKET_BUFFER_TYPE type;
//     DWORD status, size = 0;
//     status = WinHttpWebSocketReceive(websocket_handle, this->rec_buff, sizeof(this->rec_buff) - 1, &size, &type);
//     if (status == ERROR_SUCCESS && size > 0) {
//         this->rec_buff[size] = '\0';

//         std::cout << "DISCORD GATEWAY: " << this->rec_buff << std::endl;

//         if (std::strstr(this->rec_buff, "\"op\":10") != nullptr || std::strstr(this->rec_buff, "\"op\": 10") != nullptr) {
//             std::cout << this->rec_buff << std::endl;
//             const char *interval = std::strstr(this->rec_buff, "\"heartbeat_interval\":");
//             if (interval != nullptr) {
//                 std::sscanf(interval, "\"heartbeat_interval\":%u", &this->heartbeat);
//                 std::cout << "HOR_GATEWAY: Hello received. Heartbeat interval: " << this->heartbeat << "ms." << std::endl;
//                 this->_send_identify();
//             } 
//         }
//     }
// #else

// #endif
//     return gateway_status::SUCCESS;
}

void gateway::terminate(void) noexcept {
    this->is_conn = 0;
#ifdef _WIN32
    if (this->websocket) {
        WinHttpWebSocketClose(static_cast<HINTERNET>(this->websocket), WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
        WinHttpCloseHandle(static_cast<HINTERNET>(this->websocket));
        this->websocket = nullptr;
    }
    if (this->request) {
        WinHttpCloseHandle(static_cast<HINTERNET>(this->request));
        this->request = nullptr;
    }
    if (this->conn) {
        WinHttpCloseHandle(static_cast<HINTERNET>(this->conn));
        this->conn = nullptr;
    }
    if (this->session) {
        WinHttpCloseHandle(static_cast<HINTERNET>(this->session));
        this->session = nullptr;
    }
#endif
}

gateway_status gateway::_send_identify(void) noexcept {
    char data[512]; // i need to count it better later
    std::snprintf(data, sizeof(data), "{\"op\":2,\"d\":{\"token\":\"%s\",\"intents\":0,\"properties\":{\"os\":\"windows\",\"browser\":\"HOR\",\"device\":\"HOR\"}}}", this->token);
#ifdef _WIN32
    return this->_send_frame_win32(data);
#else
     return gateway_status::SUCCESS; // add linux and fix it
#endif
}

gateway_status gateway::_send_heartbeat(void) noexcept {
    const char *data = "{\"op\":1,\"d\":null}";
#ifdef _WIN32
    return this->_send_frame_win32(data);
#else
    return gateway_status::SUCCESS; // unix
#endif
}

gateway_status gateway::_read_frame(void) noexcept {
#ifdef _WIN32
    if (this->websocket == nullptr) {
        return gateway_status::DISCONNECTED;
    }

    WINHTTP_WEB_SOCKET_BUFFER_TYPE type;
    DWORD size = 0;
    DWORD win_rec_status = WinHttpWebSocketReceive(static_cast<HINTERNET>(this->websocket), this->rec_buff, sizeof(this->rec_buff) - 1, &size, &type);

    if (win_rec_status == ERROR_SUCCESS && size > 0) {
        this->rec_buff[size] = '\0';
        return gateway_status::SUCCESS;
    }

    return gateway_status::DISCONNECTED;
#else
    return gateway_status::SUCCESS;
#endif
}

#ifdef _WIN32
gateway_status gateway::_connect_win32(void) noexcept {
    this->session = WinHttpOpen(L"DiscordBot/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!this->session) {
        std::cout << "conn session error" << std::endl;
        return gateway_status::INIT_ERROR;
    }

    WinHttpSetTimeouts(static_cast<HINTERNET>(this->session), 0, 10000, 10000, 10);

    this->conn = WinHttpConnect(static_cast<HINTERNET>(this->session), L"gateway.discord.gg", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!this->conn) {
        std::cout << "conn error" << std::endl;
        WinHttpCloseHandle(static_cast<HINTERNET>(this->session));
        return gateway_status::CONNECT_ERROR;
    }

    this->request = WinHttpOpenRequest(static_cast<HINTERNET>(this->conn), L"GET", L"/?v=10&encoding=json", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!this->request) {
        std::cout << "req error" << std::endl;
        WinHttpCloseHandle(static_cast<HINTERNET>(this->conn));
        WinHttpCloseHandle(static_cast<HINTERNET>(this->session));
        return gateway_status::CONNECT_ERROR;
    }

    if (!WinHttpSetOption(static_cast<HINTERNET>(this->request), WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0)) {
        std::cout << "HOR_GATEWAY: WinHttpSetOption failed: " << GetLastError() << std::endl;
        WinHttpCloseHandle(static_cast<HINTERNET>(this->request));
        WinHttpCloseHandle(static_cast<HINTERNET>(this->conn));
        WinHttpCloseHandle(static_cast<HINTERNET>(this->session));
        return gateway_status::CONNECT_ERROR;
    }

    if (!WinHttpSendRequest(static_cast<HINTERNET>(this->request), WINHTTP_NO_ADDITIONAL_HEADERS, 0, nullptr, 0, 0, 0)) {
        std::cout << "HOR_GATEWAY: WinHttpSendRequest failed: " << GetLastError() << std::endl;
        WinHttpCloseHandle(static_cast<HINTERNET>(this->request));
        WinHttpCloseHandle(static_cast<HINTERNET>(this->conn));
        WinHttpCloseHandle(static_cast<HINTERNET>(this->session));
        return gateway_status::CONNECT_ERROR;
    }
    
    if (!WinHttpReceiveResponse(static_cast<HINTERNET>(this->request), nullptr)) {
        std::cout << "rec res error: " << GetLastError() << std::endl;
        WinHttpCloseHandle(static_cast<HINTERNET>(this->request));
        WinHttpCloseHandle(static_cast<HINTERNET>(this->conn));
        WinHttpCloseHandle(static_cast<HINTERNET>(this->session));
        return gateway_status::CONNECT_ERROR;
    }

    DWORD status_code = 0;
    DWORD status_size = sizeof(status_code);
    WinHttpQueryHeaders(static_cast<HINTERNET>(this->request), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &status_size, WINHTTP_NO_HEADER_INDEX);
    
    if (status_code != 101) {
        std::cout << "HOR_GATEWAY: Expected HTTP 101, but got HTTP " << status_code << std::endl;
        WinHttpCloseHandle(static_cast<HINTERNET>(this->request));
        WinHttpCloseHandle(static_cast<HINTERNET>(this->conn));
        WinHttpCloseHandle(static_cast<HINTERNET>(this->session));
        return gateway_status::CONNECT_ERROR;
    }

    this->websocket = WinHttpWebSocketCompleteUpgrade(static_cast<HINTERNET>(this->request), 0);
    if (!this->websocket) {
        std::cout << "HOR_GATEWAY: WinHttpWebSocketCompleteUpgrade failed: " << GetLastError() << std::endl;
        WinHttpCloseHandle(static_cast<HINTERNET>(this->request));
        WinHttpCloseHandle(static_cast<HINTERNET>(this->conn));
        WinHttpCloseHandle(static_cast<HINTERNET>(this->session));
        return gateway_status::CONNECT_ERROR;
    }

    return gateway_status::SUCCESS;
}

gateway_status gateway::_send_frame_win32(const char *data) noexcept {
    if (!this->websocket) {
        return gateway_status::CONNECT_ERROR;
    }

    DWORD gw_wb_send_status = WinHttpWebSocketSend(static_cast<HINTERNET>(this->websocket), WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, reinterpret_cast<PVOID>(const_cast<char*>(data)), static_cast<DWORD>(std::strlen(data)));
    if (gw_wb_send_status != NO_ERROR) {
        return gateway_status::SEND_ERROR;
    }

    return gateway_status::SUCCESS;
}

#else

gateway_status gateway::_connect_linux(void) noexcept {
    return gateway_status::SUCCESS;
}

gateway_status gateway::_send_frame_linux(const char *data) noexcept {
    return gateway_status::SUCCESS;
}
#endif