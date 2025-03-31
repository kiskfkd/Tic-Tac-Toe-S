#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 8080;
const int BOARD_SIZE = 3;

int board[BOARD_SIZE][BOARD_SIZE] = { 0 };
int turn = 0;
SOCKET clientSockets[2];

bool CheckWin(int player) {
    // ���̃`�F�b�N
    for (int y = 0; y < BOARD_SIZE; y++) {
        if (board[y][0] == player && board[y][1] == player && board[y][2] == player) {
            return true;
        }
    }

    // �c�̃`�F�b�N
    for (int x = 0; x < BOARD_SIZE; x++) {
        if (board[0][x] == player && board[1][x] == player && board[2][x] == player) {
            return true;
        }
    }

    // �΂߂̃`�F�b�N
    if (board[0][0] == player && board[1][1] == player && board[2][2] == player) {
        return true;
    }
    if (board[0][2] == player && board[1][1] == player && board[2][0] == player) {
        return true;
    }

    return false;
}

bool CheckDraw() {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x] == 0) return false; // �󂫂�����Ȃ疢����
        }
    }
    return true; // ���ׂĖ��܂��Ă���Ȃ��������
}
void InitializeServer() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 2);

    std::cout << "�T�[�o�[���J�n����܂����B�N���C�A���g��ҋ@��...\n";

    // �N���C�A���g2���󂯓����
    for (int i = 0; i < 2; i++) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        clientSockets[i] = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        std::cout << "�N���C�A���g" << i + 1 << "���ڑ����܂����B\n";

        // �N���C�A���g�Ɏ�����ID�𑗂�
        char buffer[32];
        sprintf(buffer, "ClientID: %d", i);

        int sendResult = send(clientSockets[i], buffer, strlen(buffer), 0);
        if (sendResult == SOCKET_ERROR) {
            std::cerr << "�N���C�A���g" << i << " �ւ̑��M�Ɏ��s���܂���: " << WSAGetLastError() << std::endl;
        }
        else {
            std::cout << "�N���C�A���g" << i << " �� ID �𑗐M: " << i << std::endl;
        }
    }

    closesocket(serverSocket);
}

void BroadcastBoard() {
    char buffer[64];
    int index = 0;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            index += sprintf(buffer + index, "%d ", board[y][x]);
        }
    }
    sprintf(buffer + index, "%d", turn);

    for (int i = 0; i < 2; i++) {
        send(clientSockets[i], buffer, strlen(buffer), 0);
    }
}

void HandleMoves() {
    while (true) {
        fd_set readfds;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        FD_ZERO(&readfds);
        FD_SET(clientSockets[0], &readfds);
        FD_SET(clientSockets[1], &readfds);

        int ret = select(0, &readfds, NULL, NULL, &timeout);

        if (ret > 0) {
            for (int i = 0; i < 2; i++) {
                if (FD_ISSET(clientSockets[i], &readfds)) {
                    char buffer[32];
                    int bytesReceived = recv(clientSockets[i], buffer, sizeof(buffer) - 1, 0);

                    if (bytesReceived > 0) {
                        buffer[bytesReceived] = '\0';
                        int x, y, player;
                        sscanf(buffer, "%d %d %d", &x, &y, &player);

                        if (board[y][x] == 0 && (turn % 2 == i)) {
                            board[y][x] = player;
                            turn++;

                            // ���s����
                            if (CheckWin(player)) {
                                char msg[32];
                                sprintf(msg, "Win: %d", player);
                                send(clientSockets[0], msg, strlen(msg), 0);
                                send(clientSockets[1], msg, strlen(msg), 0);
                                return; // �Q�[���I��
                            }
                            if (CheckDraw()) {
                                char msg[] = "Draw";
                                send(clientSockets[0], msg, strlen(msg), 0);
                                send(clientSockets[1], msg, strlen(msg), 0);
                                return; // �Q�[���I��
                            }

                            BroadcastBoard();
                        }
                    }
                }
            }
        }
    }
}

int main() {
    InitializeServer();
    HandleMoves();

    for (int i = 0; i < 2; i++) {
        closesocket(clientSockets[i]);
    }
    WSACleanup();
    return 0;
}
