#undef UNICODE

// IP 132.207.29.118
#include <winsock2.h>
#include <iostream>
#include <algorithm>
#include <strstream>
#include <string>
#include <cstring>
#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <ctime>
#include <vector>

using namespace std;

const string PASSWORDKEY = "$password=";
const string USERNAMEKEY = "$username=";
const char userDB[] = "userDB.txt";
const char msgDB[] = "msgDB.txt";
vector<SOCKET> connectedUsers;

HANDLE msgMutex;
HANDLE userMutex;

// link with Ws2_32.lib
#pragma comment( lib, "ws2_32.lib" )

// External functions
extern DWORD WINAPI EchoHandler(void* sd_);
extern boolean validateCredentials(string username, string password);
extern string formatMessage(string msg, string username, string port, string ip);
extern void addMessageToDB(string msg, string username, string port, string ip);
extern void sendFifteenLatestMessages(SOCKET sd);
extern vector<string> getLastestMessages();


// List of Winsock error constants mapped to an interpretation string.
// Note that this list must remain sorted by the error constants'
// values, because we do a binary search on the list when looking up
// items.
static struct ErrorEntry {
	int nID;
	const char* pcMessage;

	ErrorEntry(int id, const char* pc = 0) :
		nID(id),
		pcMessage(pc)
	{
	}

	bool operator<(const ErrorEntry& rhs) const
	{
		return nID < rhs.nID;
	}
} gaErrorList[] = {
	ErrorEntry(0,                  "No error"),
	ErrorEntry(WSAEINTR,           "Interrupted system call"),
	ErrorEntry(WSAEBADF,           "Bad file number"),
	ErrorEntry(WSAEACCES,          "Permission denied"),
	ErrorEntry(WSAEFAULT,          "Bad address"),
	ErrorEntry(WSAEINVAL,          "Invalid argument"),
	ErrorEntry(WSAEMFILE,          "Too many open sockets"),
	ErrorEntry(WSAEWOULDBLOCK,     "Operation would block"),
	ErrorEntry(WSAEINPROGRESS,     "Operation now in progress"),
	ErrorEntry(WSAEALREADY,        "Operation already in progress"),
	ErrorEntry(WSAENOTSOCK,        "Socket operation on non-socket"),
	ErrorEntry(WSAEDESTADDRREQ,    "Destination address required"),
	ErrorEntry(WSAEMSGSIZE,        "Message too long"),
	ErrorEntry(WSAEPROTOTYPE,      "Protocol wrong type for socket"),
	ErrorEntry(WSAENOPROTOOPT,     "Bad protocol option"),
	ErrorEntry(WSAEPROTONOSUPPORT, "Protocol not supported"),
	ErrorEntry(WSAESOCKTNOSUPPORT, "Socket type not supported"),
	ErrorEntry(WSAEOPNOTSUPP,      "Operation not supported on socket"),
	ErrorEntry(WSAEPFNOSUPPORT,    "Protocol family not supported"),
	ErrorEntry(WSAEAFNOSUPPORT,    "Address family not supported"),
	ErrorEntry(WSAEADDRINUSE,      "Address already in use"),
	ErrorEntry(WSAEADDRNOTAVAIL,   "Can't assign requested address"),
	ErrorEntry(WSAENETDOWN,        "Network is down"),
	ErrorEntry(WSAENETUNREACH,     "Network is unreachable"),
	ErrorEntry(WSAENETRESET,       "Net connection reset"),
	ErrorEntry(WSAECONNABORTED,    "Software caused connection abort"),
	ErrorEntry(WSAECONNRESET,      "Connection reset by peer"),
	ErrorEntry(WSAENOBUFS,         "No buffer space available"),
	ErrorEntry(WSAEISCONN,         "Socket is already connected"),
	ErrorEntry(WSAENOTCONN,        "Socket is not connected"),
	ErrorEntry(WSAESHUTDOWN,       "Can't send after socket shutdown"),
	ErrorEntry(WSAETOOMANYREFS,    "Too many references, can't splice"),
	ErrorEntry(WSAETIMEDOUT,       "Connection timed out"),
	ErrorEntry(WSAECONNREFUSED,    "Connection refused"),
	ErrorEntry(WSAELOOP,           "Too many levels of symbolic links"),
	ErrorEntry(WSAENAMETOOLONG,    "File name too long"),
	ErrorEntry(WSAEHOSTDOWN,       "Host is down"),
	ErrorEntry(WSAEHOSTUNREACH,    "No route to host"),
	ErrorEntry(WSAENOTEMPTY,       "Directory not empty"),
	ErrorEntry(WSAEPROCLIM,        "Too many processes"),
	ErrorEntry(WSAEUSERS,          "Too many users"),
	ErrorEntry(WSAEDQUOT,          "Disc quota exceeded"),
	ErrorEntry(WSAESTALE,          "Stale NFS file handle"),
	ErrorEntry(WSAEREMOTE,         "Too many levels of remote in path"),
	ErrorEntry(WSASYSNOTREADY,     "Network system is unavailable"),
	ErrorEntry(WSAVERNOTSUPPORTED, "Winsock version out of range"),
	ErrorEntry(WSANOTINITIALISED,  "WSAStartup not yet called"),
	ErrorEntry(WSAEDISCON,         "Graceful shutdown in progress"),
	ErrorEntry(WSAHOST_NOT_FOUND,  "Host not found"),
	ErrorEntry(WSANO_DATA,         "No host data of that type was found")
};
const int kNumMessages = sizeof(gaErrorList) / sizeof(ErrorEntry);


//// WSAGetLastErrorMessage ////////////////////////////////////////////
// A function similar in spirit to Unix's perror() that tacks a canned 
// interpretation of the value of WSAGetLastError() onto the end of a
// passed string, separated by a ": ".  Generally, you should implement
// smarter error handling than this, but for default cases and simple
// programs, this function is sufficient.
//
// This function returns a pointer to an internal static buffer, so you
// must copy the data from this function before you call it again.  It
// follows that this function is also not thread-safe.
const char* WSAGetLastErrorMessage(const char* pcMessagePrefix, int nErrorID = 0)
{
	// Build basic error string
	static char acErrorBuffer[256];
	ostrstream outs(acErrorBuffer, sizeof(acErrorBuffer));
	outs << pcMessagePrefix << ": ";

	// Tack appropriate canned message onto end of supplied message 
	// prefix. Note that we do a binary search here: gaErrorList must be
	// sorted by the error constant's value.
	ErrorEntry* pEnd = gaErrorList + kNumMessages;
	ErrorEntry Target(nErrorID ? nErrorID : WSAGetLastError());
	ErrorEntry* it = lower_bound(gaErrorList, pEnd, Target);
	if ((it != pEnd) && (it->nID == Target.nID)) {
		outs << it->pcMessage;
	}
	else {
		// Didn't find error in list, so make up a generic one
		outs << "unknown error";
	}
	outs << " (" << Target.nID << ")";

	// Finish error message off and return it.
	outs << ends;
	acErrorBuffer[sizeof(acErrorBuffer) - 1] = '\0';
	return acErrorBuffer;
}

int main(void)
{
	//----------------------
	// Initialize Winsock.

	cout << formatMessage("messagggeeee", "userrrr", "5000", "123.213.43.31") << endl;

	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		cerr << "Error at WSAStartup()\n" << endl;
		return 1;
	}

	////// initialize mutexes 

	msgMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	if (msgMutex == NULL)
	{
		printf("CreateMutex error: %d\n", GetLastError());
		return 1;
	}
	userMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

	if (userMutex == NULL)
	{
		printf("CreateMutex error: %d\n", GetLastError());
		return 1;
	}

	//----------------------
	// Create a SOCKET for listening for
	// incoming connection requests.
	SOCKET ServerSocket;
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ServerSocket == INVALID_SOCKET) {
		cerr << WSAGetLastErrorMessage("Error at socket()") << endl;
		WSACleanup();
		return 1;
	}
	char option[] = "1";
	setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, option, sizeof(option));

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.
	//int port=5000;

	//Recuperation de l'adresse locale
	hostent *thisHost;

	printf("server has started...\n");
	bool validIp = false;
	bool validPort = false;

	string ip_input;
	string port;
	struct sockaddr_in serverService;

	struct sockaddr_in sa;


	//use get_s instead of cin to avoid shenanigans
	while (!validIp) {
		printf("Enter your IP address: \n");
		getline(std::cin, ip_input);
		if (inet_pton(AF_INET, ip_input.c_str(), &(sa.sin_addr)) == 1) {
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
	thisHost = gethostbyname(ip_input.c_str());
	char* ip;
	ip = inet_ntoa(*(struct in_addr*) *thisHost->h_addr_list);
	printf("Adresse locale trouvee %s : \n\n", ip);
	sockaddr_in service;
	service.sin_family = AF_INET;
	//service.sin_addr.s_addr = inet_addr("127.0.0.1");
	//	service.sin_addr.s_addr = INADDR_ANY;
	service.sin_addr.s_addr = inet_addr(ip_input.c_str());
	service.sin_port = htons(stoi(port));

	if (bind(ServerSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
		cerr << WSAGetLastErrorMessage("bind() failed.") << endl;
		closesocket(ServerSocket);
		WSACleanup();
		return 1;
	}

	//----------------------
	// Listen for incoming connection requests.
	// on the created socket
	if (listen(ServerSocket, 30) == SOCKET_ERROR) {
		cerr << WSAGetLastErrorMessage("Error listening on socket.") << endl;
		closesocket(ServerSocket);
		WSACleanup();
		return 1;
	}

	printf("En attente des connections des clients sur le port %d...\n\n", ntohs(service.sin_port));

	while (true) {

		sockaddr_in sinRemote;
		int nAddrSize = sizeof(sinRemote);
		// Create a SOCKET for accepting incoming requests.
		// Accept the connection.
		SOCKET sd = accept(ServerSocket, (sockaddr*)&sinRemote, &nAddrSize);
		if (sd != INVALID_SOCKET) {
			cout << "Connection acceptee De : " <<
				inet_ntoa(sinRemote.sin_addr) << ":" <<
				ntohs(sinRemote.sin_port) << "." <<
				endl;

			DWORD nThreadID;
			connectedUsers.push_back(sd);
			CreateThread(0, 0, EchoHandler, (void*)sd, 0, &nThreadID);
		}
		else {
			cerr << WSAGetLastErrorMessage("Echec d'une connection.") <<
				endl;
			// return 1;
		}
	}
	CloseHandle(userMutex);
	CloseHandle(msgMutex);
}


//// EchoHandler ///////////////////////////////////////////////////////
// Handles the incoming data by reflecting it back to the sender.

DWORD WINAPI EchoHandler(void* sd_)
{
	//132.207.29.118
	SOCKET sd = (SOCKET)sd_;

	//string mess = formatMessage("message", "olivia");
	//cout << mess << endl;

	// Read Data from client
	char readBuffer[2000], outBuffer[2000];
	int readBytes;
	memset(readBuffer, 0, 2000);
	memset(outBuffer, 0, 2000);
	readBytes = recv(sd, readBuffer, 2000, 0);
	while (readBytes > 0) {
		cout << "Received " << readBytes << " bytes from client." << endl;
		cout << "Received " << readBuffer << " from client." << endl;

		if (string(readBuffer).find(USERNAMEKEY) != std::string::npos &&
			string(readBuffer).find(PASSWORDKEY) != std::string::npos) { // condition to check credentials 

			int usernameStartIndex = string(readBuffer).find(USERNAMEKEY) + string(USERNAMEKEY).length();
			int usernameEndIndex = string(readBuffer).find(PASSWORDKEY) - +string(PASSWORDKEY).length();
			int passwordStartIndex = string(readBuffer).find(PASSWORDKEY) + string(PASSWORDKEY).length();
			int passwordEndIndex = string(readBuffer).length() - 2;

			string username = string(readBuffer).substr(usernameStartIndex, usernameEndIndex);
			string password = string(readBuffer).substr(passwordStartIndex, passwordEndIndex);

			bool goodCredentials = validateCredentials(username, password);

			if (goodCredentials) {
				string good = "goodCredentials";
				memset(outBuffer, 0, 2000);
				strcpy(outBuffer, good.c_str());
				cout << "Sending " + string(outBuffer) << endl;
				send(sd, outBuffer, 2000, 0);
				////// send 15 latest messages (or less if applicable) 
				sendFifteenLatestMessages(sd);
			}
			else {
				string bad = "badCredentials";
				memset(outBuffer, 0, 2000);
				strcpy(outBuffer, bad.c_str());
				send(sd, outBuffer, 2000, 0);
			}
		}
		//// no need to check credentials... just add msg to DB & broadcast to connected clients (?) 
		// TO DO : add message to DB 
		// TO DO : call multicast function 

		//send(sd, outBuffer, readBytes, 0);
		// clear buffer & continue reading 
		memset(readBuffer, 0, 2000);
		readBytes = recv(sd, readBuffer, 2000, 0);
	}
	if (readBytes == SOCKET_ERROR) {
		cout << WSAGetLastErrorMessage("Echec de la reception !") << endl;
	}

	//closesocket(sd);

	return 0;
}



void sendFifteenLatestMessages(SOCKET sd) {
	vector<string> toSend = getLastestMessages();
	char sendBuffer[2000];
	memset(sendBuffer, 0, 2000);
	int i = 0;
	if (toSend.empty()) {
		send(sd, sendBuffer, 2000, 0);
		return;
	}

	for (int i = 0; i < toSend.size(); i++) {
		toSend.at(i) = toSend.at(i) + '\n'; // each message will be on its own line
		memset(sendBuffer, 0, 2000);
		strcpy(sendBuffer, toSend.at(i).c_str());
		
		if (i == toSend.size() - 1) {
			sendBuffer[toSend.at(i).size()] = '\0'; // add nul to signal last message
			send(sd, sendBuffer, strlen(toSend.at(i).c_str())+1, 0);
			cout << "sending " + string(sendBuffer) << endl;
			memset(sendBuffer, 0, 2000);
		}
		else {
			send(sd, sendBuffer, strlen(toSend.at(i).c_str()), 0);
			cout << "sending " + string(sendBuffer) << endl;
			memset(sendBuffer, 0, 2000);
		}
	}
}

//// format message before sending if you want it to be pretty
// sends message to all connected users
void multicast(string msg) {
	for (int i = 0; i < connectedUsers.size(); i++) {
		//send nul terminated message to all users 
		send(connectedUsers[i], msg.c_str() + '\0', strlen(msg.c_str())+1, 0); 
	}
}

string formatMessage(string msg, string username, string port, string ip)
{
	time_t t = time(0);   // get time now
	struct tm * now = localtime(&t);
	string currentTime = " - " + to_string(now->tm_year + 1900) + " - " + to_string(now->tm_mon + 1) + "-" + to_string(now->tm_mday) + "@" + to_string(now->tm_hour) + ":" + to_string(now->tm_min);

	msg.insert(0, currentTime);
	msg.insert(0, "[");
	msg.insert(currentTime.size() + 1, "]: ");
	msg.insert(1, port);
	msg.insert(1, ip + ":");
	msg.insert(1, username + " - ");

	return msg + '\n';
}

void addMessageToDB(string msg, string username, string port, string ip) {
	string formattedMessageToAdd = formatMessage(msg, username, port, ip);
	DWORD dwWaitResult;
	dwWaitResult = WaitForSingleObject(
		msgMutex,    // handle to mutex
		INFINITE);  // no time-out interval
					// The thread got ownership of the mute
	if (dwWaitResult == WAIT_OBJECT_0) {
		fstream workFileWrite(msgDB, ios_base::app | ios_base::out);
		workFileWrite << formattedMessageToAdd;
	}
	else if (dwWaitResult == WAIT_ABANDONED) {
		return;
	}
}

vector<string> getLastestMessages() {

	DWORD dwWaitResult;

	dwWaitResult = WaitForSingleObject(
		msgMutex,    // handle to mutex
		INFINITE);  // no time-out interval
					// The thread got ownership of the mute
	if (dwWaitResult == WAIT_OBJECT_0) {
		fstream workFileRead(msgDB, ios_base::in);
		int lineCount = 0;
		vector<string> messagesReadFromDB;
		string line;
		while (getline(workFileRead, line) && lineCount < 15)
		{
			lineCount++;
			messagesReadFromDB.push_back(line);
		}
		return messagesReadFromDB;
	}
	else if (dwWaitResult == WAIT_ABANDONED) {
		return {};
	}
	return {};

}

boolean validateCredentials(string username, string password)
{
	//fstream userDBStream(userDB);

	DWORD dwWaitResult;

	dwWaitResult = WaitForSingleObject(
		userMutex,    // handle to mutex
		INFINITE);  // no time-out interval
		// The thread got ownership of the mutex
	if (dwWaitResult == WAIT_OBJECT_0) {
		fstream workFileRead(userDB, ios_base::in);
		fstream workFileWrite(userDB, ios_base::app | ios_base::out);

		// If file does not exist, Create new file
		if (!workFileRead)
		{
			cout << "Cannot open file, file does not exist. Creating new file..";
			system("pause");
			return -1;
		}
		// use existing file
		cout << "success " << userDB << " found. \n";
		boolean userExists = false;
		string line;

		while (getline(workFileRead, line))
		{
			string userFromDB = line.substr(0, line.find(" "));
			if (userFromDB == username) {
				userExists = true;
				string passwordFromDB = line.substr(line.find(" ") + 1, line.length() - 1);
				if (passwordFromDB == password) {
					workFileRead.close();
					return true;
				}
				else {
					workFileRead.close();
					return false;
				}
			}
		}

		if (!userExists) {
			cout << "Writing username to file" << endl;
			workFileWrite << username + " " + password << endl;
		}
		workFileWrite.close();
		cout << "\n";

		return true;
	}
	// The thread got ownership of an abandoned mutex
	// The database is in an indeterminate state
	else if (dwWaitResult == WAIT_ABANDONED) {
		return false;
	}
	return false;
}

