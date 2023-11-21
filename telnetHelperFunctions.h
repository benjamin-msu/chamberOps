#pragma once

#define TELNET_SEND_BUFFER_SIZE (128)
#define TELNET_RECEIVE_BUFFER_SIZE (2048)
#define TELNET_SEND_DELAY (500)
#define TELNET_RECEIVE_TIMEOUT (5000)
#define TELNET_EXPECTED_PROMPT ("\r\nSCPI> ")

extern WSADATA wsaDataConnection; // used for telent socket environment
extern SOCKET telnetFieldFox;
extern std::string telnetReceiveText;

const std::string telnetExpExtra = TELNET_EXPECTED_PROMPT;

int telnetInitWSA(WSADATA* wsaDataConnection) {
	return WSAStartup(MAKEWORD(2, 2), wsaDataConnection); // return 0 if no error
}

int telnetCleanupWSA(WSADATA* wsaDataConnection) {
	return WSACleanup();
}

int telnetStopControl(SOCKET* socketObj) {
	return closesocket((*socketObj));
}

int telnetSend(SOCKET* socketObj, std::string command) {
	// end lines with \r\n
	//char buf[TELNET_SEND_BUFFER_SIZE];
	const char* buf = command.c_str();
	int result = send((*socketObj), buf, command.length(), 0);
	Sleep(TELNET_SEND_DELAY);
	return result;
}
int telnetRecv(SOCKET* socketObj, std::string* receivedText) {
	//(*receivedText) = "Hello!\r\n";
	char buf[TELNET_RECEIVE_BUFFER_SIZE];
	// initialize buf to be all string terminators
	for (int i= 0; i < TELNET_RECEIVE_BUFFER_SIZE; i++) {
		buf[i] = '\0';
	}

	int bytesReceived = 0;
	// in future, configure timeout
	bytesReceived = recv((*socketObj), buf, TELNET_RECEIVE_BUFFER_SIZE, 0);
	//std::cout << bytesReceived << std::endl;
	if (bytesReceived > 0 && bytesReceived + 1 <= TELNET_RECEIVE_BUFFER_SIZE) {
		buf[bytesReceived] = '\0'; // terminate the c string
	}
	else {
		buf[TELNET_RECEIVE_BUFFER_SIZE - 1] = '\0';
	}
	(*receivedText) = buf;
	return (int)(bytesReceived == 0); // return 0 if bytes received
}

int telnetCommand(SOCKET* socketObj, std::string command, std::string* receivedText) {
	int status;
	size_t nextPromptSubStringLocation = std::string::npos;
	std::string tempString = "";
	std::string workspace = ""; // the total response, until end of response is reached
	bool endOfTransmission = false;
	bool timeoutExceeded = false;
	SYSTEMTIME currentTime, startTime;
	telnetSend(socketObj, command);
	GetSystemTime(&startTime);
	do
	{
		status = telnetRecv(socketObj, &tempString);
		if (status == 0) { // did receive
			GetSystemTime(&startTime);
			workspace.append(tempString);
			//std::cout << "Current workspace is: " << workspace << std::endl; // has two sets of newlines - from command, and by fieldfox for result
			nextPromptSubStringLocation = workspace.find(TELNET_EXPECTED_PROMPT);
			if (nextPromptSubStringLocation != std::string::npos) {
				//std::cout << "Did Find!" << std::endl;
				endOfTransmission = true;
			}
		}
		GetSystemTime(&currentTime);
		tempString = "";
		timeoutExceeded = (currentTime.wMilliseconds - startTime.wMilliseconds > TELNET_RECEIVE_TIMEOUT);
	} while (!(endOfTransmission || timeoutExceeded));

	if (endOfTransmission) {
		(*receivedText) = workspace.substr(command.length(), workspace.length() - command.length() - telnetExpExtra.length());
	}
	else {
		return 2; // we did receive something, just timed out
	}
	return status;
}

// used in telnetStartControl()
int telnetClearReceiveBuffer(SOCKET* socketObj) {
	std::string tempString;
	telnetRecv(socketObj, &tempString);
	return 0;
}

int telnetStartControl(SOCKET* socketObj, const char* targetIP, int targetPort, const char* bindIP) {
	sockaddr_in bindAddr, targetAddr;
	(*socketObj) = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ((*socketObj) == INVALID_SOCKET) {
		return 1; // couldn't make the socket... should end program
	}
	// now, bind to an address - must be a configured address on your computer
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = 0; // winsock will assign between 1024 and 5000
	bindAddr.sin_addr.S_un.S_addr = inet_addr(bindIP);
	if (bind((*socketObj), (sockaddr*)(&bindAddr), sizeof(bindAddr)) != 0) {
		return 2; // couldn't bind the address... should end program
	}
	// now, connect to target ip address
	targetAddr.sin_family = AF_INET;
	targetAddr.sin_port = htons(targetPort);
	targetAddr.sin_addr.S_un.S_addr = inet_addr(targetIP);
	if (connect((*socketObj), (sockaddr*)(&targetAddr), sizeof(targetAddr)) != 0) {
		return 3; // could't connect to target ip... should end program
	}
	// all steps successful
	Sleep(TELNET_SEND_DELAY); // to allow response time if immediate read is needed
	telnetClearReceiveBuffer(socketObj); // clear the welcome message at the top of the session, so that command responses are processed properly
	telnetCommand(socketObj, "\r\n",&telnetReceiveText); // to reset the line prompt on received text
	Sleep(TELNET_SEND_DELAY);
	return 0;
}
