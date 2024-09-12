#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "Common.h"
#include "ServerOperations.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"

bool InitializeWindowsSockets();
DWORD WINAPI voteTimerThread(LPVOID lpParam);

int  main(void) 
{
    // Socket used for listening for new clients 
    SOCKET listenSocket = INVALID_SOCKET;
    // Socket used for communication with client
    SOCKET clientSocket[3];
    int clientIdx = 0;
    // variable used to store function return value
    int iResult;
    // Buffer used for storing incoming data
    char recvbuf[DEFAULT_BUFLEN];
    
    if(InitializeWindowsSockets() == false)
    {
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
    }
    
    // Prepare address information structures
    addrinfo *resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
    if ( iResult != 0 )
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    listenSocket = socket(AF_INET,      // IPv4 address famly
                          SOCK_STREAM,  // stream socket
                          IPPROTO_TCP); // TCP

    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    listenSocket = socket(AF_INET,      // IPv4 address famly
        SOCK_STREAM,  // stream socket
        IPPROTO_TCP); // TCP

    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket - bind port number and local address 
    // to socket
    iResult = bind( listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(resultingAddress);

    // Set listenSocket in listening mode
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

	printf("Server initialized, waiting for clients.\n");

    unsigned long mode = 1; //non-blocking mode
    iResult = ioctlsocket(listenSocket, FIONBIO, &mode);
    if (iResult != NO_ERROR)
        printf("ioctlsocket failed with error: %ld\n", iResult);

    NODE* head;
    CRITICAL_SECTION cs;
    InitializeCriticalSection(&cs);
    Init(&head, cs);
    VOTE_TIMER_PARAMS params;
    params.head = &head;
    params.cs = &cs;
    DWORD voteTimerId;
    HANDLE voteTimerHandle;

    voteTimerHandle = CreateThread(NULL, 0, &voteTimerThread, &params, 0, &voteTimerId);
    
    do
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenSocket, &readfds);
        timeval timeVal;
        timeVal.tv_sec = 1;
        timeVal.tv_usec = 0;

        int result = select(0, &readfds, NULL, NULL, &timeVal);

        if (result == 0)
        {
            // t za cekanje je isteklo
        }
        else if (result == SOCKET_ERROR)
        {
            //desila se greska prilikom poziva funkcije
        }
        else
        {
            if (FD_ISSET(listenSocket, &readfds) && clientIdx < 3)
            {
                clientSocket[clientIdx] = accept(listenSocket, NULL, NULL);

                if (clientSocket[clientIdx] == INVALID_SOCKET)
                {
                    printf("accept failed with error: %d\n", WSAGetLastError());
                    closesocket(listenSocket);
                    WSACleanup();
                    return 1;
                }

                mode = 1;
                iResult = ioctlsocket(clientSocket[clientIdx], FIONBIO, &mode);
                if (iResult != NO_ERROR)
                    printf("ioctlsocket failed with error: %ld\n", iResult);

                sprintf_s(recvbuf, "1. Stranka 1\n2. Stranka 2\n3. Stranka 3");

                iResult = send(clientSocket[clientIdx], recvbuf, (int)strlen(recvbuf) + 1, 0);
                if (iResult == SOCKET_ERROR)
                {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(clientSocket[clientIdx]);
                    WSACleanup();
                    return 1;
                }
                clientIdx++;
            }
        }

        timeval timeVal2;
        timeVal2.tv_sec = 1;
        timeVal2.tv_usec = 0;
        fd_set readFdsClients;
        FD_ZERO(&readFdsClients);
        for (int i = 0; i < clientIdx; i++)
        {
            FD_SET(clientSocket[i], &readFdsClients);
        }
        result = select(0, &readFdsClients, NULL, NULL, &timeVal);

        if (result == 0)
        {
        }
        else if (result == SOCKET_ERROR)
        {
        }
        else
        {
            for (int i = 0; i < clientIdx; i++)
            {
                if (FD_ISSET(clientSocket[i], &readFdsClients))
                {
                    iResult = recv(clientSocket[i], recvbuf, DEFAULT_BUFLEN, 0);
                    if (iResult > 0)
                    {
                        REQUEST cg = *(REQUEST*)recvbuf;
                        VOTE g;
                        strcpy(g.r.id, cg.id);
                        g.r.choice = cg.choice;
                        g.t = time(NULL);
                        Enqueue(&head, g, cs);
                    }
                    else if (iResult == 0)
                    {
                        // connection was closed gracefully
                        printf("Connection with client closed.\n");
                        closesocket(clientSocket[i]);
                    }
                    else
                    {
                        // there was an error during recv
                        printf("recv failed with error: %d\n", WSAGetLastError());
                        closesocket(clientSocket[i]);
                    }
                }
            }
        }

    } while (1);


    closesocket(listenSocket);
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
DWORD WINAPI voteTimerThread(LPVOID lpParam)
{
    VOTE_TIMER_PARAMS* params = (VOTE_TIMER_PARAMS*)lpParam;
    VOTES ret;
    for (int i = 0; i < 3; i++)
    {
        ret.votes[i] = 0;
    }
    Sleep(30000);
    while (*params->head != NULL)
    {
        VOTE v = Dequeue(&(*params->head), *params->cs);
        ret.votes[v.r.choice - 1]++;
    }
    bool isRecieved = true;

    SOCKET connectSocket = INVALID_SOCKET;
    // variable used to store function return value
    int iResult;
    // message to send
    char messageToSend[512];
    if (InitializeWindowsSockets() == false)
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
    serverAddress.sin_port = htons(27018);
    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }

    iResult = send(connectSocket, (char*)&ret, sizeof(VOTES), 0);
    if (iResult == SOCKET_ERROR)
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }


    return 0;
}