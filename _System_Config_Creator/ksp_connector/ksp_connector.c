#include "ksp_connector.h"
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 9000

SOCKET server_socket = INVALID_SOCKET;
SOCKET client_socket = INVALID_SOCKET;

// Call once to setup server socket and start listening
int open_connection() {
    WSADATA wsa;
    struct sockaddr_in server_addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 0;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) return 0;

    u_long mode = 1;  // 1 = non-blocking
    ioctlsocket(server_socket, FIONBIO, &mode);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) return 0;
    if (listen(server_socket, 1) == SOCKET_ERROR) return 0;

    printf("Server listening...\n");
    return 1;
}

// Call periodically to try to accept a client without blocking
void looking_for_client(KspConnector *conn) {
    while (conn->try_to_connect) {
        if (client_socket != INVALID_SOCKET) return; // Already connected

        struct sockaddr_in client_addr;
        int len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &len);

        if (client_socket != INVALID_SOCKET) {
            printf("Client connected.\n");
            conn->has_client = 1;
            return;
        }

        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            printf("Accept failed: %d\n", WSAGetLastError());
        }
    }
}

// Call periodically when client is connected to try receiving data
int receive_data(double vec[3]) {
    if (client_socket == INVALID_SOCKET) return 0;

    u_long mode = 1;
    ioctlsocket(client_socket, FIONBIO, &mode); // Make sure it's non-blocking

    int expected = 3 * sizeof(double);
    int received = recv(client_socket, (char*)vec, expected, 0);

    if (received == expected) {
        printf("Received Vector3: (%f, %f, %f)\n", vec[0], vec[1], vec[2]);
        return 1;
    } else if (received == 0) {
        printf("Client disconnected.\n");
        closesocket(client_socket);
        client_socket = INVALID_SOCKET;
    } else {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
            printf("Receive error: %d\n", WSAGetLastError());
    }

    return 0;
}

void close_connection() {
    if (client_socket != INVALID_SOCKET) {
        closesocket(client_socket);
        client_socket = INVALID_SOCKET;
    }
    if (server_socket != INVALID_SOCKET) {
        closesocket(server_socket);
        server_socket = INVALID_SOCKET;
    }
    WSACleanup();
    printf("Connection closed.\n");
}