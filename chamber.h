#pragma once
// anechoic chamber
// program support functions

// disable warnings for time.h - had to go into project properties > C/C++ and turn off SDL checks
//#define _CRT_SECURE_NO_WARNINGS

#define SAVE_STATE_FILE_DEFAULT (".chamber-savestate.dat")

#define POSITION_AZIMUTH_INDEX (0)
#define POSITION_ELEVATION_INDEX (1)
#define SIGNAL_FREQUENCY_INDEX (0)
#define SIGNAL_POWER_INDEX (1)

// in ms
#define DEFAULT_TURNTABLE_MOVEMENT_ESTIMATE (4000)
#define DEFAULT_MEASUREMENT_TIME (10000)
#define MINIMUM_MEASUREMENT_TIME (1000)

#include "helperFunctions.h"
#include "inputArgs.h"
#include "signalGenerator.h"
#include "fieldfox.h"
#include "turntable.h"
#include "visaHelperFunctions.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <Windows.h> // for Beep(), and signal handling function

//inopvsy    aefAE   z
extern bool  progFlags[26];
extern std::string progFlagArgs[26];

// from turntable.h
extern float turntableAziLimitMin;
extern float turntableAziLimitMax;
extern float turntableEleLimitMin;
extern float turntableEleLimitMax;

// provide configurable measurement time, and fieldfox options
int spectrumAnalyzerMeasurementTime = DEFAULT_MEASUREMENT_TIME;

struct testPosition {
	float elevation;
	float azimuth;
	long long frequency; // need range to cover high GHz values, like 12GHz
	int power;
};

// provide other files with a "should save and close" functionality
bool saveAndCloseFlag = false;

// function declarations
void saveSweepState(testPosition* positions, int totalPositions, int nextIndex); // for sweep mode

bool shouldSaveAndClose() {
	return saveAndCloseFlag;
}

// settings from the hardware - struct of hardware

bool positionIsValid(float azimuth, float elevation) { //make sure position is inside the hard / soft limits
	return isTurntablePosValid(azimuth, elevation);
	//errorOut("Position checking will require data from the turntable. Not yet implemented.");
	//return false;
	//exit(-1);
}
//testNextPosition() //tracks index in the position list
//testPosition(n)

// used to create sweep params - double pointer neccessary because we are modifying the address value of the incoming "testPosition* positions" variable
int createPositionTargets(testPosition** positions, long long targetFreq, int targetPower, float elevationMin, float elevationMax, 
							float azimuthMin, float azimuthMax, float aziDensity, float elevDensity) {
	// calculate number of positions - num of trials, in 2d
	int numAziPositions = 0;
	int numElePositions = 0;
	int totalPositions = 0;
	
	float rangeAzimuth   = azimuthMax   - azimuthMin;
	float rangeElevation = elevationMax - elevationMin;

	if (rangeAzimuth < 0 || rangeElevation < 0) {
		errorOut("Elevation and Azimuth ranges must be valid.");
		exit(-1);
	}
	if (aziDensity == 0 || elevDensity == 0) {
		errorOut("Density of elevation or density must not be zero.");
		exit(-1);
	}
	//correct density if it is larger than the range
	if (aziDensity > rangeAzimuth) {
		aziDensity = rangeAzimuth;
	}
	if (elevDensity > rangeElevation) {
		elevDensity = rangeElevation;
	}
	// in case of single value max/min, correct size to 1
	if (rangeAzimuth == 0) {
		numAziPositions = 1;
	} else {
		numAziPositions = (int)round(rangeAzimuth / aziDensity) + 1; // add one, since 4 sections doesn't account for a mid point
	}
	if (rangeElevation == 0) {
		numElePositions = 1;
	} else {
		numElePositions = (int)round(rangeElevation / elevDensity) + 1; // add one, since 4 sections doesn't account for a mid point
	}
	
	totalPositions  = numAziPositions * numElePositions;

	// create positions
	*positions = new testPosition[totalPositions];

	for (int i = 0; i < totalPositions; i++) {
		// triangle wave code based on Lightsider's answer to
		// https://stackoverflow.com/questions/1073606/is-ther-a-one-line-function-that-generates-a-triangle-wave
		//float triAmplitude = rangeAzimuth;
		//int triHalfPeriod = numAziPositions;
		//float newAzimuth = azimuthMin + (triAmplitude/triHalfPeriod) * (triHalfPeriod - abs(i % (2*triHalfPeriod) - triHalfPeriod));
		int numPositionsPerAziSweep = ((int)round(rangeAzimuth / aziDensity) + 1);
		float newAzimuth   = (azimuthMin + aziDensity *(i % numPositionsPerAziSweep))
			*( (numPositionsPerAziSweep <= i%(2*numPositionsPerAziSweep)) ? (1) : (-1) ); // multiply by 1 or -1; allows sweeping backwards and forwards
		float newElevation = elevationMin + elevDensity * (i / numAziPositions);
		//float newAzimuth = azimuthMin + aziDensity*( i%( (int)round(rangeAzimuth/aziDensity) + 1) ); // original behavior; sweep only 1 direction
		
		positionIsValid(newAzimuth,newElevation);
		//positions[i] = new float [2];
		//positions[i][POSITION_AZIMUTH_INDEX]   = newAzimuth;
		//positions[i][POSITION_ELEVATION_INDEX] = newElevation;
		(*positions)[i].azimuth   = newAzimuth;
		(*positions)[i].elevation = newElevation;
		(*positions)[i].frequency = targetFreq;
		(*positions)[i].power = targetPower;
	}
	return totalPositions;
}

void printPositionsTable(testPosition* positions, int totalPositions) {
	// change to interfaceOut() function
	for (int i = 0; i < totalPositions; i++) {
		interfaceOut(std::to_string(positions[i].azimuth) + ","
					+ std::to_string(positions[i].elevation) + ","
					+ std::to_string(positions[i].frequency) + ","
					+ std::to_string(positions[i].power), false);
		/*
		std::cout << positions[i].azimuth << ","
			<< positions[i].elevation << ","
			<< positions[i].frequency << ","
			<< positions[i].power << std::endl;
		*/
	}
}

void cleanupPositionTargets(testPosition* positions) {
	// removed argument , int totalPositions
	//int arrLength = positions
	//for (int i = 0; i < totalPositions; i++) {
		//delete[] positions[i];
	//}
	delete[] positions;
}

bool verifyDevicesReady(bool* statusSpectrumAnalyzer, bool* statusSignalGen, bool* statusTurntableAzi, bool* statusTurntableEle) {
	// don't check "connected", that variable may not be regularly updated...
	*(statusSpectrumAnalyzer) = isSpectrumAnalyzerReady();
	*(statusSignalGen) = isSignalGenReady();
	*(statusTurntableAzi) = isTurntableAziReady();
	*(statusTurntableEle) = isTurntableEleReady();
	return *(statusSpectrumAnalyzer) && *(statusSignalGen) && *(statusTurntableAzi) && *(statusTurntableEle);
}

bool verifyDevicesReady() {
	bool temp1, temp2, temp3, temp4;
	return verifyDevicesReady(&temp1, &temp2, &temp3, &temp4);
}

bool initiateDevices() {
	// setup device connections
	bool visaSuccess = setupVisa();
	bool turntableLimitsSuccess = getTurntableSoftLimits();
	bool fieldFoxSuccess = setupTelnet();
	if (getProgFlag(V_FLAG_INDEX)) {
		printDeviceStatus();
		if (deviceConnectionStatus[DEVICE_STATUS_INDEX_FIELDFOX] != true) {
			errorOut("It seems that the fieldfox is unplugged. Has this computer configured the following network connection, for direct LAN access?\
					\nIP: 192.168.0.2 ---------- Subnet mask: 255.255.248.0");
		}
	}
	return visaSuccess && turntableLimitsSuccess && fieldFoxSuccess && verifyDevicesReady();
}

void interactiveModeStart() { //until quit, display a user interface and allow interaction
	errorOut("Interactive mode not yet implemented.");
	exit(-1);
}

bool sweepModeStart(testPosition* positions, int* totalPositions, int* nextIndex) { // returns true if sweep was finished to the end
	int numMeasurementsDesired = 3; // how many iterations of the fieldfox scan should we wait for?
	
	unsigned long int startTime   = 0;
	unsigned long int endTime     = 0;
	unsigned long int elapsedTime = 0;
	long int movementStartTimestamp = 0;
	long int movementEndTimestamp   = 0;
	long int movementAverageTime    = DEFAULT_TURNTABLE_MOVEMENT_ESTIMATE; // moving average; A_(n+1) =  A_(n) + ( x_(n+1) + n*A_(n) ) / ( n + 1 )
	int remainingPositions = *totalPositions - *nextIndex;
	unsigned long int remainingTimeEstimate = 0;

	// estimate measurement times more accurately
	interfaceOut("Estimating Spectrum Analyzer measurement time...", false);
	setSpectrumAnalyzerTraceModeClearRewriteLive();
	startTime = timestampMs();
	setSpectrumAnalyzerCaptureModeImmediate(); // blocking call
	endTime = timestampMs(); // in seconds
	setSpectrumAnalyzerCaptureModeContinuous(true); // restore normal operation time
	setSpectrumAnalyzerTraceModeMaxHold();
	spectrumAnalyzerMeasurementTime = (endTime - startTime) * numMeasurementsDesired;
	// enforce a minimum scan time of 5 seconds
	spectrumAnalyzerMeasurementTime = (spectrumAnalyzerMeasurementTime > MINIMUM_MEASUREMENT_TIME) ?
																	 spectrumAnalyzerMeasurementTime : MINIMUM_MEASUREMENT_TIME;
	// reset timers
	startTime = 0;
	endTime = 0;

	// estimate time of each measurement, seperate from the movement of the turntable
	remainingTimeEstimate = remainingPositions * (spectrumAnalyzerMeasurementTime + movementAverageTime + 6000); // constant for I/O

	// print current values and confirm startup
	interfaceOut("Sweep mode Current Settings:",false);
	printPositionsTable(positions, (*totalPositions > 10 ? 10 : *totalPositions));
	interfaceOut("Total Positions: " + std::to_string(*totalPositions) + 
		((*nextIndex != 0)? "(starting at position " + std::to_string(*nextIndex+1) + ")" : ""), false);
	interfaceOut("Time of each SA measurement: " + std::to_string(spectrumAnalyzerMeasurementTime/1000)+" seconds", false);
	interfaceOut("Time Estimate:   " + std::to_string(remainingTimeEstimate/1000/60) + " minutes " 
		                             + std::to_string(remainingTimeEstimate/1000%60) + " seconds", false);
	infoBeep();
	if ('n' == ynPrompt("Proceed with these values?")) {
		if ('y' == ynPrompt("Would you like to savestate your current settings, for inspection?")) {
			saveSweepState(positions, *totalPositions, *nextIndex);
			interfaceOut("Savestate created.", false);
		} else {
			interfaceOut("Savestate will not be created.", false);
		}
		exit(-1);
	}
	interfaceOut("Data format is | timestamp, azimuth, elevation, frequency, powerTx, and powerRx",false);

	// move turntable to initial position before starting loop
	/*
	for (bool warningIssued = false; !isTurntableReady(); warningIssued = true) {
		if (!warningIssued) { errorOut("Turntable not responding for first move..."); }
		Sleep(500);
	}
	*/
	moveTurntable(0.9, -0.9);
	moveTurntable(positions[0].azimuth, positions[0].elevation);

	startTime = timestampMs(); // used to determine total elapsed time at end

	while (*(nextIndex) < *(totalPositions) && !shouldSaveAndClose()) { // ctrl+c will trigger shouldSaveAndClose()
		// need timestamp, azi, ele, frequency, powerTx, and powerRx on each iteration
		// make sure all the devices are ready
		for (bool warningIssued = false; !verifyDevicesReady(); warningIssued = true) {
			if (!warningIssued) { errorOut("Some instruments are not responding..."); }
			errorBeep();
			Sleep(10000);
		}

		// move turntable
		movementStartTimestamp = timestampMs();
		moveTurntable(positions[*(nextIndex)].azimuth, positions[*(nextIndex)].elevation);
		movementEndTimestamp = timestampMs();

		// reset max hold on spectrum analyzer
		setSpectrumAnalyzerTraceModeClearRewriteLive();
		setSpectrumAnalyzerTraceModeMaxHold();

		setSignalGenOn();
		Sleep(spectrumAnalyzerMeasurementTime);
		setSignalGenOff();

		// take measurement - from all instruments // timestamp,azi,ele,freq,pTx,pRx
		int dataTimestamp = chamberTimestamp(); // should be in seconds
		std::string dataAziTxt   = "";
		std::string dataEleTxt   = "";
		std::string dataFreqTxt  = "";
		std::string dataPowTxTxt = "";
		std::string dataPowRxTxt = "";
		double dataAzi   = getTurntableAziPosition(&dataAziTxt);
		double dataEle   = getTurntableElePosition(&dataEleTxt);
		double dataFreq  = getSignalGenFreq();
		dataFreqTxt = std::to_string(dataFreq);
		double dataPowTx = getSignalGenPower();
		dataPowTxTxt = std::to_string(dataPowTx);
		double dataPowRx = getSpectrumAnalyzerMarkerValue(1, &dataPowRxTxt);

		// update values for next loop (not index yet)
		// moving average; A_(n+1) =  A_(n) + ( x_(n+1) + n*A_(n) ) / ( n + 1 )
		int n = *(nextIndex) + 1; // add 1 so it will never be zero
		movementAverageTime = movementAverageTime + (((movementEndTimestamp - movementStartTimestamp) - movementAverageTime) / (n + 1));
		remainingPositions = *totalPositions - (*nextIndex+1);
		remainingTimeEstimate = remainingPositions * (spectrumAnalyzerMeasurementTime + movementAverageTime + 6000); // constant accounts for some I/O

		// output data (to file and to console)
		std::string dataToConsole = "[" + std::to_string(*(nextIndex)+1)+"/" + std::to_string(*(totalPositions)) + "] "
										+ (std::to_string(remainingTimeEstimate / 1000 / 60)) + " min "
										+ (std::to_string(remainingTimeEstimate / 1000 % 60)) + " sec left | "
										+ std::to_string(dataTimestamp) + ","
										+ dataAziTxt + "," + dataEleTxt + ","
										+ dataFreqTxt + "," 
										+ dataPowTxTxt + "," + dataPowRxTxt;
		std::string dataToFile = std::to_string(dataTimestamp) + "," + std::to_string(*(nextIndex)) + "," 
								+ std::to_string(dataAzi) + "," + std::to_string(dataEle) + ","
								+std::to_string(dataFreq) + "," + std::to_string(dataPowTx) + "," + std::to_string(dataPowRx);
		// output data to console first, in case file operations crash
		interfaceOut(dataToConsole, false);
		if (dataOut(dataToFile) == false) { errorOut("Failed to write to file.");errorBeep(); }
		else{ infoBeep(); }

		// update index
		*(nextIndex) = *(nextIndex) + 1;
	} // reached end of sweep positions

	endTime = timestampMs();
	elapsedTime = endTime - startTime;

	interfaceOut("Sweep Run time: " + std::to_string(elapsedTime/1000/60) + " minutes " + std::to_string(elapsedTime/1000%60) + " seconds",false);
}

/*
void runSweep(testPosition* positions, int totalPositions, int nextIndex) { //optional starting index i, incase we need to resume; n loops
	errorOut("Sweep mode requires functions from specific devices. Function not implemented.");
	return;
}
*/

// save program state
const int saveFormatVersion = 1;
void saveSweepState(testPosition* positions, int totalPositions, int nextIndex) { //from dedicated.dat file in local directory
	// FORMAT
	// version (int)
	// timestamp (int)
	// nextPosition index (int)
	// totalPositions
	// list (format below)
	// 
	// azimuth (float), elevation (float), freq in Hz(int), power in dB (int)

	bool fileExists = false;
	char promptResponse = 'q';

	// if dedicated.dat exists, ask if it should be overwritten
	if (FILE* file = fopen(SAVE_STATE_FILE_DEFAULT, "r")) {
		fileExists = true;
	}

	if (fileExists) {
		promptResponse = ynPrompt("Should the existing savestate file be overwritten?");
		if (promptResponse == 'n'){
			interfaceOut("Program state will not be saved.",false);
			return;
		}
	} // end checks for file existance, and overwrite protection

	std::ofstream savefile;
	// write to file
	savefile.open(SAVE_STATE_FILE_DEFAULT, 'w');
	savefile << saveFormatVersion  << std::endl; // version (int)
	savefile << chamberTimestamp() << std::endl; // timestamp (int)
	savefile << nextIndex          << std::endl; // nextPosition index (int)
	savefile << totalPositions     << std::endl; // totalPositions
	// list (format below)
	for (int i = 0; i < totalPositions; i++) {
		savefile << positions[i].azimuth << ",";
		savefile << positions[i].elevation << ",";
		savefile << positions[i].frequency << ",";
		savefile << positions[i].power << std::endl;
	}
	savefile.close();
}
bool loadSweepState(testPosition** positions, int* nextPosition, int* totalPositions) { //from dedicated.dat file in local directory
	int version = -1;
	int lineOfFile = 0;
	int lineOfTable = 0;
	int subOffset = 0;
	int subOffset2 = 0;
	int subOffset3 = 0;
	std::ifstream savefile;
	std::string s = ""; // line by line of file
	std::string subs = ""; // sub-string to process the comma delineation
	savefile.open(SAVE_STATE_FILE_DEFAULT,std::ios::in);
	if (!savefile.is_open()) {
		return false;
	}
	// version number      //std::stoi() 
	// timestamp (int)
	// nextPosition index (int)
	// totalPositions
	// list (format below)
	while (std::getline(savefile, s)) {
		if (lineOfFile == 0) { // version of file format
			version = atoi(s.c_str());
			if (version != saveFormatVersion) {
				errorOut("Can't read savestate file! Wrong version."); exit(-1);
			}
		} else if(lineOfFile == 1){ // timestamp
			debugOut("Timestamp read successfully");
		} else if(lineOfFile == 2){ // nextPositionIndex
			(*nextPosition) = atoi(s.c_str());
		} else if(lineOfFile == 3){ // totalPositions
			(*totalPositions) = atoi(s.c_str());
			// allocate size of table
			debugOut("Loading positions array of size:");
			int slotsToAllocate = (*totalPositions);
			*positions = new testPosition[slotsToAllocate];
		} else if(lineOfFile >= 4){
			if (lineOfTable >= (*totalPositions)) {
				errorOut("Date file may be malformed...there is more position data than expected.");
				break;
			}
			// parse the line - comma delimited
			// azimuth (float), elevation (float), freq in Hz(int), power in dB (int)
			//getline(s, subs, ','); (*positions).azimuth = atoi(subs.c_str());
			//getline(s, subs, ','); (*positions).elevation = atoi(subs.c_str());
			//getline(s, subs, ','); (*positions).frequency = atoi(subs.c_str());
			//getline(s, subs, ','); (*positions).power = atoi(subs.c_str());
			subOffset = 0;
			subOffset2 = 0;
			subOffset3 = 0;
			//if(s.find) // check the number of , to verify
			subOffset = s.find(',', 0);             
				(*positions)[lineOfTable].azimuth   = atof((s.substr(0, subOffset - 0)).c_str());
			subOffset2 = s.find(',', subOffset+1);  
				(*positions)[lineOfTable].elevation = atof((s.substr(subOffset +1, subOffset2 - subOffset)).c_str());
			subOffset3 = s.find(',', subOffset2+1); 
				(*positions)[lineOfTable].frequency = atof((s.substr(subOffset2+1, subOffset3 - subOffset2)).c_str());
				(*positions)[lineOfTable].power     = atoi((s.substr(subOffset3+1, 99999)).c_str());
			lineOfTable++;
		} else { // lineOfFile is somehow negative
			errorOut("Line of File variable is somehow negative. Aborting file read...");
			break;
		}

		lineOfFile++;
	} // end while getline
	savefile.close();
	if (lineOfTable != (*totalPositions)) {
		errorOut("Data file may be malformed. Did not read enough data lines.");
		exit(-1);
	}
	return true; // if we got this far, we read the data successfully!
}

// activate in main by using SetConsoleCtrlHandler(chamberCtrlHandler, TRUE);
// see https://learn.microsoft.com/en-us/windows/console/registering-a-control-handler-function
BOOL WINAPI chamberCtrlHandler(DWORD fdwCtrlType) {// returning true will cause program to exit
	switch (fdwCtrlType) {
	case CTRL_C_EVENT:
		errorOut("\nCtrl-C event\n");
		saveAndCloseFlag = true;
		errorBeep();
		return TRUE; // ctrl+c will not end the program when return true; this function handles it

		// CTRL-CLOSE: confirm that the user wants to exit.
	case CTRL_CLOSE_EVENT:
		errorOut("Ctrl-Close event - please use ctrl+c to stop program first\n\n");
		errorBeep();
		return TRUE;

		// Pass other signals to the next handler.
	case CTRL_BREAK_EVENT:
		// ctrl+fn+p, for pause - will kill program
		errorOut("Ctrl-Break event\n\n");
		return FALSE;

	case CTRL_LOGOFF_EVENT:
		// don't close program if asked to logoff! I dont like windows update!
		errorOut("Ctrl-Logoff event\n\n");
		errorBeep();
		return TRUE;

	case CTRL_SHUTDOWN_EVENT:
		// don't close program if asked to logoff! I dont like windows update!
		errorOut("Ctrl-Shutdown event\n\n");
		errorBeep();
		return TRUE;

	default:
		return FALSE;
	}
}


/*
// save state when problems occur, and beep
onPositionError() - describe which position failed, then throw larger error; very long timeOut, perhaps? - handle in main?

// require context to pick parameters from args
setupFieldFox()
setupSignalGenerator()
setupTurntable() // will likely start movement - must check externally if finished - also gets the limits of both axis for reference
*/
