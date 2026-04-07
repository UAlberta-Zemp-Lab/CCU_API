#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "../../ccu.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif

// Cross-platform sleep function in milliseconds
void sleep_ms(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000); 
#endif
}

#define WAIT_TIME 0.5
#define BIAS_SIZE 256

static struct sp_port *port;

int TestPattern(int (*patterns)[256], int numElements) {
	for (int i = 0; i < numElements; i++) {
		for (int j = 1; j < numElements; j++) {
			patterns[i][0] = 1;
			patterns[i][j] = -1;
		}
	}
	return numElements;

}

int ChristmasTree(int (*patterns)[256], int numElements, int filler, int target) {
	for (int i = 0; i < numElements; i++) {
		for (int j = 0; j < numElements; j++) {
			if (j == i)
				patterns[i][j] = target;
			else
				patterns[i][j] = filler;
		}
	}
	return numElements;
}

int SpecialPattern(int (*patterns)[256], int numElements) {
	for (int i = 0; i < 128; i+=4) {
		for (int j = 0; j < numElements; j++) {
			patterns[i][j] = 0;
		}
		for (int j = 0; j < numElements; j++) {
			patterns[i+1][j] = 1;
		}
		for (int j = 0; j < numElements; j++) {
			patterns[i+2][j] = -1;
		}
		for (int j = 0; j < numElements; j++) {
			patterns[i+3][j] = 2;
		}
	}
	return 128;
}

void closeCCU() {
	if (!port)
		return;
	printf("Closing CCU port.\nGood bye.\n");
	setVPP(port, 0);
	setVNN(port, 0);
	closePort(port);
	port = NULL;
}

void checkResult(void *res, char *message) {
	if (!res) {
		printf(message);
		closeCCU();
		exit(-1);
	}
}

void checkResultInt(int res, char *message) {
	if (!res) {
		printf(message);
		closeCCU();
		exit(-1);
	}
}

void programPatterns(struct sp_port *port, int (*patterns)[256], int numPatterns) {
	for (int i = 0; i < numPatterns; i++) {	
		printf("\rProgramming pattern %d/%d", i+1, numPatterns);
		fflush(stdout);
		checkResult(setSequence256(port, patterns[i], numPatterns, i+1), "\nCould not set pattern.\n");
	}
	printf("\n");
}

// Cross-platform handler function
#if defined(_WIN32) || defined(_WIN64)
BOOL WINAPI signal_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT) {
        printf("\nInterrupt signal caught. Exiting.\n");
        closeCCU();
		exit(2);
        return TRUE;
    }
    return FALSE;
}
#else
void signal_handler(int signum) {
	printf("\nInterrupt signal caught. Exiting.\n");
	closeCCU();
	exit(2);
}
#endif

void register_signal_handler() {
	#if defined(_WIN32) || defined(_WIN64)
		if (!SetConsoleCtrlHandler(signal_handler, TRUE)) {
			fprintf(stderr, "Error setting signal handler\n");
		}
	#else
		if (signal(SIGINT, signal_handler) == SIG_ERR) {
			fprintf(stderr, "Error setting signal handler\n");
		}
	#endif
}

/**
 * Safely reads an integer from standard input, handling invalid input.
 * Prompts the user and continues to ask until a valid integer is entered.
 */
int get_safe_int(const char* prompt, int default_num, long llimit, long ulimit) {
    char line[256]; // Buffer to store input line
    long value;
    char* endptr;
    int valid_input = 0;

    while (!valid_input) {
        printf("%s", prompt); // Display the prompt

        // Read the entire line of input
        if (fgets(line, sizeof(line), stdin) == NULL) {
            // Handle EOF or read error
            printf("Error reading input. Exiting.\n");
            exit(EXIT_FAILURE);
        }

        // Remove the trailing newline character, if present
        line[strcspn(line, "\n")] = 0;

        // Convert the string to a long integer
        value = strtol(line, &endptr, 10);

        // Check for conversion errors and invalid characters
        if (endptr == line) {
			valid_input = 1;
			value = default_num;
        } else if (*endptr != '\0' && !isspace((unsigned char)*endptr)) {
            // Check if there are non-numeric characters after the number
            printf("Invalid input. Contains non-numeric characters after the number. Please enter a number.\n");
        } else if (value > ulimit || value < llimit) {
            printf("Please enter a number between %ld - %ld.\n", llimit, ulimit);
        } else {
            valid_input = 1; // Input is valid
        }
    }

    return (int)value;
}

int askForPatternChoice(int (*patterns)[BIAS_SIZE], int numElements) {
	char patterns_list[] = \
	"Patterns:\n" \
	"1. Christmas Tree (All VNN, Target VPP).\n" \
	"2. Christmas Tree (All VPP, Target VNN).\n" \
	"3. Christmas Tree (All Ground, Target VPP)\n" \
	"4. Christmas Tree (All Ground, Target VNN).\n" \
	"5. Christmas Tree (All Open Circuit, Target VPP).\n" \
	"6. Christmas Tree (All Open Circuit, Target VNN).\n" \
	"7. Four Pattern Cycle (VPP, VNN, Ground, Open Circuit).\n";

	printf(patterns_list);
	char prompt[] = "Enter a pattern number (1-7): ";

	int choice = get_safe_int(prompt, 1, 1, 7);
	
	printf("Using pattern %d.\n\n", choice);

	int numPatterns = 0;

	switch (choice) {
		case 1:
			//numPatterns = TestPattern(patterns, numElements);
			numPatterns = ChristmasTree(patterns, numElements, -1, 1);
			break;
		case 2:
			numPatterns = ChristmasTree(patterns, numElements, 1, -1);
			break;
		case 3:
			numPatterns = ChristmasTree(patterns, numElements, 0, 1);
			break;
		case 4:
			numPatterns = ChristmasTree(patterns, numElements, 0, -1);
			break;
		case 5:
			numPatterns = ChristmasTree(patterns, numElements, 2, 1);
			break;
		case 6:
			numPatterns = ChristmasTree(patterns, numElements, 2, -1);
			break;
		case 7:
			numPatterns = SpecialPattern(patterns, numElements);
			break;
		default:
	}
	
	return numPatterns;
}

int askForWaitTime() {
	char prompt[] = "Enter wait time in ms (default = 50): ";
	int choice = get_safe_int(prompt, 50, 1, 10000);
	printf("Using a wait time of %d ms.\n\n", choice);
	return choice;
}

int main() {
	const int numElements = 256;
	const int vpp = 25;
	const int vnn = 25;
	const int vppc = 350;
	const int vnnc = 350;

	int waitTime = 50;

	register_signal_handler();

	printf(
			"-----------------------------------------------------------\n"
			"Welcome to the CliniSonix Bias Board Tester. (C) 2025-2026.\n"
			"-----------------------------------------------------------\n\n");

	port = getAndInitializePort();
	checkResult(port, "Could not open CCU port. Please check if it's in use by another program.\n");

	CCU_Version_Response *res = getVersion(port);
	checkResult(res, "Could not get CCU version. Please check CCU is at least version 3.\n");
	printf("Found CCU. Major verison: %d; Stamp: %lu\n", 
			res->version_major, res->stamp);

	checkResult(setVPPCurrentLimit(port, vppc), "Could not set VPP current limit.\n");
	checkResult(setVNNCurrentLimit(port, vnnc), "Could not set VNN current limit.\n");
	checkResult(setStop(port, 0), "Could not initialize CCU.\n");
	checkResult(setVPP(port, vpp), "Could not set VPP.\n");
	checkResult(setVNN(port, vnn), "Could not set VNN.\n");

	printf("\n");
	int patterns[BIAS_SIZE][BIAS_SIZE];
	int numPatterns = askForPatternChoice(patterns, numElements);
	waitTime = askForWaitTime();

	programPatterns(port, patterns, numPatterns);

	checkResult(setNextSequence(port, 0, numElements), "Could not set start sequence number.\n");

	sleep_ms(1000);

	for (int i = 0; i < numPatterns; i++) {
		printf("\rSetting pattern %d/%d (wait time = %d ms)", i+1, numPatterns, waitTime);
		fflush(stdout);
		checkResultInt(emulateTrigger(port), "\nCould not set next pattern (emulate trigger).\n");
		sleep_ms(waitTime);
	}
	printf("\n\n");

	closeCCU();

    printf("Press any key to exit...\n");
    getchar();

	return 0;
}
