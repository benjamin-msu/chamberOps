#pragma once
// signal generator
#define VISA_ADDRESS_SIGNAL_GENERATOR    ("GPIB0::20::INSTR")

#define VISA_COMMAND_SIGNAL_GENERATOR_GET_POWER     ("POW?\r\n")
#define VISA_COMMAND_SIGNAL_GENERATOR_GET_FREQUENCY ("FREQ?\r\n")

#define FREQUENCY_KHZ (3)
#define FREQUENCY_MHZ (6)
#define FREQUENCY_GHZ (9)

// double check these!
#define SIG_GEN_MIN_POWER (-30)
#define SIG_GEN_MAX_POWER (5)

#include <iostream>
#include <string>

#include <visa.h> //https://edadocs.software.keysight.com/connect/setting-up-a-visual-studio-c++-visa-project-482552069.html
#include "helperFunctions.h"
#include "chamber.h"


extern ViSession globalVisaResourceManager;
extern ViStatus  globalVisaStatus;
extern std::string visaReceiveText;

extern ViSession visaSignalGeneratorSession;
extern ViRsrc signalGeneratorRsrc;

extern bool deviceConnectionStatus[4];


bool isSignalGenConnected() {
	return deviceConnectionStatus[DEVICE_STATUS_INDEX_SIGNAL_GENERATOR];
}
bool isSignalGenReady() {
	//visaClearReceiveBuffer(&visaSignalGeneratorSession);
	visaCommand(&visaSignalGeneratorSession, VISA_COMMAND_CHECK_OPERATION_COMPLETE, &visaReceiveText);
	if (visaReceiveText == VISA_RESPONSE_TRUE || visaReceiveText == VISA_RESPONSE_TRUE_2) {
		return true;
	} else if (visaReceiveText == VISA_RESPONSE_FALSE || visaReceiveText == VISA_RESPONSE_FALSE_2) {
		return false;
	} else {
		errorOut("Unreconized response from VISA Device (SignalGenerator).");
		errorOut(visaReceiveText);
		return false;
	}
}
bool resetSignalGen() {
	visaSend(&visaSignalGeneratorSession, "*RST\r\n");
	return isSignalGenReady();
}
float getSignalGenPower() { // in dBm
	visaCommand(&visaSignalGeneratorSession, VISA_COMMAND_SIGNAL_GENERATOR_GET_POWER, &visaReceiveText);
	double d = strtod(visaReceiveText.c_str(), nullptr);
	return (float)d;
}
int getSignalGenFreq() { // in Hz
	visaCommand(&visaSignalGeneratorSession, VISA_COMMAND_SIGNAL_GENERATOR_GET_FREQUENCY, &visaReceiveText);
	double d = strtod(visaReceiveText.c_str(), nullptr);
	return (int)d;
}
bool setSignalGenPower(float powerInDB) {
	visaSend(&visaSignalGeneratorSession, "POW " + std::to_string(powerInDB) + "dBm" + "\r\n");
	return isSignalGenReady();
}
bool setSignalGenFreq(double freqInHz, int exponent) {
	visaSend(&visaSignalGeneratorSession, "FREQ " + std::to_string(freqInHz) + "E" + std::to_string(exponent) + "Hz" + "\r\n");
	return isSignalGenReady();
}
bool setSignalGenModOn() {
	visaSend(&visaSignalGeneratorSession, "OUTP:MOD ON\r\n");
	return isSignalGenReady();
}
bool setSignalGenModOff() {
	visaSend(&visaSignalGeneratorSession, "OUTP:MOD OFF\r\n");
	return isSignalGenReady();
}
bool setSignalGenMod(bool modStatus) { // on or off, but will always use off
	if (modStatus) {
		return setSignalGenModOn();
	} else {
		return setSignalGenModOff();
	}
}
bool setSignalGenOn() {
	visaSend(&visaSignalGeneratorSession, "OUTPUT ON\r\n");
	return isSignalGenReady();
}
bool setSignalGenOff() {
	visaSend(&visaSignalGeneratorSession, "OUTPUT OFF\r\n");
	return isSignalGenReady();
}
bool setSignalGenState(bool powerStatus) {
	if (powerStatus) {
		return setSignalGenOn();
	} else {
		return setSignalGenOff();
	}
}
