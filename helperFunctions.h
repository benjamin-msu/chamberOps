#pragma once
// program helper functions

#define LOG_FILE_DEFAULT ("chamberTurntable.log")
#define DATA_FILE_DEFAULT ("chamberTurntable.csv")

#define VISA_ADDRESS_TURNTABLE_AZIMUTH   ("GPIB0::18::INSTR")
#define VISA_ADDRESS_TURNTABLE_ELEVATION ("GPIB0::19::INSTR")
#define VISA_ADDRESS_SIGNAL_GENERATOR    ("GPIB0::20::INSTR")

#define DEVICE_STATUS_INDEX_TURNTABLE_AZIMUTH   (0)
#define DEVICE_STATUS_INDEX_TURNTABLE_ELEVATION (1)
#define DEVICE_STATUS_INDEX_SIGNAL_GENERATOR    (2)
#define DEVICE_STATUS_INDEX_FIELDFOX            (3)

#define VISA_COMMAND_CHECK_OPERATION_COMPLETE ("*OPC?\r\n")
#define TELNET_COMMAND_CHECK_OPERATION_COMPLETE ("*OPC?\r\n")

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <sys/timeb.h> // for ms time
#include <visa.h> //https://edadocs.software.keysight.com/connect/setting-up-a-visual-studio-c++-visa-project-482552069.html
#include <Windows.h> // for Beep()

#include "inputArgs.h"
#include "visaHelperFunctions.h"
#include "telnetHelperFunctions.h"

//inopvsy    aefAE   z
extern bool  progFlags[26];
extern std::string progFlagArgs[26];

extern ViSession globalVisaResourceManager;
extern ViStatus  globalVisaStatus;
extern std::string visaReceiveText;

extern ViSession visaTurntableAzimuthSession    = 0;
extern ViSession visaTurntableElevationSession  = 0;
extern ViSession visaSignalGeneratorSession     = 0;

extern ViRsrc turntableAzimuthRsrc = ViRsrc(VISA_ADDRESS_TURNTABLE_AZIMUTH);
extern ViRsrc turntableElevationRsrc = ViRsrc(VISA_ADDRESS_TURNTABLE_ELEVATION);
extern ViRsrc signalGeneratorRsrc = ViRsrc(VISA_ADDRESS_SIGNAL_GENERATOR);

extern WSADATA wsaDataConnection; // used for telent socket environment
extern SOCKET telnetFieldFox;
extern std::string telnetReceiveText;

// track status of each device
extern bool deviceConnectionStatus[4] = { false, false, false, false };

int statusTurntableAzimuth = -1;
int statusTurntableElevation = -1;
int statusSignalGenerator = -1;
int statusTelnetFieldFox = -1;

const char telnetFieldFoxIP[] = "192.168.0.1";
const char telnetBindIP[] = "192.168.0.2";
const int  telnetFieldFoxPort = 5024;

bool debugFlagIsSet() {
#ifdef DEBUG
	return true;
#endif // DEBUG
	return false;
}

int chamberTimestamp(std::string* timestampText) {
	// timestamp variables; will use to output linux format timestamps (easy to sort, since its only measured in seconds since Jan 1, 1970)
	time_t currentTime;
	struct tm* currentTimeInfo;
	std::stringstream ss;

	// get timestamp
	time(&currentTime);
	currentTimeInfo = gmtime(&currentTime);
	//gmtime_s(currentTimeInfo, &currentTime);  // double check functionality? not sure?
	currentTime = mktime(currentTimeInfo);

	// convert to string
	ss << currentTime;
	*(timestampText) = ss.str();
	return atoi(ss.str().c_str());
}

int chamberTimestamp() {
	std::string temp = "";
	return chamberTimestamp(&temp);
}

unsigned long int timestampMs() {
	struct timeb timeNow;
	ftime(&timeNow);
	return 1000 * timeNow.time + timeNow.millitm;
}


char ynPrompt(std::string prompt) {
	char ynPromptVal = 'q'; // default to nonsense value
	bool cinStatus = true;
	do {
		std::cin.clear();
		std::cout << prompt << "[y/n]";
		std::cin >> ynPromptVal;
		cinStatus = std::cin.fail();
		if (cinStatus) {
			std::cin.clear();
		}
	} while ((!cinStatus || std::cin.good()) && ynPromptVal != 'y' && ynPromptVal != 'n' && ynPromptVal != 'Y' && ynPromptVal != 'N');
	std::cin.clear();
	if (ynPromptVal == 'y' || ynPromptVal == 'Y') {
		return 'y';
	}
	else {
		return 'n';
	}
}

void errorOut(std::string mesg) { //suppressed conditionally; error may be placed in a log file
	if (progFlags[Z_FLAG_INDEX] && !progFlags[V_FLAG_INDEX]) {
		// print to log file instead of to screen
		std::ofstream logfile;
		// write to file
		logfile.open(LOG_FILE_DEFAULT, 'a');
		logfile << chamberTimestamp() << " " << mesg << std::endl;
		logfile.close();
	}
	else {
		if (progFlags[V_FLAG_INDEX]) {
			std::cerr << chamberTimestamp() << " ";
		}
		std::cerr << mesg << std::endl;
	}
}

bool interfaceOut(std::string mesg, bool needsAnswer) { // needed for getting user input
	if (needsAnswer) {
		errorOut("interfaceOut does not yet support 'needsAnswer' argument yet.");
		exit(-1);
	}

	if (progFlags[Y_FLAG_INDEX]) { // flags indicate no output
		return false;
	}
	else if (progFlags[Z_FLAG_INDEX]) { // must log output. However, program is being used in non-interactive mode. Still no output here
		return false;
	}
	else {
		std::cout << mesg << std::endl;
		return true;
	}
}
void debugOut(std::string mesg) {
#ifdef DEBUG
	std::cerr << mesg << std::endl;
#endif // DEBUG
#ifndef DEBUG
	return;
#endif
}

//bool interfaceIn() // handle z flag with a "default" value

bool dataOut(std::string mesg) { // sent to file, or to standard out, depending on settings
	std::ofstream outputFile;
	if (progFlags[O_FLAG_INDEX] && progFlagArgs[O_FLAG_INDEX] != "") {
		// write to file
		outputFile.open(progFlagArgs[O_FLAG_INDEX], std::ios_base::app); // append mode
		if (!outputFile.is_open()) { outputFile.close(); return false; }
		outputFile << mesg << std::endl;
		outputFile.close();
		return true;
	}
	else if (progFlags[O_FLAG_INDEX] && progFlagArgs[O_FLAG_INDEX] == "") {
		// no file provided
		errorOut("When using -o, please provide an output file.");
		return false;
	}
	else { // open the default file
		outputFile.open(DATA_FILE_DEFAULT, std::ios_base::app);
		if (!outputFile.is_open()) { outputFile.close(); return false; }
		outputFile << mesg << std::endl;
		outputFile.close();
		return true;
	}
}

void errorBeep() {
	Beep(800, 2000); // example was 523 hertz (C5) for 500 milliseconds
}

void infoBeep() {
	Beep(1200, 500); // example was 523 hertz (C5) for 500 milliseconds
}

// visa session setup
bool setupVisa() {
	bool errorFlag = false;
	// setup instrument connection managers
	visaInitResourceManager(&globalVisaResourceManager);

	// establish connection to instruments
	statusTurntableAzimuth = visaOpenInstrument(&globalVisaResourceManager, turntableAzimuthRsrc, &visaTurntableAzimuthSession);
	statusTurntableElevation = visaOpenInstrument(&globalVisaResourceManager, turntableElevationRsrc, &visaTurntableElevationSession);
	statusSignalGenerator = visaOpenInstrument(&globalVisaResourceManager, signalGeneratorRsrc, &visaSignalGeneratorSession);

	if (statusTurntableAzimuth != 0) {
		errorOut("Turntable (Azi) did not open properly.");
		deviceConnectionStatus[DEVICE_STATUS_INDEX_TURNTABLE_AZIMUTH] = false;
		errorFlag = true;
	} else {
		deviceConnectionStatus[DEVICE_STATUS_INDEX_TURNTABLE_AZIMUTH] = true;
	}
	if (statusTurntableElevation != 0) {
		errorOut("Turntable (Ele) did not open properly.");
		deviceConnectionStatus[DEVICE_STATUS_INDEX_TURNTABLE_ELEVATION] = false;
		errorFlag = true;
	} else {
		deviceConnectionStatus[DEVICE_STATUS_INDEX_TURNTABLE_ELEVATION] = true;
	}
	if (statusSignalGenerator != 0) {
		errorOut("Signal Generator did not open properly");
		deviceConnectionStatus[DEVICE_STATUS_INDEX_SIGNAL_GENERATOR] = false;
		errorFlag = true;
	} else {
		deviceConnectionStatus[DEVICE_STATUS_INDEX_SIGNAL_GENERATOR] = true;
	}

	// check error flag
	return !errorFlag;
}

//visa session cleanup
void cleanupVisa() {
	visaCloseInstrument(&visaTurntableAzimuthSession);     deviceConnectionStatus[DEVICE_STATUS_INDEX_TURNTABLE_AZIMUTH] = false;
	visaCloseInstrument(&visaTurntableElevationSession);   deviceConnectionStatus[DEVICE_STATUS_INDEX_TURNTABLE_ELEVATION] = false;
	visaCloseInstrument(&visaSignalGeneratorSession);      deviceConnectionStatus[DEVICE_STATUS_INDEX_SIGNAL_GENERATOR] = false;
	visaCloseResourceManager(&globalVisaResourceManager);
}

bool setupTelnet() {
	telnetInitWSA(&wsaDataConnection);
	statusTelnetFieldFox = telnetStartControl(&telnetFieldFox, telnetFieldFoxIP, telnetFieldFoxPort, telnetBindIP);
	if (statusTelnetFieldFox != 0) {
		deviceConnectionStatus[DEVICE_STATUS_INDEX_FIELDFOX] = false;
		errorOut("Fieldfox did not open properly.");
		return false;
	} else {
		// connection established
		deviceConnectionStatus[DEVICE_STATUS_INDEX_FIELDFOX] = true;
		unsigned long on = 1;
		unsigned long off = 0;
		ioctlsocket(telnetFieldFox, FIONBIO, &on); // disable socket blocking
		return true;
	}
}

void cleanupTelnet() {
	telnetStopControl(&telnetFieldFox);
	telnetCleanupWSA(&wsaDataConnection);
	deviceConnectionStatus[DEVICE_STATUS_INDEX_FIELDFOX] = false;
}

void printDeviceStatus() // check which instruments are connected, print status to console, then send data to setup instruments if required
{
	if (deviceConnectionStatus[DEVICE_STATUS_INDEX_TURNTABLE_AZIMUTH]) {
		interfaceOut("Turntable Azimuth:   CONNECTED", false);
	}
	else {
		interfaceOut("Turntable Azimuth:   DISCONNECTED", false);
	}

	if (deviceConnectionStatus[DEVICE_STATUS_INDEX_TURNTABLE_ELEVATION]) {
		interfaceOut("Turntable Elevation: CONNECTED", false);
	}
	else {
		interfaceOut("Turntable Elevation: DISCONNECTED", false);
	}

	if (deviceConnectionStatus[DEVICE_STATUS_INDEX_SIGNAL_GENERATOR]) {
		interfaceOut("Signal Generator:    CONNECTED", false);
	}
	else {
		interfaceOut("Signal Generator:    DISCONNECTED", false);
	}

	if (deviceConnectionStatus[DEVICE_STATUS_INDEX_FIELDFOX]) {
		interfaceOut("Field Fox:           CONNECTED", false);
	}
	else {
		interfaceOut("Field Fox:           DISCONNECTED", false);
	}
}