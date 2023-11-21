#pragma once
// visa helper functions

#include <visa.h> //https://edadocs.software.keysight.com/connect/setting-up-a-visual-studio-c++-visa-project-482552069.html

#define VISA_MAX_RESPONSE_SIZE (8192)
#define VISA_SEND_DELAY (100)
#define VISA_RECEIVE_TIMEOUT (1000)

#define VISA_RESPONSE_TRUE  ("1")
#define VISA_RESPONSE_FALSE ("0")
#define VISA_RESPONSE_TRUE_2  (" 1")
#define VISA_RESPONSE_FALSE_2 (" 0")

extern ViSession globalVisaResourceManager = 0;
extern ViStatus  globalVisaStatus = 0;
extern std::string visaReceiveText = "";

int visaInitResourceManager(ViSession* resourceManager) {
	ViStatus status = 0;
	status = viOpenDefaultRM(resourceManager);
	if (status < VI_SUCCESS) {
		std::cerr << "There was a problem with the visa default resource manager. ";
		std::cerr << "Error Code: " << status << std::endl;
	}
	return status;
}
int visaCloseResourceManager(ViSession* resourceManager) {
	ViStatus status = 0;
	status = viClose((*resourceManager));
	if (status < VI_SUCCESS) {
		std::cerr << "There was a problem closing the visa default resource manager. ";
		std::cerr << "Error Code: " << status << std::endl;
	}
	return status;
}
int visaOpenInstrument(ViSession* resourceManager, ViRsrc visaAddress, ViSession* instSession) {
	ViStatus status = 0;
	ViChar fullAddress[100];
	status = viOpen((*resourceManager), visaAddress, VI_NO_LOCK, VISA_RECEIVE_TIMEOUT, instSession);
	if (status < VI_SUCCESS) {
		std::cerr << "There was a problem opening instrument " << visaAddress << " ";
		std::cerr << "Error Code: " << status << std::endl;
		return 1;
	}
	// For Serial and TCP/IP connections, enable read Termination Char or it read will timeout
	viGetAttribute((*instSession), VI_ATTR_RSRC_NAME, fullAddress);
	if ((strstr(fullAddress, "ASRL")) || (strstr(fullAddress, "SOCKET"))) {
		viSetAttribute((*instSession), VI_ATTR_TERMCHAR_EN, VI_TRUE);
	}
	Sleep(VISA_SEND_DELAY);
	return 0;
}
int visaCloseInstrument(ViSession* instSession) {
	ViStatus status = 0;
	viClose((*instSession));
	if (status < VI_SUCCESS) {
		std::cerr << "There was a problem closing instrument." << " ";
		std::cerr << "Error Code: " << status << std::endl;
		return 1;
	}
	return 0;
}
int visaSend(ViSession* instSession, std::string command) {
	ViStatus status = 0;
	ViConstString tempCommand = command.c_str();
	status = viPrintf((*instSession), tempCommand);
	if (status < VI_SUCCESS) {
		std::cerr << "There was a problem writing instrument command \""
			<< command << "\" " << "Error Code: " << status << std::endl;
		return 1;
	}
	Sleep(VISA_SEND_DELAY);
	return 0;
}
int visaRecv(ViSession* instSession, std::string* receivedText) {
	ViStatus status = 0;
	ViChar idnResponse[VISA_MAX_RESPONSE_SIZE];
	int responseLength = 0;
	//idnResponse[VISA_MAX_RESPONSE_SIZE - 1] = "\0"; // null terminate, just in case
	// replace with a safer scan function?
	status = viScanf((*instSession), "%t", idnResponse);
	if (status < VI_SUCCESS) {
		std::cerr << "There was a problem reading instrument" << " "
			<< "Error Code: " << status << std::endl;
		return 1;
	}
	(*receivedText) = idnResponse;
	responseLength = (int)(*receivedText).length();
	//here, I remove trailing enter... probobly should be handled by a modifier to results in main?
	if (responseLength > 1) {
		(*receivedText) = (*receivedText).substr(0, responseLength - 1);
	}
	//std::cout << "visaRecv got length " << (*receivedText).length() << " string: " << (*receivedText) << std::endl;
	return 0;
}
int visaCommand(ViSession* instSession, std::string command, std::string* receivedText) {
	ViStatus status = 0;
	status = visaSend(instSession, command);
	if (status != 0) {
		std::cerr << "visaCommand::Error in send..." << std::endl;
	}
	status = visaRecv(instSession, receivedText);
	if (status != 0) {
		std::cerr << "###### visaCommand::Error in receive...\n### Command was " << command << std::endl;
	}
	return status;
}
int visaClearReceiveBuffer(ViSession* instSession) {
	std::string tempString;
	return visaRecv(instSession, &tempString);
}