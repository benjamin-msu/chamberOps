// Anechoic Chamber data collection program,
// designed for use inside Mississippi State University's 
// Electrical Engineering chamber setup
// by Benjamin Wilkinson, all rights reserved
// 10/31/2023

// various settings needed to be changed to make this exe useable on computers that are not the computer that compiled it.
// either install the Microsoft VC++ Redistributable Package on target machine (both x64 and x86 may be required)
// at https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170
// OR, change the compiling options to statically link the libraries instead of dynamically,
// as suggested by https://stackoverflow.com/questions/15297270/problems-with-running-exe-file-built-with-visual-studio-on-another-computer#15297493
// Project Properties -> C/C++ -> Code Generation -> Runtime Library. Change from /MD to /MT (and use the version /MTd if in debug configuration)
// this will allow the file to run on other computers

//#define DEBUG
//#define SHOULD_PREPRINT_POSITIONS
#define PROGRAM_VERSION (7)
#define ACCEPTABLE_ARGUMENTS ("hmsf:p:ro:viyz")

#define EXPERIMENT_DEFAULT_POSITIONS (nullptr)
#define EXPERIMENT_DEFAULT_NEXT_POSITION   (-1)
#define EXPERIMENT_DEFAULT_TOTAL_POSITIONS (-1)

#define DEFAULT_AZIMUTH_DENSITY   (1.0)
#define DEFAULT_ELEVATION_DENSITY (1.0)

#define NAN_DOUBLE (std::numeric_limits<double>::quiet_NaN())

enum programMode_t{
	SETUP_MODE, SWEEP_MODE, INTERACTIVE_MODE, CLEANUP_MODE
} programMode;

#include <string>
#include <iostream>
#include "inputArgs.h"
#include <limits> // for nan
//#include <unistd.h>
//#pragma comment(lib, "Ws2_32.lib")

#include "chamber.h"

void printHelp() {
	std::cout << "chamberOps.exe version " << std::to_string(PROGRAM_VERSION) <<std::endl
			  << "  -f: targetFreq  (only hz for now, future take suffix of GHz, KHz, etc)" << std::endl
			  << "  -i: interactive mode (the default) - NOT YET IMPLEMENTED" << std::endl
			  << "  -m: use current fieldfox and signal generator settings (manual override)" << std::endl
			  << "  -o: sets a name for output file, for data" << std::endl
			  << "  -p: targetPower (in dbm)" << std::endl
			  << "  -r: resume from savestate" << std::endl
			  << "  -s: sweep mode, taking 4 or 6 arguments for  " << std::endl
			  << "      azimuthMin, azimuthMax, elevationMin, elevationMax, (optional)aziDensity, (optional)elevDensity" << std::endl
			  << "      ALSO REQUIRES -f and -p" << std::endl;
}

// -v: forces preview of all positions, and setup/progress messages
////// FUTURE //////
// -i: interactive

// modes: interactive, sweep, resume

int main(int argc, char* argv[]) {
	// status variables
	int  nextIndexOfRemainingArguments = -1; // the index for extra parameters - an argv[] index
	bool didReadSaveState = false;
	bool saveStateFileIsValid = false;

	// experiment sweep variables
	testPosition* experimentPositions = EXPERIMENT_DEFAULT_POSITIONS;
	int experimentNextPosition        = EXPERIMENT_DEFAULT_NEXT_POSITION;
	int experimentTotalPositions      = EXPERIMENT_DEFAULT_TOTAL_POSITIONS;

	// reasonable sweep defaults
	long long targetSweepFrequency = -1;
	double targetSweepPower     = 0; // dbm

	// sweep variables
	double elevationRangeMin = NAN_DOUBLE;
	double elevationRangeMax = NAN_DOUBLE;
	double azimuthRangeMin = NAN_DOUBLE;
	double azimuthRangeMax = NAN_DOUBLE;
	double aziDensity  = DEFAULT_AZIMUTH_DENSITY;
	double elevDensity = DEFAULT_ELEVATION_DENSITY;

	std::string flagValProcessingBuffer = "";

	programMode = SETUP_MODE; // should be the default mode

	// activate signal handler
	if(SetConsoleCtrlHandler(chamberCtrlHandler, TRUE)) { // setup signal handlers (see chamber.h)
		debugOut("Registered signal handler successfully.");
	}

	// process arguments
	nextIndexOfRemainingArguments = processArguments(argc, argv, ACCEPTABLE_ARGUMENTS);
	// change to include -v arg printing things (may be in function?)
	if (debugFlagIsSet()) {
		printProgramArguments();
	}

	// verify savestate file
	didReadSaveState = loadSweepState(&experimentPositions, &experimentNextPosition, &experimentTotalPositions);
	if (didReadSaveState 
		&& (experimentPositions != EXPERIMENT_DEFAULT_POSITIONS) 
		&& (experimentNextPosition != EXPERIMENT_DEFAULT_NEXT_POSITION) 
		&& (experimentTotalPositions != EXPERIMENT_DEFAULT_TOTAL_POSITIONS)) {
		// save state was read, AND that all values were successfully read
		interfaceOut("Save state file was detected.", false);
		saveStateFileIsValid = true;
	}
	if (getProgFlag(R_FLAG_INDEX) && !saveStateFileIsValid) {
		errorOut("No save state file detected!");
		exit(-1);
	}

	////// check flags and set mode arguments
	// check freq flag - make sure is non the default NAN below, before generating sweep
	if (getProgFlag(F_FLAG_INDEX, &flagValProcessingBuffer)) {
		targetSweepFrequency = atof(flagValProcessingBuffer.c_str());
		if (targetSweepFrequency < 0 || 1000000000000 < targetSweepFrequency) {
			errorOut("Frequency must be positive, and below the THz range.");
			exit(-1);
		}
	}
	// check power flag (defaults to 0)
	if (getProgFlag(P_FLAG_INDEX,&flagValProcessingBuffer)) {
		targetSweepPower = atof(flagValProcessingBuffer.c_str());
		if (targetSweepPower < SIG_GEN_MIN_POWER || SIG_GEN_MAX_POWER < targetSweepPower || isnan(targetSweepPower)) {
			errorOut("Power must be in valid range for the signal generator. Default is 0 dBm.");
			exit(-1);
		}
	}
	// verify filenamev - do further checks in the future
	if (getProgFlag(O_FLAG_INDEX,&flagValProcessingBuffer)) {
		if(flagValProcessingBuffer.empty()){
			errorOut("Filename must be specified with the O flag.");
			exit(-1);
		}
		if (getProgFlag(V_FLAG_INDEX)) { interfaceOut("File output set to "+flagValProcessingBuffer, false); }
	}
	// check for sweep mode flags and parameters
	if (getProgFlag(S_FLAG_INDEX) && nextIndexOfRemainingArguments != -1) {
		int extraArgs = 0;
		if (saveStateFileIsValid && 'n' == ynPrompt("Proceed with command line values, rather than savestate? Current savestate may be overwritten.")) {
			exit(-1);
		}
		// arg list in help: azimuthMin, azimuthMax, elevationMin, elevationMax, (optional)aziDensity, (optional)elevDensity"
		for (int i = optind; i < argc; i++) {
			if (extraArgs == 0) {
				azimuthRangeMin = atof(argv[i]);
			} else if (extraArgs == 1) {
				azimuthRangeMax = atof(argv[i]);
			} else if (extraArgs == 2) {
				elevationRangeMin = atof(argv[i]);
			} else if (extraArgs == 3) {
				elevationRangeMax = atof(argv[i]);
			} else if (extraArgs == 4) {
				aziDensity = atof(argv[i]);
			} else if (extraArgs == 5) {
				elevDensity = atof(argv[i]);
			} else {
				errorOut("Sweep flag does not support more than 6 arguments");
				exit(-1);
			}
			extraArgs++;
		}
		// sanity check the read arguments - must not be isnan(), must be in valid ranges
		if (isnan(elevationRangeMin) || isnan(elevationRangeMax) || isnan(azimuthRangeMin) || isnan(azimuthRangeMax)) {
			errorOut("Sweep mode requires at least 4 arguments: elevationMin, elevationMax, azimuthMin, azimuthMax. Density arguments are optional.");
			exit(-1);
		} else {
			if (targetSweepFrequency < 0 || 1000000000000 < targetSweepFrequency) {
				errorOut("Frequency must be positive, and below the THz range.");
				exit(-1);
			} else if(targetSweepPower < SIG_GEN_MIN_POWER || SIG_GEN_MAX_POWER < targetSweepPower || isnan(targetSweepPower)){
				errorOut("Sweep mode requires a reasonable target power (defaults to 0dBm).");
				exit(-1);
			}
			// generate positions
			experimentTotalPositions = createPositionTargets(&experimentPositions, targetSweepFrequency, targetSweepPower,
								elevationRangeMin, elevationRangeMax, azimuthRangeMin, azimuthRangeMax,
								aziDensity,elevDensity);
			experimentNextPosition = 0;
		}
	}

	if (getProgFlag(H_FLAG_INDEX)) {
		printHelp();
		exit(0);
	}

	// used for debug
	#ifdef SHOULD_PREPRINT_POSITIONS
		printPositionsTable(experimentPositions, experimentTotalPositions);
	#endif

	if (initiateDevices() == false) {
		interfaceOut("Failed to connect to some devices.",false);
		if ('y' == ynPrompt("Would you like to savestate your current settings, for inspection?")) {
			saveSweepState(experimentPositions, experimentTotalPositions, experimentNextPosition);
			interfaceOut("Savestate created.",false);
		} else {
			interfaceOut("Savestate will not be created.",false);
		}
		exit(-1);
	}

	// what mode should be run?
	if ((getProgFlag(R_FLAG_INDEX) || getProgFlag(S_FLAG_INDEX)) && !getProgFlag(I_FLAG_INDEX)) {
		programMode = SWEEP_MODE;
		if (!getProgFlag(M_FLAG_INDEX)) { // don't change settings if "manual" flag is specified
			// set signal generator
			setSignalGenFreq(targetSweepFrequency,0);
			setSignalGenPower(targetSweepPower);
			setSignalGenModOff();
			setSignalGenOff();

			int frequencyRangeScale = DEFAULT_SPECTRUM_ANALYZER_RANGE_SCALE; // +/- a frequency offset, to create a window

			long long targetFreqStart = targetSweepFrequency - frequencyRangeScale;
			long long targetFreqStop  = targetSweepFrequency + frequencyRangeScale;
			if (targetFreqStart < 0) { // fix potential negative frequencies
				targetFreqStop  += std::abs(targetFreqStart);
				targetFreqStart += std::abs(targetFreqStart);
			}

			int targetSpectrumAnalyzerBWres  = DEFAULT_SPECTRUM_ANALYZER_BW_RES;
			int targetSpectrumAnalyzerVidBW  = DEFAULT_SPECTRUM_ANALYZER_VIDEO_BW_RES;
			int targetSpectrumAnalyzerPoints = DEFAULT_SPECTRUM_ANALYZER_POINTS;
			
			presetSpectrumAnalyzer();
			setSpectrumAnalyzerMode("SA");
			setSpectrumAnalyzerRangeStart(targetFreqStart,0);
			setSpectrumAnalyzerRangeStop(targetFreqStop, 0);
			setSpectrumAnalyzerBandWResolution(targetSpectrumAnalyzerBWres,0);
			setSpectrumAnalyzerBandWVideo(targetSpectrumAnalyzerVidBW, 0);
			setSpectrumAnalyzerSweepPoints(targetSpectrumAnalyzerPoints);
			setSpectrumAnalyzerMarkerNormal(1, targetSweepFrequency, 0);
			setSpectrumAnalyzerTraceModeMaxHold();
		} else {
			interfaceOut("Manual settings on Spectrum Analyzer and Signal Generator will be used.", false);
		}

		// run sweeps; should be set to one or the other. Sweep values will override resume values if need be
		sweepModeStart(experimentPositions, &experimentTotalPositions, &experimentNextPosition);
	} else { // start interactive mode as a default (I flag should bring us here too)
		programMode = INTERACTIVE_MODE;
		// setup function
		interactiveModeStart();
		// check for ctrl+c inside, and exit when finished
	}

	// save state if we were in sweep mode, and the sweep was not finished
	if (programMode == SWEEP_MODE && (experimentNextPosition < experimentTotalPositions)) {
		programMode = CLEANUP_MODE;
		saveSweepState(experimentPositions, experimentTotalPositions, experimentNextPosition);
		interfaceOut("Save state file created. Progress was ["
			+ std::to_string(experimentNextPosition) + "/" + std::to_string(experimentTotalPositions) + "]", false);
	} else if(programMode == SETUP_MODE) { // we never had a sweep, or interactive mode! likely, no arguments at all
		programMode = CLEANUP_MODE;
		printHelp();
		exit(0);
	} else {
		programMode = CLEANUP_MODE;
	}

	// cleanup
	cleanupPositionTargets(experimentPositions);
	cleanupVisa();
	cleanupTelnet();

	/*
	// setup device connections
	setupVisa();
	getTurntableSoftLimits();
	setupTelnet();
	printDeviceStatus();
	if (deviceConnectionStatus[DEVICE_STATUS_INDEX_FIELDFOX] != true) {
		errorOut("It seems that the fieldfox is unplugged. Has this computer configured the following network connection, for direct LAN access?\
	\nIP: 192.168.0.2 ---------- Subnet mask: 255.255.248.0");
	}

	// decide how the program will run
	
	// check for existing savestate file
	didReadSaveState = loadSweepState(&experimentPositions, &experimentNextPosition, &experimentTotalPositions);

	if (!didReadSaveState) {
		// check arguments and create positions - for now, focus on sweep params
		// 

	} else {
		interfaceOut("Loaded Save state successfully.", false);

		debugOut("Check to make sure there were not extra arguments...");
	}

	// print positions, and ask user to verify
	printPositionsTable(experimentPositions, experimentTotalPositions);
	switch (ynPrompt("Proceed with these settings?"))
	{
	case 'y':
	case 'Y':
		break;
	case 'n':
	case 'N':
		interfaceOut("Please correct the program settings, and try again.",false);
		break;
	default:
		errorOut("No valid response...Aborting...");
		errorBeep();
		exit(-1);
		break;
	}


	*/
}