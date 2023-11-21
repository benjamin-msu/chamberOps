#pragma once
// fieldfox functions
#include <iostream>
#include <string>
#include <winsock.h>
#pragma comment(lib, "Ws2_32.lib")

#include "helperFunctions.h"
#include "telnetHelperFunctions.h"
#include <visa.h> //https://edadocs.software.keysight.com/connect/setting-up-a-visual-studio-c++-visa-project-482552069.html

// +/- 0.5 GHz, used to start estimating how far to either side of target frequency the bounds should be
#define DEFAULT_SPECTRUM_ANALYZER_RANGE_SCALE (500000000)
#define DEFAULT_SPECTRUM_ANALYZER_BW_RES (500000)
#define DEFAULT_SPECTRUM_ANALYZER_VIDEO_BW_RES (5000)
#define DEFAULT_SPECTRUM_ANALYZER_POINTS (1001)

extern ViSession globalVisaResourceManager;
extern ViStatus  globalVisaStatus;
extern std::string visaReceiveText;

extern bool deviceConnectionStatus[4]; // from helper functions

WSADATA wsaDataConnection; // used for telent socket environment
SOCKET telnetFieldFox;
std::string telnetReceiveText;

bool isSpectrumAnalyzerConnected() {
	return deviceConnectionStatus[DEVICE_STATUS_INDEX_FIELDFOX];
}
//telnetCommand(&telnetFieldFox, "SYST:PRES;*OPC?\r\n", &telnetReceiveText); // supposed to check status using *OPC?, waiting for a 1
bool isSpectrumAnalyzerReady() {
	telnetCommand(&telnetFieldFox, TELNET_COMMAND_CHECK_OPERATION_COMPLETE, &telnetReceiveText);
	if (telnetReceiveText == VISA_RESPONSE_TRUE || telnetReceiveText == VISA_RESPONSE_TRUE_2) {
		return true;
	}
	else if (telnetReceiveText == VISA_RESPONSE_FALSE || telnetReceiveText == VISA_RESPONSE_FALSE_2) {
		return false;
	}
	else {
		errorOut("Unreconized response from TELNET Device (FieldFox).");
		errorOut(telnetReceiveText);
		return false;
	}
}

int issueSpectrumAnalyzerCommand(std::string command, std::string* receivedText) {
	return telnetCommand(&telnetFieldFox, command, receivedText);
}

bool presetSpectrumAnalyzer() {
	issueSpectrumAnalyzerCommand("SYST:PRES;\r\n", &telnetReceiveText);
	return isSpectrumAnalyzerReady();
}

bool setSpectrumAnalyzerMode(std::string mode) { // SA, for example
	issueSpectrumAnalyzerCommand("INST:SEL '" + mode + "';\r\n", &telnetReceiveText);
	//std::cout << "mode output:" << telnetReceiveText << std::endl;
	return isSpectrumAnalyzerReady();
}

bool setSpectrumAnalyzerRangeStart(float freqInHz, int exponent) {
	issueSpectrumAnalyzerCommand("SENS:FREQ:START " + std::to_string(freqInHz) + "E" + std::to_string(exponent) + "Hz" + ";\r\n",&telnetReceiveText);
	return isSpectrumAnalyzerReady();
}

bool setSpectrumAnalyzerRangeStop(float freqInHz, int exponent) {
	//"FREQ " + std::to_string(freqInHz) + "E" + std::to_string(exponent) + "Hz" + "\r\n"
	issueSpectrumAnalyzerCommand("SENS:FREQ:STOP " + std::to_string(freqInHz) + "E" + std::to_string(exponent) + "Hz" + ";\r\n", &telnetReceiveText);
	return isSpectrumAnalyzerReady();
}

// bool setSpectrumAnalyzerRangeCenter()
// bool setSpectrumAnalyzerRangeSpan()

bool setSpectrumAnalyzerBandWResolution(float freqInHz, int exponent) {
	issueSpectrumAnalyzerCommand("SENS:BAND:RES " + std::to_string(freqInHz) + "E" + std::to_string(exponent) + "Hz" + ";\r\n", &telnetReceiveText);
	return isSpectrumAnalyzerReady();
}

bool setSpectrumAnalyzerBandWVideo(float freqInHz, int exponent) {
	issueSpectrumAnalyzerCommand("SENS:BAND:VID " + std::to_string(freqInHz) + "E" + std::to_string(exponent) + "Hz" + ";\r\n", &telnetReceiveText);
	return isSpectrumAnalyzerReady();
}

bool setSpectrumAnalyzerSweepPoints(int points) { // usually odd (ie, 401 or 1001) so there is a clear middle point
	issueSpectrumAnalyzerCommand("SENS:SWEEP:POINTS " + std::to_string(points) + ";\r\n", &telnetReceiveText);
	return isSpectrumAnalyzerReady();
}

bool setSpectrumAnalyzerCaptureModeContinuous(bool onIfTrue) {
	std::string status = "ON";
	if (!onIfTrue) {
		status = "OFF";
	}
	issueSpectrumAnalyzerCommand("INIT:CONT " + status + ";\r\n", &telnetReceiveText);
	return isSpectrumAnalyzerReady();
}

bool setSpectrumAnalyzerCaptureModeImmediate() { // single capture only, starting now; blocking until capture ends!!!
	issueSpectrumAnalyzerCommand("INIT:CONT OFF;\r\n", &telnetReceiveText);
	issueSpectrumAnalyzerCommand("INIT:IMM;\r\n", &telnetReceiveText);
	//issueSpectrumAnalyzerCommand("INIT:CONT ON;\r\n", &telnetReceiveText);
	return isSpectrumAnalyzerReady();
}

bool setSpectrumAnalyzerMarkerNormal(int markerNumber, double freq, int exponent) {
	if (markerNumber > 6 || markerNumber < 1) {
		errorOut("Can't activate marker number " + std::to_string(markerNumber) + " (out of range)!");
	}
	issueSpectrumAnalyzerCommand("CALC:MARK" + std::to_string(markerNumber) + " NORM;\r\n", &telnetReceiveText);
	issueSpectrumAnalyzerCommand("CALC:MARK1:X "+ std::to_string(freq) + "E" + std::to_string(exponent) + ";\r\n", &telnetReceiveText);
	return isSpectrumAnalyzerReady();
}

double getSpectrumAnalyzerMarkerValue(int markerNumber, std::string* textValue) { // gets a frequency value in text and number
	isSpectrumAnalyzerReady();
	if (markerNumber > 6 || markerNumber < 1) {
		errorOut("Can't read marker number " + std::to_string(markerNumber) + " (out of range)!");
	}
	issueSpectrumAnalyzerCommand("CALC:MARK" + std::to_string(markerNumber) + ":Y?;\r\n", &telnetReceiveText);
	*(textValue) = telnetReceiveText;

	isSpectrumAnalyzerReady(); // modifies telnetReceiveText, so must come after that value is read
	return strtod((*(textValue)).c_str(), nullptr);
}

bool setSpectrumAnalyzerTraceModeClearRewriteLive() {
	telnetCommand(&telnetFieldFox, "TRAC:TYPE CLRW;\r\n", &telnetReceiveText);
	return isSpectrumAnalyzerReady();
}

bool setSpectrumAnalyzerTraceModeMaxHold() { // will reset trace by switching to Clear/Rewrite first
	setSpectrumAnalyzerTraceModeClearRewriteLive();
	telnetCommand(&telnetFieldFox, "TRAC:TYPE MAXH;\r\n", &telnetReceiveText);
	return isSpectrumAnalyzerReady();
}

bool getSpectrumAnalyzerTraceData(std::string* traceData) { // returns full frequency sweep; could allow detecting the strongest frequency automatically
	issueSpectrumAnalyzerCommand("TRACE:DATA?;\r\n", &telnetReceiveText);
	*(traceData) = telnetReceiveText;
	return isSpectrumAnalyzerReady();
}
