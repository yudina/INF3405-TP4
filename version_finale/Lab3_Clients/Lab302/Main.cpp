#undef UNICODE

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <cstring>

using namespace std;

const string PASSWORDKEY = "$password=";
const string USERNAMEKEY = "$username=";
bool clientIsConnected;
SOCKET leSocket;// = INVALID_SOCKET;

// Link avec ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

extern DWORD WINAPI listener(LPVOID arg);

int __cdecl main(int argc, char **argv)
{
	WSADATA wsaData;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char motEnvoye[2000];
	char motRecu[2000];
	int iResult;

	//--------------------------------------------
	// InitialisATION de Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("Error on startup: %d\n", iResult);
		return 1;
	}
	// On va creer le socket pour communiquer avec le serveur
	leSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (leSocket == INVALID_SOCKET) {
		printf("Erreur de socket(): %ld\n\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		printf("Enter any key to finish\n");
		getchar();
		return 1;
	}
	//--------------------------------------------
	// On va chercher l'adresse du serveur en utilisant la fonction getaddrinfo.
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;        // Famille d'adresses
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;  // Protocole utilisé par le serveur

									  // On indique le nom et le port du serveur auquel on veut se connecter
									  //char *host = "L4708-XX";
									  //char *host = "L4708-XX.lerb.polymtl.ca";
									  //char *host = "add_IP locale";
	bool connecting = true;
	bool validIp = false;
	bool validPort = false;

	string ip;
	string port_Input;
	string username;
	string password;
	struct sockaddr_in sa;
	//char host;
	//char port;

		//use getline instead of cin to avoid shenanigans
	bool connected = false; 

	while (!connected) {
		while (!validIp) {
			printf("Enter your IP address: \n");
			getline(std::cin, ip);
			if (inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) == 1) {
				validIp = true;
			}
		}

		while (!validPort) {
			printf("Enter your port address: \n");
			getline(std::cin, port_Input);

			try { // use try catch in case user doesn't input an int friendly format
				if (stoi(port_Input) >= 5000 && stoi(port_Input) <= 5050) {
					validPort = true;
				}
			}
			catch (...) {
				validPort = false;
				cout << "Invalid port! Try again." << endl;
			}
		}

		char *host = (char*)ip.c_str();
		char *port = (char*)port_Input.c_str();

		// getaddrinfo obtient l'adresse IP du host donné
		iResult = getaddrinfo(host, port, &hints, &result);
		if (iResult != 0) {
			printf("Error: %d\n", iResult);
			WSACleanup();
			return 1;
		}
		//---------------------------------------------------------------------		
		//On parcours les adresses retournees jusqu'a trouver la premiere adresse IPV4
		while ((result != NULL) && (result->ai_family != AF_INET))
			result = result->ai_next;

		//-----------------------------------------
		if (((result == NULL) || (result->ai_family != AF_INET))) {
			freeaddrinfo(result);
			printf("Can't find address \n\n");
			WSACleanup();
			printf("Enter any key to finish\n");
			getchar();
			return 1;
		}

		sockaddr_in *adresse;
		adresse = (struct sockaddr_in *) result->ai_addr;
		//----------------------------------------------------
		printf("Found address for server %s : %s\n\n", host, inet_ntoa(adresse->sin_addr));
		printf("Trying to connect to server %s with the port %s\n\n", inet_ntoa(adresse->sin_addr), port);

		// On va se connecter au serveur en utilisant l'adresse qui se trouve dans
		// la variable result.
		iResult = connect(leSocket, result->ai_addr, (int)(result->ai_addrlen));
		if (iResult == SOCKET_ERROR) {
			printf("Can't connect to server %s on the port %s\n\n", inet_ntoa(adresse->sin_addr), port);
			freeaddrinfo(result);
			//WSACleanup();
			validPort = false; 
			validIp = false; 
			//printf("Can't connect. Press any key to exit. \n");
			//getchar();
			//return 1;
		}
		else {
			connected = true;
			printf("Connected to server %s:%s\n\n", host, port);
			freeaddrinfo(result);
		}
	}

	////// password validation : /////////

	bool goodCredentials = false;
	// loop until we get an existing user / password combo (checked on the server)
	while (!goodCredentials) {

		printf("Enter your username: \n");
		getline(std::cin, username);
		printf("Enter your password: \n");
		getline(std::cin, password);

		string userCredentials = USERNAMEKEY + username + PASSWORDKEY + password;

		iResult = send(leSocket, userCredentials.c_str(), strlen(userCredentials.c_str()), 0);
		if (iResult == SOCKET_ERROR) {
			printf("Problem sending: %d\n", WSAGetLastError());
			closesocket(leSocket);
			WSACleanup();
			printf("Enter any key to finish\n");
			getchar();
			return 1;
		}
		string msg;
		bool receiving = true;
		int readBytes;
		readBytes = recv(leSocket, motRecu, 2000, 0);
		msg += motRecu;

		while (readBytes > 0 && motRecu[readBytes - 1] != '\0') {
			msg += motRecu;
			readBytes = recv(leSocket, motRecu, 2000, 0);
		}

		if (iResult > 0) {
			motRecu[iResult] = '\0';
		}
		else {
			printf("Erreur de reception : %d\n", WSAGetLastError());
			closesocket(leSocket);
			WSACleanup();
			printf("Enter any key to finish\n");
			getchar();
			return 1;
		}

		if (msg == "goodCredentials") {
			goodCredentials = true;
			// on recoit les 15 messages (ou moins) les plus récents
			memset(motRecu, 0, 2000);
			readBytes = recv(leSocket, motRecu, 2000, 0);
			string latestsMsgs;
			latestsMsgs += motRecu;
			cout << "Login succesful!" << endl;
			while (readBytes > 0 && motRecu[readBytes - 1] != '\0') {
				memset(motRecu, 0, 2000);
				readBytes = recv(leSocket, motRecu, 2000, 0);
				latestsMsgs += motRecu;
			}
			if (readBytes == -1) {
				cout << "Problem connecting to server!" << endl;
				closesocket(leSocket);
				WSACleanup();
				printf("Enter any key to finish\n");
				getchar();
				return 1;
			}
			if (latestsMsgs.size() > 0) {
				latestsMsgs = latestsMsgs.substr(0, latestsMsgs.size() - 1); // remove end signal
			}
			if (!latestsMsgs.empty()) {
				cout << "Here is a log of recent messages: \n " + latestsMsgs << endl;
			}
			else {
				cout << "There are no messages in recent history. You're probably the first user. \n  " + latestsMsgs << endl;
			}

		}

		else if (msg == "badCredentials") {
			cout << "Bad username/password. Try again!" << endl;

		}
	}

	clientIsConnected = true; //we went through all the steps to connect the client to the chat

	HANDLE th[1];
	DWORD thid;
	HANDLE listeningThread = CreateThread(0, 0, listener, (LPVOID)&th[0], 0, &thid);


	////// chatting : ////////
	// TO DO : make listening thread & allow user to write 
	printf("Type here to send messages of 200 characters or less: ");
	while (clientIsConnected) {
		gets_s(motEnvoye, 2000); // get user input 

		// check how long it is ...

		if (string(motEnvoye).length() <= 200) {
			//-----------------------------
			// Envoyer le mot au serveur

			iResult = send(leSocket, motEnvoye, strlen(motEnvoye), 0);
			if (iResult == -1) {
				cout << "We can't connect to server!" << endl;
				return 1;
			}
			else if (iResult == SOCKET_ERROR) {
				printf("Erreur du send: %d\n", WSAGetLastError());
				closesocket(leSocket);
				WSACleanup();
				printf("Enter any key to finish\n");
				getchar();
				return 1;
			}
		}
		else {
			cout << "Your message is too long. It has to be 200 characters or less. Try again:" << endl;
		}

	}
	// cleanup // end of tranmissions
	WaitForSingleObject(listeningThread, INFINITE);
	closesocket(leSocket);
	WSACleanup();

	printf("Appuyez une touche pour finir\n");
	getchar();
	return 0;
}

DWORD WINAPI listener(LPVOID arg) {
	while (clientIsConnected) {
		char motEnvoye[2000];
		char motRecu[2000];
		int readBytes;
		string msg;

		//recevoir l' information envoyée par le serveur
		readBytes = recv(leSocket, motRecu, 2000, 0);
		msg += motRecu;
		while (readBytes > 0 && motRecu[readBytes - 1] != '\0') {
			readBytes = recv(leSocket, motRecu, 2000, 0);
			msg += motRecu;
		}

		if (readBytes == -1) {
			clientIsConnected = false;
			printf("\Server disconnected. Enter any key to finish\n");
			getchar();
			return 0;
		}
		else {
			cout << msg << endl;
		}
	}
	return 0;
}
