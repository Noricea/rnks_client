#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "package.h"

#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable: 4996)

#define NO_SOCKET -1

int verify_user(char* input) {
    return
        strlen(input) == 6 &&
        (input[0] == 's' ||
            input[0] == 'S') &&
        isdigit(input[1]) != 0 &&
        isdigit(input[2]) != 0 &&
        isdigit(input[3]) != 0 &&
        isdigit(input[4]) != 0 &&
        isdigit(input[5]) != 0;
}

int recv_msg_handler(SOCKET target_socket) {
    package receiver;
    ZeroMemory(receiver.buffer, sizeof(receiver.buffer));

    int ret = recv(target_socket, receiver.buffer, sizeof(receiver.buffer), 0);
    if (ret > 0) {
        printf("%s\n", receiver.buffer);
    }
    else {
        return 1;
    }

    return 0;
}

int send_msg_handler(SOCKET target_socket, package* data) {
    if (_kbhit()) {
        char c = _getch();
        if (c == '\r') {
            data->message[data->pt++] = '\0';
            if (strlen(data->message) > 0) {
                if (strcmp(data->message, "/exit") == 0) {
                    printf("\n[-] You left the chat\n");
                    return 1;
                }
                else {
                    ZeroMemory(data->buffer, sizeof(data->buffer));
                    sprintf(data->buffer, "[%s] %s", data->s_num, data->message);
                    send(target_socket, data->buffer, sizeof(data->buffer), 0);
                    data->pt = 0;
                    ZeroMemory(data->message, sizeof(data->message));
                    printf("\n");
                }
            }
        }
        else if (data->pt < sizeof(data->message) - 1) {
            data->message[data->pt++] = c;
            printf("\33[2K\r[%s] %s", data->s_num, data->message);
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    struct sockaddr_in6 server_addr_IPv6;
    SOCKET server_socket = NO_SOCKET;
    fd_set read_socket, write_socket, except_socket;
    package data;
    char IP_IPv6[40];
    int Port_IPv6;

    if (!argv[1] || !argv[2] || !argv[3]) {
        printf("No parameter.\n");
        exit(EXIT_FAILURE);
    }

    if (!verify_user(argv[1])) {
        printf("Username invalid.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(data.s_num, argv[1]);
    ZeroMemory(IP_IPv6, sizeof(IP_IPv6));
    strcat(IP_IPv6, argv[2]);
    Port_IPv6 = strtol(argv[3], NULL, 10);

    printf("Logged in as: %s\n", data.s_num);
    printf("Connecting to: %s:%d\n\n", IP_IPv6, Port_IPv6);

    //WSA Startup
    WSADATA winsock;
    if (WSAStartup(MAKEWORD(2, 2), &winsock) != 0) {
        printf("[-] Startup failed. Error Code : %d.\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    server_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    ZeroMemory(&server_addr_IPv6, sizeof(server_addr_IPv6));
    if (server_socket == INVALID_SOCKET) {
        printf("[-] Socket creation failed...\n");
        exit(EXIT_FAILURE);
    }
    printf("[+] Socket successfully created.\n");

    server_addr_IPv6.sin6_family = AF_INET6;
    server_addr_IPv6.sin6_port = htons(Port_IPv6);
    inet_pton(AF_INET6, IP_IPv6, &server_addr_IPv6.sin6_addr);

    // connect the client_addrent socket to server socket
    if (connect(server_socket, (struct sockaddr_in*)&server_addr_IPv6, sizeof(server_addr_IPv6)) != 0) {
        printf("[-] Connecting failed...\n");
        exit(0);
    }
    else
        printf("[+] Client connected to Server.\n\n");

    ZeroMemory(data.message, sizeof(data.message));
    data.pt = 0;

    while (1) {
        //select is destructive -> reassign sockets
        FD_ZERO(&read_socket);
        FD_SET(server_socket, &read_socket);

        FD_ZERO(&write_socket);
        FD_SET(server_socket, &write_socket);

        FD_ZERO(&except_socket);
        FD_SET(server_socket, &except_socket);

        int highest_socket = server_socket;

        int return_select = select(highest_socket + 1, &read_socket, &write_socket, &except_socket, NULL);
        switch (return_select) {
        case -1:
            printf("[-] Select failed.\n");
            exit(EXIT_FAILURE);

        case 0:
            printf("[-] Select timed out.\n");
            exit(EXIT_FAILURE);

        default:
            if (FD_ISSET(server_socket, &read_socket)) {
                if (recv_msg_handler(server_socket) != 0) {
                    printf("[-] Server went offline.");
                    shutdown(server_socket, SD_BOTH);
                    server_socket = NO_SOCKET;
                    exit(EXIT_SUCCESS);
                }
            }

            if (FD_ISSET(server_socket, &write_socket)) {
                if (send_msg_handler(server_socket, &data) != 0) {
                    shutdown(server_socket, SD_BOTH);
                    server_socket = NO_SOCKET;
                    exit(EXIT_SUCCESS);
                }
            }

            if (FD_ISSET(server_socket, &except_socket)) {
                printf("[-] Except for server.\n");
            }
        }
    }
}