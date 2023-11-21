#pragma once
// turntable controller
#include <iostream>
#include <string>

#define VISA_ADDRESS_TURNTABLE_AZIMUTH   ("GPIB0::18::INSTR")
#define VISA_ADDRESS_TURNTABLE_ELEVATION ("GPIB0::19::INSTR")

// default turntable limit ranges should be nonsense, so that not getting the real limits is obvious
#define TURNTABLE_LIMIT_DEFAULT_AZIMUTH_MIN (9999)
#define TURNTABLE_LIMIT_DEFAULT_AZIMUTH_MAX (-9999)
#define TURNTABLE_LIMIT_DEFAULT_ELEVATION_MIN (9999)
#define TURNTABLE_LIMIT_DEFAULT_ELEVATION_MAX (-9999)

//#define VISA_MAX_RESPONSE_SIZE (8192)
//#define VISA_SEND_DELAY (100)
//#define VISA_RECEIVE_TIMEOUT (1000)

#include "helperFunctions.h"
#include <visa.h> //https://edadocs.software.keysight.com/connect/setting-up-a-visual-studio-c++-visa-project-482552069.html
#include "chamber.h"

extern ViSession globalVisaResourceManager;
extern ViStatus  globalVisaStatus;
extern std::string visaReceiveText;

extern ViSession visaTurntableAzimuthSession;
extern ViSession visaTurntableElevationSession;

extern ViRsrc turntableAzimuthRsrc;
extern ViRsrc turntableElevationRsrc;

// setup in chamber.h
extern float turntableAziLimitMin = TURNTABLE_LIMIT_DEFAULT_AZIMUTH_MIN;
extern float turntableAziLimitMax = TURNTABLE_LIMIT_DEFAULT_AZIMUTH_MAX;
extern float turntableEleLimitMin = TURNTABLE_LIMIT_DEFAULT_ELEVATION_MIN;
extern float turntableEleLimitMax = TURNTABLE_LIMIT_DEFAULT_ELEVATION_MAX;

bool isTurntablePosValid(float azi, float ele) {
	if (isnan(azi) || isnan(ele)) { return false; }
	if (azi < turntableAziLimitMin || turntableAziLimitMax < azi) { return false; }
	if (ele < turntableEleLimitMin || turntableEleLimitMax < ele) { return false; }
	return true;
}

bool isTurntableAziConnected() {
	errorOut("Turntable (azimuth) not connected.");
	return false;
}
bool isTurntableEleConnected() {
	errorOut("Turntable (elevation) not connected.");
	return false;
}
bool isTurntableConnected() { //checks both axis
	return isTurntableAziConnected() && isTurntableEleConnected();
}
bool isTurntableAziReady() {
	visaCommand(&visaTurntableAzimuthSession, VISA_COMMAND_CHECK_OPERATION_COMPLETE, &visaReceiveText);
	if (visaReceiveText == VISA_RESPONSE_TRUE || visaReceiveText == VISA_RESPONSE_TRUE_2) {
		return true;
	} else if (visaReceiveText == VISA_RESPONSE_FALSE || visaReceiveText == VISA_RESPONSE_FALSE_2) {
		return false;
	} else {
		errorOut("Unreconized response from VISA Device (Turntable-Azimuth).");
		errorOut(visaReceiveText);
		return false;
	}
}
bool isTurntableEleReady() {
	visaCommand(&visaTurntableElevationSession, VISA_COMMAND_CHECK_OPERATION_COMPLETE, &visaReceiveText);
	if (visaReceiveText == VISA_RESPONSE_TRUE || visaReceiveText == VISA_RESPONSE_TRUE_2) {
		return true;
	} else if (visaReceiveText == VISA_RESPONSE_FALSE || visaReceiveText == VISA_RESPONSE_FALSE_2) {
		return false;
	} else {
		errorOut("Unreconized response from VISA Device (Turntable-Elevation).");
		errorOut(visaReceiveText);
		return false;
	}
}
bool isTurntableReady() { // only works well after another command - will always return zero if no command has run yet
	bool statusAzi = isTurntableAziReady();
	//Sleep(500);
	bool statusEle = isTurntableEleReady();
	//debugOut(std::to_string(statusAzi) + ", " + std::to_string(statusEle));
	return statusAzi && statusEle;
}

double getTurntableAziPosition(std::string* textValue) {
	visaCommand(&visaTurntableAzimuthSession, "CP\r\n", &visaReceiveText);
	*(textValue) = visaReceiveText;
	return strtod((*(textValue)).c_str(), nullptr);
}

double getTurntableElePosition(std::string* textValue) {
	visaCommand(&visaTurntableElevationSession, "CP\r\n", &visaReceiveText);
	*(textValue) = visaReceiveText;
	return strtod((*(textValue)).c_str(), nullptr);
}

bool getTurntableSoftLimitsAzi() {
	visaCommand(&visaTurntableAzimuthSession, "UL\r\n", &visaReceiveText); // Upper limit
	turntableAziLimitMax = strtod(visaReceiveText.c_str(), nullptr);
	visaCommand(&visaTurntableAzimuthSession, "LL\r\n", &visaReceiveText); // Lower limit
	turntableAziLimitMin = strtod(visaReceiveText.c_str(), nullptr);
	return true;
}

bool getTurntableSoftLimitsEle() {
	visaCommand(&visaTurntableElevationSession, "UL\r\n", &visaReceiveText); // Upper limit
	turntableEleLimitMax = strtod(visaReceiveText.c_str(), nullptr);
	visaCommand(&visaTurntableElevationSession, "LL\r\n", &visaReceiveText); // Lower limit
	turntableEleLimitMin = strtod(visaReceiveText.c_str(), nullptr);
	return true;
}

bool getTurntableSoftLimits() {
	getTurntableSoftLimitsAzi();
	getTurntableSoftLimitsEle();
	return true;
}

bool setTurntableAziPosition(float pos) { // use with isTurntableReady()
	if (turntableAziLimitMin == TURNTABLE_LIMIT_DEFAULT_AZIMUTH_MIN || turntableAziLimitMax == TURNTABLE_LIMIT_DEFAULT_AZIMUTH_MAX) {
		errorOut("Turntable limits were not read - setTurntableAziPosition() may fail");
	}
	if (!(turntableAziLimitMin <= pos && pos <= turntableAziLimitMax)) {
		errorOut("azimuth limits exceeded by value " + std::to_string(pos));
		return false;
	}
	visaSend(&visaTurntableAzimuthSession, "GOTO " + std::to_string((int)pos) + "." + std::to_string((pos-(int)pos)*100) + "\r\n");
	return true;
}

bool setTurntableElePosition(float pos) { // use with isTurntableReady()
	if (turntableEleLimitMin == TURNTABLE_LIMIT_DEFAULT_ELEVATION_MIN || turntableEleLimitMax == TURNTABLE_LIMIT_DEFAULT_ELEVATION_MAX) {
		errorOut("Turntable limits were not read - setTurntableElePosition() may fail");
	}
	if (!(turntableEleLimitMin <= pos && pos <= turntableEleLimitMax)) {
		errorOut("elevation limits exceeded by value " + std::to_string(pos));
		return false;
	}
	visaSend(&visaTurntableElevationSession, "GOTO " + std::to_string((int)pos) + "." + std::to_string((pos - (int)pos) * 100) + "\r\n");
	return true;
}

bool moveTurntable(float aziPos, float elePos) { // blocks until turntable is finished moving
	setTurntableAziPosition(aziPos);
	setTurntableElePosition(elePos);
	while (!(isTurntableReady()));
	return true;
}

// untested
/*
getTurntablePosition()

*/