#pragma once
// for consolidating program args

#include <string>

// can't include C headers in C++ otherwise
extern "C" {
#include "getopt.h"
}

#define A_FLAG_INDEX (0)
#define B_FLAG_INDEX (1)
#define C_FLAG_INDEX (2)
#define D_FLAG_INDEX (3)
#define E_FLAG_INDEX (4)
#define F_FLAG_INDEX (5)
#define G_FLAG_INDEX (6)
#define H_FLAG_INDEX (7)
#define I_FLAG_INDEX (8)
#define J_FLAG_INDEX (9)
#define K_FLAG_INDEX (10)
#define L_FLAG_INDEX (11)
#define M_FLAG_INDEX (12)
#define N_FLAG_INDEX (13)
#define O_FLAG_INDEX (14)
#define P_FLAG_INDEX (15)
#define Q_FLAG_INDEX (16)
#define R_FLAG_INDEX (17)
#define S_FLAG_INDEX (18)
#define T_FLAG_INDEX (19)
#define U_FLAG_INDEX (20)
#define V_FLAG_INDEX (21)
#define W_FLAG_INDEX (22)
#define X_FLAG_INDEX (23)
#define Y_FLAG_INDEX (24)
#define Z_FLAG_INDEX (25)

extern int  opterr,                 /* if error message should be printed */
			optind,                 /* index into parent argv vector */
			optopt,                 /* character checked for validity */
			optreset;               /* reset getopt */
extern char* optarg;                /* argument associated with option */

extern bool  progFlags[26] = { false, false, false, false, false, false, false, false, false, false, false, false, false,
								false, false, false, false, false, false, false, false, false, false, false, false, false };
extern std::string progFlagArgs[26] = { "", "", "", "", "", "", "", "", "", "", "", "", "",
										"", "", "", "", "", "", "", "", "", "", "", "", ""};

int processArguments(int argc, char* argv[], const char* argumentList) { // returns index of first non-flag argument
	while (true) {
		char currentArg = (char)getopt(argc, argv, argumentList);
		switch (currentArg) {
		default:
		#ifdef DEBUG
				std::cerr << "Ending getop() operation...Unrecognized Flag - " << currentArg << std::endl;
		#endif // DEBUG
			return optind; // continue processing as normal, non-flag arguments (usually for -s flags sweep mode)
			//exit(-1);
		case -1:
			#ifdef DEBUG
				std::cerr << "Got argument -1. No more arguments." << std::endl;
			#endif // DEBUG
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
			progFlags[((int)currentArg - (int)'a')] = true;
			if (optarg != NULL) {
				progFlagArgs[((int)currentArg - (int)'a')] = optarg;
			}
			continue;
		}
		break; // we should only get here when switch statement gets -1
	}
	return optind;
} // end processArguments()

void printProgramArguments() {
	std::cout << "------------------------------" << std::endl;
	std::cout << "Program arguments:" << std::endl;
	for (int i = 0; i < ((int)'z' - (int)'a'); i++) {
		std::cout << "arg " << (char)((char)i + (int)'a') << " - " << progFlags[i] << ", " << progFlagArgs[i] << std::endl;
	}
	std::cout << "------------------------------" << std::endl;
}

bool getProgFlag(int progFlagIndex, std::string *flagArgValue) {
	if (progFlagIndex < 0 || 26 <= progFlagIndex) {
		std::cerr << "Bad arg flag index: " << std::to_string(progFlagIndex) << std::endl;
		return false;
	} else {
		*(flagArgValue) = progFlagArgs[progFlagIndex];
		return progFlags[progFlagIndex];
	}
}

bool getProgFlag(int progFlagIndex) {
	std::string tempStr = "";
	return getProgFlag(progFlagIndex, &tempStr);
}