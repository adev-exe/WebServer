#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

// Make sure build environment links to Winsock library file
#pragma comment(lib, "Ws2_32.lib")

// Define default global constants
#define BUFFERSIZE 1000
#define DEFAULTPORT 80
const string ROOT = "Website\\";


// Define global variable for listen socket so it can be
// closed by the shutdown handler
static SOCKET listenSocket = INVALID_SOCKET;

// Event handler to process Ctrl-C or other shutdown events
static BOOL WINAPI Handler(DWORD cntrlEvent);

// Function to close the listening socket and 
// perform DLL cleanup (WSACleanup)
void cleanup();

//Program does not work with FireFox
int main()
{

	WSADATA		wsaData;				// Structure to hold info about Windows sockets implementation
	SOCKET		clientSocket;			// Socket for communication with the client
	SOCKADDR_IN	serverAddr;				// Structure to hold server address information
	int			iResult;				// Holds code from socket function
	char		buffer[BUFFERSIZE];		// Holds client request 
	string		input;					// Generic input
	string		IPADDR = "127.0.0.1";	// String for the IP Address
	fstream		outFile;				// Opens the file
	int			outCode = 404;			// Gives code for the error 404
	string		header;					// String for the header
	string		http;					// String for http
	int			handled = 0;			// Counts how many times a handle was made
	string		temp = "";				// Temporary string
	string		data;					// String to hold data for getLine
	string		finalStr;				// Hold the completed and final string


	// Set the handler function to handle shutdown events
	if (!SetConsoleCtrlHandler(Handler, TRUE))
	{
		cout << "Error setting server exit handler\n";
		return 1;
	}

	cout << "\n*** WEB SERVER ***\n\n";

	/*********************************************************/
	// Add server startup code here
	/*********************************************************/

	// Gets IP address and sets to default
	// Use to startup Server
	cout << "Started Server...." << endl;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	// Posts if there was an error for WSAStartup
	if (iResult != NO_ERROR)
	{
		cerr << "WSAStartup failed with error: " << iResult << endl;
		return 1;
	}
	cout << "Server Started" << endl;
	cout << "Creating Socket...." << endl;

	// Creates a new socket for communication
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// If socket fails, post the error
	if (listenSocket == INVALID_SOCKET)
	{
		cerr << "Socket failed with error: " << WSAGetLastError() << endl;
		cleanup();
		return 1;
	}

	cout << "Socket Created" << endl;
	cout << "Binding..." << endl;

	//  setup SOCKADDR_IN struture that holds address 
	// and port information for the server
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(DEFAULTPORT);
	inet_pton(AF_INET, IPADDR.c_str(), &serverAddr.sin_addr);

	// Attemps to BIND the local network address to the socket
	iResult = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (iResult == SOCKET_ERROR)
	{
		cerr << "Bind failed with ERROR: " << WSAGetLastError() << endl;
		cleanup();
		return 1;
	}

	cout << "Waiting for connection" << endl;
	cout << "Running on port number " << DEFAULTPORT << "\n\n";

	// Attemps to listen to the socket and prints out an error if it does not work
	iResult = listen(listenSocket, 1);
	if (iResult == SOCKET_ERROR)
	{
		cerr << "Listen failed with ERROR: " << WSAGetLastError() << endl;
		cleanup();
		return 1;
	}


	while (listenSocket != INVALID_SOCKET)	// While the listening socket is not invalid it keeps the loop going.
	{
		/*********************************************************/
		// Add code here to process client requests
		/*********************************************************/
		cout << "Waiting for connection" << endl;

		header = "";

		clientSocket = accept(listenSocket, NULL, NULL);

		// Error Checking
		if (clientSocket == INVALID_SOCKET)
		{
			cerr << "Accept failed with ERROR: " << WSAGetLastError() << endl; // starts loop over and closes client socket if the socket is invalid
			closesocket(clientSocket);
			continue;
		}

		cout << "Request Received" << endl;
		input = "";
		buffer[BUFFERSIZE - 1] = '\0';

		// Breaks out of the loop
		bool breakOut = false;

		// Do-While loop for iResult and its messages
		do
		{
			iResult = recv(clientSocket, buffer, BUFFERSIZE - 1, 0); // size of will let the complier pick size

			// If iResult is greater than 0, make the input equal to input plus the buffer
			if (iResult > 0)
			{
				input = input + (string)buffer;
			}
			// If iResult is equal to 0, close clientSocket
			else if (iResult == 0)
			{
				cout << "Connection closed\n" << endl;
				// Need to close clientSocket; listenSocket was already closed
				closesocket(clientSocket);
				breakOut = true;
			}
			// If iResult is less than 0, print a failed message
			else
			{
				cerr << "Recv failed with error: " << WSAGetLastError() << endl;
				closesocket(clientSocket);
				breakOut = true;
			}
		} while ((int)input.find("\r\n\r\n") < 0 && !breakOut);  // Finds a string of lines that are less than 0

		// Continue back to the loop
		if(breakOut == true)
		{
			continue;
		}

		// gets substring and finds HTTP					
		temp = input.substr(0, input.find(" HTTP") + 1);
		temp = temp.substr(5, temp.size() - 5);

		// checks if temp is " "  or "" and it is then temp is set to index.htm
		if (temp == " " || temp == "")
		{
			temp = "index.htm";
		}

		// used to see what http is and puts it into finalStr
		http = input.substr(input.find(" HTTP") + 1, input.find("\r\n") - (input.find(" HTTP") + 1));
		cout << temp << endl << http << endl;
		cout << input.substr(0, (int)input.find("\r\n\r\n") + 4) << endl;
		finalStr = ROOT + temp;

		// Failed code/page section
		outFile.open(ROOT + temp);
		if (outFile.fail())
		{
			outCode = 404;
			header = http;
			header += " 404 Not Found\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n";
			// 127.0.0.1/notFound.htm takes you to the 404 page
			outFile.open(ROOT + "notfound.htm");

			// Prints a 404 code if there is no page and exits
			if (outFile.fail())
			{
				cout << "ERROR: there is no 404 page." << endl;
				closesocket(clientSocket);
				cleanup();
				exit(1);
			}
		}

		// Sets outcode to 200 and makes the header equal to http
		else
		{
			outCode = 200;
			header = http;
			header += " 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n";
		}

		//Sends the header
		iResult = send(clientSocket, header.c_str(), (int)header.size(), 0);
		// If iResult is greater than 0, send a message with the Response Header
		if (iResult > 0)
		{
			cout << "Response Header: " << header << endl;
		}
		// If iResult equal to 0, Close connection
		else if (iResult == 0)
		{
			cout << "Connection closed.";
			closesocket(clientSocket);
			continue;
		}
		// If iResult is less than 0, print a error message
		else
		{
			cerr << "Send failed with error: " << WSAGetLastError() << endl;
			closesocket(clientSocket);
			continue;
		}

		// Breaks out of the loop
		breakOut = false; 

		// Sends file in blocks until file is empty 
		do
		{
			getline(outFile, data);
			if (data.size() > 0)
			{
				iResult = send(clientSocket, data.c_str(), (int)data.size(), 0);
				// If iResult is greater than 0, empty string
				if (iResult > 0)
				{
					cout << "";
				}
				// If iResult equal to 0, Close connection
				else if (iResult == 0)
				{
					cout << "Connection closed.\n";
					closesocket(clientSocket);
					breakOut = true;
				}
				// If iResult is less than 0, print a error message
				else
				{
					cerr << "Send failed with error: " << WSAGetLastError() << endl;
					closesocket(clientSocket);
					breakOut = true;
				}
			}
		} while (!outFile.eof() && !breakOut);	// While the current file is not closed it continues the loops
		
		outFile.close();

		// Continue back to the loop
		if (breakOut == true)
		{
			continue;
		}

		string finish = "\r\n\r\n";
		iResult = send(clientSocket, finish.c_str(), (int)finish.size(), 0);
		// If iResult is greater than 0, print empty string
		if (iResult > 0)
		{
			cout << "" << endl;
		}
		// If iResult equal to 0, Close connection
		else if (iResult == 0)
		{
			cout << "Connection closed." << endl;;
			closesocket(clientSocket);
			continue;
		}
		// If iResult is less than 0, print a error message
		else
		{
			cerr << "Send failed with error: " << WSAGetLastError() << endl;
			closesocket(clientSocket);
			continue;
		}
		handled++;
		cout << "Requests handled: " << handled << endl;
		closesocket(clientSocket);
	}

	// runs clean up and exits out of main 
	cleanup();
	return 0;
}


BOOL WINAPI Handler(DWORD cntrlEvent)
{
	cout << "Close event received\n";
	Sleep(1000);
	cleanup();
	return FALSE;
}


void cleanup()
{
	if (listenSocket != INVALID_SOCKET)
	{
		closesocket(listenSocket);
		listenSocket = INVALID_SOCKET;
	}

	WSACleanup();
	Sleep(100);
	cout << "Error setting server exit handler\n";
	cout << "Cleanup finished\n";
	Sleep(1200);
}