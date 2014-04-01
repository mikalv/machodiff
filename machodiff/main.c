//
//  main.c
//  machodiff
//
//  Created by Sam Marshall on 2/22/14.
//  Copyright (c) 2014 Sam Marshall. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "diff.h"
#include "generate_objc.h"

static char *help_arg_def = "-h,--help";
static char *input_arg_def = "-i,--input";
static char *output_arg_def = "-o,--output";
static char *arch_arg_def = "-a,--arch";
static char *dump_arg_def = "-d,--dump";

enum MachODiffOptions {
	OptionsHelp = 0,
	OptionsInput,
	OptionsOutput,
	OptionsArch,
	OptionsDump,
	OptionsCount
};

enum AnalysisFlags {
	Invaid = 0,
	InputOK,
	OutputOK,
	ArchOK,
	DumpOK
};

static struct option long_options[OptionsCount] = {
	{"help", no_argument, 0, 'h'},
	{"input", required_argument, 0, 'i'},
	{"outout", required_argument, 0, 'o'},
	{"arch", required_argument, 0, 'a'},
	{"dump", no_argument, 0, 'd'}
};

static uint8_t options_enabled[OptionsCount] = {0};

#define kSatisfiedHelp (options_enabled[OptionsHelp] != 0)
#define kSatisfiedInput (options_enabled[OptionsInput] == 2)
#define kSatisfiedOutput (options_enabled[OptionsOutput] == 1)
#define kSatisfiedArch (options_enabled[OptionsArch] != 0)
#define kSatisfiedDump (options_enabled[OptionsDump] != 0)

void usage(void);

void usage() {
	printf("%s : display help\n",help_arg_def);
	printf("%s : dump headers",dump_arg_def);
	printf("%s [Mach-O binary] : specify an Mach-O binary as input\n",input_arg_def);
	printf("%s [directory] : specify an output directory\n",output_arg_def);
	printf("%s [i386|x86_64|armv6|armv7|armv7s|arm64|ppc|ppc64] : specify an architecture to target\n",arch_arg_def);
}

int main(int argc, const char * argv[]) {
	
	char *input_args[2] = {0};
	char *output_arg = NULL;
	
	bool search_args = true;
	uint8_t run_analysis = 0;
	
	int c;
	while (search_args) {
		int option_index = 0;
		c = getopt_long (argc, (char * const *)argv, "hda:i:o:",long_options, &option_index);
		if (c == -1) {
			break;
		}
		switch (c) {
			case 'a': {
				if (optarg) {
					if (strcmp(optarg, "i386") == 0) {
						options_enabled[OptionsArch] = loader_arch_i386_type;
					}
					else if (strcmp(optarg, "x86_64") == 0) {
						options_enabled[OptionsArch] = loader_arch_x86_64_type;
					}
					else if (strcmp(optarg, "armv6") == 0) {
						options_enabled[OptionsArch] = loader_arch_armv6_type;
					}
					else if (strcmp(optarg, "armv7") == 0) {
						options_enabled[OptionsArch] = loader_arch_armv7_type;
					}
					else if (strcmp(optarg, "armv7s") == 0) {
						options_enabled[OptionsArch] = loader_arch_armv7s_type;
					}
					else if (strcmp(optarg, "arm64") == 0) {
						options_enabled[OptionsArch] = loader_arch_arm64_type;
					}
					else if (strcmp(optarg, "ppc") == 0) {
						options_enabled[OptionsArch] = loader_arch_ppc_type;
					}
					else if (strcmp(optarg, "ppc64") == 0) {
						options_enabled[OptionsArch] = loader_arch_ppc64_type;
					}
				}
				break;
			};
			case 'h': {
				options_enabled[OptionsHelp]++;
				break;
			};
			case 'i': {
				if (optarg) {
					if (options_enabled[OptionsInput] < 2) {
						input_args[options_enabled[OptionsInput]] = optarg;
						options_enabled[OptionsInput]++;
					}
				}
				break;
			};
			case 'o': {
				if (optarg) {
					if (options_enabled[OptionsOutput] < 1) {
						output_arg = optarg;
						options_enabled[OptionsOutput]++;
					}
				}
				break;
			}
			case 'd': {
				options_enabled[OptionsDump]++;
				break;
			}
			default: {
				break;
			};
		}
		search_args = (((kSatisfiedInput && kSatisfiedOutput) || (kSatisfiedHelp)) ? false : true);
	}
	
	if (kSatisfiedHelp) {
		usage();
	}
	else if (input_args[0] != NULL && input_args[1] != NULL){
		uint8_t type_1 = SDMIsBinaryFat(input_args[0]);
		uint8_t type_2 = SDMIsBinaryFat(input_args[1]);
		if ((type_1 != loader_binary_arch_invalid_type && type_2 != loader_binary_arch_invalid_type) && (type_1 == loader_binary_arch_fat_type || type_2 == loader_binary_arch_fat_type)) {
			if (kSatisfiedArch) {
				run_analysis |= ArchOK;
			}
			else {
				printf("Please specify architecture for fat binaries.\n");
			}
		}
		
		if (kSatisfiedInput) {
			run_analysis |= InputOK;
		}
		else {
			printf("Insufficient input specified.\n");
		}
		
		if (kSatisfiedOutput) {
			run_analysis |= OutputOK;
		}
		else {
			printf("No output directory specified.\n");
		}
	}
	
	if (run_analysis == (InputOK | OutputOK | ArchOK)) {
		struct loader_binary *input_one = SDMLoadTarget(input_args[0], options_enabled[OptionsArch]);
		struct loader_binary *input_two = SDMLoadTarget(input_args[1], options_enabled[OptionsArch]);
		
		bool status = SDMMakeNewFolderAtPath(output_arg, 0700);
		if (status && input_one && input_two) {
			SDMPerformComparison(input_one, input_two, output_arg);
		}
		
		if (kSatisfiedDump) {
			GenerateObjcHeaders(input_one->objc, output_arg);
			//GenerateObjcHeaders(input_two->objc, "binary2");
		}
		
		SDMReleaseBinary(input_one);
		SDMReleaseBinary(input_two);
	}
    return 0;
}