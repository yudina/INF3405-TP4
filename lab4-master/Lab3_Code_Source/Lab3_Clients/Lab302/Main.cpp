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

// Link avec ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

int __cdecl main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET leSocket;// = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char motEnvoye[200];
	char motRecu[200];
	int iResult;

	//--------------------------------------------
	// InitialisATION de Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("Erreur de WSAStartup: %d\n", iResult);
		return 1;
	}
	// On va creer le socket pour communiquer avec le serveur
	leSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (leSocket == INVALID_SOCKET) {
		printf("Erreur de socket(): %ld\n\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		printf("Appuyez une touche pour finir\n");
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
	
	bool validIp = false;
	bool validPort = false;

	string ip;
	string port_Input;
	string username;
	string password;
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
		getline(std::cin, port_Input);
		if (stoi(port_Input) >= 5000 && stoi(port_Input) <= 5050) {
			validPort = true;
		}
	}
	
	char *host = (char*)ip.c_str();
	char *port = (char*)port_Input.c_str();

	// getaddrinfo obtient l'adresse IP du host donné
	iResult = getaddrinfo(host, port, &hints, &result);
	if (iResult != 0) {
		printf("Erreur de getaddrinfo: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	//---------------------------------------------------------------------		
	//On parcours les adresses retournees jusqu'a trouver la premiere adresse IPV4
	while ((result != NULL) && (result->ai_family != AF_INET))
		result = result->ai_next;

	//	if ((result != NULL) &&(result->ai_family==AF_INET)) result = result->ai_next;  

	//-----------------------------------------
	if (((result == NULL) || (result->ai_family != AF_INET))) {
		freeaddrinfo(result);
		printf("Impossible de recuperer la bonne adresse\n\n");
		WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();
		return 1;
	}

	sockaddr_in *adresse;
	adresse = (struct sockaddr_in *) result->ai_addr;
	//----------------------------------------------------
	printf("Adresse trouvee pour le serveur %s : %s\n\n", host, inet_ntoa(adresse->sin_addr));
	printf("Tentative de connexion au serveur %s avec le port %s\n\n", inet_ntoa(adresse->sin_addr), port);

	// On va se connecter au serveur en utilisant l'adresse qui se trouve dans
	// la variable result.
	iResult = connect(leSocket, result->ai_addr, (int)(result->ai_addrlen));
	if (iResult == SOCKET_ERROR) {
		printf("Impossible de se connecter au serveur %s sur le port %s\n\n", inet_ntoa(adresse->sin_addr), port);
		freeaddrinfo(result);
		WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();
		return 1;
	}

	printf("Connecte au serveur %s:%s\n\n", host, port);
	freeaddrinfo(result);
	
	printf("Enter your username: \n");
	getline(std::cin, username);
	printf("Enter your password: \n");
	getline(std::cin, password);

	string userCredentials = USERNAMEKEY + username + PASSWORDKEY + password;

	iResult = send(leSocket, userCredentials.c_str(), strlen(userCredentials.c_str()), 0);
	if (iResult == SOCKET_ERROR) {
		printf("Erreur du send: %d\n", WSAGetLastError());
		closesocket(leSocket);
		WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();

		return 1;
	}
	iResult = recv(leSocket, motRecu, strlen(motRecu), 0);
	if (iResult > 0) {
		printf("Nombre d'octets recus: %d\n", iResult);
		motRecu[iResult] = '\0';
		printf("Le mot recu est %s\n", motRecu);
	}
	else {
		printf("Erreur de reception : %d\n", WSAGetLastError());
	}

	//----------------------------
	// Demander à l'usager un mot a envoyer au serveur
	printf("Saisir un message de moins de 200 caracteres pour envoyer au serveur: ");
	gets_s(motEnvoye, 200);

	//-----------------------------
	// Envoyer le mot au serveur
	iResult = send(leSocket, motEnvoye, strlen(motEnvoye), 0);
	if (iResult == SOCKET_ERROR) {
		printf("Erreur du send: %d\n", WSAGetLastError());
		closesocket(leSocket);
		WSACleanup();
		printf("Appuyez une touche pour finir\n");
		getchar();

		return 1;
	}

	printf("Nombre d'octets envoyes : %ld\n", iResult);

	//------------------------------
	// Maintenant, on va recevoir l' information envoyée par le serveur
	iResult = recv(leSocket, motRecu, 200, 0);
	if (iResult > 0) {
		printf("Nombre d'octets recus: %d\n", iResult);
		motRecu[iResult] = '\0';
		printf("Le mot recu est %s\n", motRecu);
	}
	else {
		printf("Erreur de reception : %d\n", WSAGetLastError());
	}

	// cleanup
	closesocket(leSocket);
	WSACleanup();

	printf("Appuyez une touche pour finir\n");
	getchar();
	return 0;
}
