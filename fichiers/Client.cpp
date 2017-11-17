#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <cstring>



using namespace std;

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"



int __cdecl main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char *sendbuf = "this is a test";
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	struct sockaddr_in clientService;

	// Validate the parameters
	//if (argc != 2) {
	//	printf("usage: %s server-name\n", argv[0]);
	//}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	bool validIp = false;
	bool validPort = false;

	string ip;
	string port;
	string nom;
	string mdp;
	struct sockaddr_in sa;
	//use get_s instead of cin to avoid shenanigans
	while (!validIp) {
		printf("Enter your IP address: \n");
		getline(std::cin, ip);
		if (inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) == 1) {
			validIp = true;
		}
	}

	while (!validPort) {
		printf("Enter your port address: \n");
		getline(std::cin, port);
		if (stoi(port) >= 5000 && stoi(port) <= 5050) {
			validPort = true;
		}
	}

	printf("Enter your username: \n");
	getline(std::cin, nom);

	printf("Enter your password: \n");
	getline(std::cin, mdp);

	// Resolve the server address and port
	iResult = getaddrinfo(argv[1], port.c_str(), &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
	}

	// Attempt to connect to an address until one succeeds
	//for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		//////////// Create a SOCKET for connecting to server
	ptr = result;

	ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ConnectSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
	}
	////////////////  Connect to server.

	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(ip.c_str());
	clientService.sin_port = htons(stoi(port));

	//PSTR s = const_cast<char *>(ip.c_str());
	//ptr->ai_addr = sa;

	//inet_ntop(AF_INET, &(sa.sin_addr), PSTR(ptr->ai_addr), INET_ADDRSTRLEN);
	iResult = connect(ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService));
	if (iResult == SOCKET_ERROR) {
		printf("Unable to connect to server... %i\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		//return 1;
	}
	/*
	char name[500] = "";
	iResult = gethostname(name, sizeof(name));
	if (iResult == NO_ERROR) {
		printf("Host Name: %s\n", name);
	}
	else if (iResult == SOCKET_ERROR) {
		printf("Could not resolve host name: %i", WSAGetLastError());
	}

	sockaddr_in sName;
	int sNameSize = sizeof(sName);

	iResult = getpeername(ConnectSocket, (struct sockaddr*)&sName, &sNameSize);
	if (iResult == NO_ERROR)
		printf("Peer Name: %s\n", inet_ntoa(sName.sin_addr));
	else if (iResult == SOCKET_ERROR)
		printf("Could not get peer name: %i\n", WSAGetLastError());*/

	////////////////////////////////////////////////////////////////////

	//freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
	}

	// Send an initial buffer
	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
	}

	printf("Bytes Sent: %ld\n", iResult);

	// shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		//return 5;
	}

	// Receive until the peer closes the connection
	do {
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
			printf("Bytes received: %d\n", iResult);
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed with error: %d\n", WSAGetLastError());

	} while (iResult > 0);


	/*const char *inet_ntop(int af, const void *src,
		char *dst, socklen_t size);

	int inet_pton(int af, const char *src, void *dst);*/

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();
	while (1);
	return 0;
}