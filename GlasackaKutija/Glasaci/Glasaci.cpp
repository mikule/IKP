#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <time.h>
#include "Common.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016

// Initializes WinSock2 library
// Returns true if succeeded, false otherwise.
bool InitializeWindowsSockets();

int __cdecl main() 
{
    // socket used to communicate with server
    SOCKET connectSocket = INVALID_SOCKET;
    // variable used to store function return value
    int iResult;
    // message to send
    char messageBuffer[DEFAULT_BUFLEN];
    char messageBuffer2[DEFAULT_BUFLEN];
    
    if(InitializeWindowsSockets() == false)
    {
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
    }

    // create a socket
    connectSocket = socket(AF_INET,
                           SOCK_STREAM,
                           IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // create and initialize address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(DEFAULT_PORT);
    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }
    
    iResult = recv(connectSocket, messageBuffer2, DEFAULT_BUFLEN, 0);
    if (iResult > 0)
    {
        printf("%s\n", messageBuffer2);
    }
    else if (iResult == 0)
    {
        // connection was closed gracefully
        printf("Connection with client closed.\n");
        closesocket(connectSocket);
    }
    else
    {
        // there was an error during recv
        printf("recv failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
    }

    do
    {
        REQUEST cg;
        system("cls");
        printf("%s\n", messageBuffer2);
        puts("Unesite id glasaca.");
        gets_s(cg.id, 20);
        char kandidat[10];
        puts("Unesite izbor.");
        gets_s(kandidat, 10);
        cg.choice = atoi(kandidat);
        
        iResult = send( connectSocket, (char*)&cg, sizeof(REQUEST), 0 );
        if (iResult == SOCKET_ERROR)
        {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }
    } while (true);

    closesocket(connectSocket);
    WSACleanup();

    return 0;
}

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
	// Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
	return true;
}
