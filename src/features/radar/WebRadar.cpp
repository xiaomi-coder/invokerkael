#include "WebRadar.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <string>

#pragma comment(lib, "ws2_32.lib")

namespace WebRadar
{
    static std::string g_sJsonPayload = "[]";
    static std::mutex g_Mutex;
    static std::thread g_ServerThread;
    static bool g_bRunning = false;
    static SOCKET g_ListenSocket = INVALID_SOCKET;

    static void ServerLoop()
    {
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) return;

        struct addrinfo* result = NULL, hints;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        iResult = getaddrinfo(NULL, "1337", &hints, &result);
        if (iResult != 0) {
            WSACleanup();
            return;
        }

        g_ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (g_ListenSocket == INVALID_SOCKET) {
            freeaddrinfo(result);
            WSACleanup();
            return;
        }

        char optval = 1;
        setsockopt(g_ListenSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        iResult = bind(g_ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        freeaddrinfo(result);

        if (iResult == SOCKET_ERROR) {
            closesocket(g_ListenSocket);
            WSACleanup();
            return;
        }

        if (listen(g_ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(g_ListenSocket);
            WSACleanup();
            return;
        }

        while (g_bRunning)
        {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(g_ListenSocket, &readfds);

            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            int sResult = select(0, &readfds, NULL, NULL, &timeout);
            if (sResult > 0 && FD_ISSET(g_ListenSocket, &readfds))
            {
                SOCKET ClientSocket = accept(g_ListenSocket, NULL, NULL);
                if (ClientSocket != INVALID_SOCKET)
                {
                    char recvbuf[512];
                    int iResult = recv(ClientSocket, recvbuf, 512, 0);
                    if (iResult > 0)
                    {
                        std::string req(recvbuf, iResult);
                        if (req.find("GET / ") != std::string::npos || req.find("GET /data") != std::string::npos || req.find("OPTIONS") != std::string::npos)
                        {
                            std::string payload;
                            {
                                std::lock_guard<std::mutex> lock(g_Mutex);
                                payload = g_sJsonPayload;
                            }

                            std::string response = 
                                "HTTP/1.1 200 OK\r\n"
                                "Access-Control-Allow-Origin: *\r\n"
                                "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
                                "Content-Type: application/json\r\n"
                                "Connection: close\r\n\r\n" +
                                payload;

                            send(ClientSocket, response.c_str(), (int)response.length(), 0);
                        }
                    }
                    closesocket(ClientSocket);
                }
            }
        }

        closesocket(g_ListenSocket);
        WSACleanup();
    }

    void Initialize()
    {
        g_bRunning = true;
        g_ServerThread = std::thread(ServerLoop);
        g_ServerThread.detach();
    }

    void Shutdown()
    {
        g_bRunning = false;
        if (g_ListenSocket != INVALID_SOCKET)
        {
            closesocket(g_ListenSocket);
            g_ListenSocket = INVALID_SOCKET;
        }
    }

    void SetEntities(const std::vector<EntityObject_t>& vecEntities)
    {
        if (!CONFIG_GET(bool, g_Variables.m_Radar.m_bWebRadar)) return;

        std::string json = "[";
        C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;

        bool first = true;
        for (const auto& ent : vecEntities)
        {
            if (!ent.m_pEntity || ent.m_eType != EEntityType::ENTITY_PLAYER) continue;

            C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(ent.m_pEntity);
            if (pPawn == pLocalPawn || pPawn->m_iHealth() <= 0) continue;
            
            CGameSceneNode* pNode = pPawn->m_pGameSceneNode();
            if (!pNode) continue;

            uint8_t localTeam = pLocalPawn ? pLocalPawn->m_iTeamNum() : 0;
            bool isEnemy = (pPawn->m_iTeamNum() != localTeam);

            Vector pos = pNode->m_vecAbsOrigin();

            if (!first) json += ",";
            first = false;

            char buf[256];
            snprintf(buf, sizeof(buf), "{\"x\":%.1f,\"y\":%.1f,\"enemy\":%s,\"hp\":%d}",
                pos.x, pos.y, isEnemy ? "true" : "false", pPawn->m_iHealth());
            json += buf;
        }

        // Add local player as the last element with special flag
        if (pLocalPawn)
        {
            CGameSceneNode* pLocalNode = pLocalPawn->m_pGameSceneNode();
            Vector pos = pLocalNode ? pLocalNode->m_vecAbsOrigin() : Vector();
            QAngle viewAngles = g_Interfaces.m_CSGOInput.m_angViewAngle;
            if (!first) json += ",";
            char buf[256];
            snprintf(buf, sizeof(buf), "{\"x\":%.1f,\"y\":%.1f,\"enemy\":false,\"hp\":%d,\"local\":true,\"yaw\":%.1f}",
                pos.x, pos.y, pLocalPawn->m_iHealth(), viewAngles.y);
            json += buf;
        }

        json += "]";

        std::lock_guard<std::mutex> lock(g_Mutex);
        g_sJsonPayload = json;
    }
}
