#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* 
   Based on GNU/Linux and glibc or not, argp.h may or may not be available.
   If it is not, fall back to getopt. Also see #ifdefs in the ParseInput.h file. 
   */
#ifdef HAVE_ARGP_H
#include <argp.h>
#define OPTARG arg
#elif defined HAVE_UNISTD_H
#include <unistd.h>
#define OPTARG optarg
#else
#include "GetOpt.h"
#define OPTARG optarg
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h> /* For u_int etc. */
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> /* For Mac OS X with resource.h */
#endif

#ifdef HAVE_RESOURCE_H
#include <sys/resource.h>
#endif

#include "../blib/BLibDefinitions.h"
#include "../blib/BError.h"
#include "Definitions.h"
#include "ReadInputFiles.h"
#include "RunAligner.h"
#include "ParseInput.h"

const char *argp_program_version =
"balign version 0.1.1\n"
"Copyright 2007.";

const char *argp_program_bug_address =
"Nils Homer <nhomer@cs.ucla.edu>";

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC, OPTIONAL_GROUP_NAME}.
   */
enum { 
	DescInputFilesTitle, DescRGFileName, DescMatchesFileName, DescScoringMatrixFileName, 
	DescAlgoTitle, DescAlgorithm, DescStartChr, DescStartPos, DescEndChr, DescEndPos, DescOffset, DescPairedEnd,
	DescOutputTitle, DescOutputID, DescOutputDir,
	DescMiscTitle, DescHelp
};

/* 
   The prototype for argp_option comes fron argp.h. If argp.h
   absent, then ParseInput.h declares it 
   */
static struct argp_option options[] = {
	{0, 0, 0, 0, "=========== Input Files =============================================================", 1},
	{"rgListFileName", 'r', "rgListFileName", 0, "Specifies the file name of the file containing all of the chromosomes", 1},
	{"matchesFileName", 'm', "matchesFileName", 0, "Specifies the file name holding the list of bmf files", 1},
	{"scoringMatrixFileName", 'x', "scoringMatrixFileName", 0, "Specifies the file name storing the scoring matrix", 1},
	{0, 0, 0, 0, "=========== Algorithm Options: (Unless specified, default value = 0) ================", 2},
	{"algorithm", 'a', "algorithm", 0, "Specifies the algorithm to use 0: Dynamic Programming", 2},
	{"startChr", 's', "startChr", 0, "Specifies the start chromosome", 2},
	{"startPos", 'S', "startPos", 0, "Specifies the end position", 2},
	{"endChr", 'e', "endChr", 0, "Specifies the end chromosome", 2},
	{"endPos", 'E', "endPos", 0, "Specifies the end postion", 2},
	{"offsetLength", 'O', "offset", 0, "Specifies the number of bases before and after the match to include in the reference genome", 2},
	{"pairedEnd", '2', 0, OPTION_NO_USAGE, "Specifies that paired end data is to be expected", 2},
	{0, 0, 0, 0, "=========== Output Options ==========================================================", 3},
	{"outputID", 'o', "outputID", 0, "Specifies the name to identify the output files", 3},
	{"outputDir", 'd', "outputDir", 0, "Specifies the output directory for the output files", 3},
	{0, 0, 0, 0, "=========== Miscellaneous Options ===================================================", 4},
	{"Parameters", 'p', 0, OPTION_NO_USAGE, "Print program parameters", 4},
	{"Help", 'h', 0, OPTION_NO_USAGE, "Display usage summary", 4},
	{0, 0, 0, 0, 0, 0}
};
/*
   ARGS_DOC. Field 3 in ARGP.
   A description of the non-option command-line arguments that we accept.
   Not complete yet. So empty string
   */
static char args_doc[] = "";
/*
   DOC.  Field 4 in ARGP.  Program documentation.
   */
static char doc[] ="This program was created by Nils Homer and is not intended for distribution.";

#ifdef HAVE_ARGP_H
/*
   The ARGP structure itself.
   */
static struct argp argp = {options, parse_opt, args_doc, doc};
#else
/* argp.h support not available! Fall back to getopt */
static char OptionString[]=
"a:d:e:m:o:r:s:x:E:H:O:S:2ph";
#endif

enum {ExecuteGetOptHelp, ExecuteProgram, ExecutePrintProgramParameters};

/*
   The main function. All command-line options parsed using argp_parse
   or getopt whichever available
   */

	int
main (int argc, char **argv)
{
	struct arguments arguments;
	RGBinary rgList;
	time_t startTime = time(NULL);
	time_t endTime;
	if(argc>1) {
		/* Set argument defaults. (overriden if user specifies them)  */ 
		AssignDefaultValues(&arguments);

		/* Parse command line args */
#ifdef HAVE_ARGP_H
		if(argp_parse(&argp, argc, argv, 0, 0, &arguments)==0)
#else
			if(getopt_parse(argc, argv, OptionString, &arguments)==0)
#endif 
			{
				switch(arguments.programMode) {
					case ExecuteGetOptHelp:
						PrintProgramParameters(stderr, &arguments);
						break;
					case ExecutePrintProgramParameters:
						PrintProgramParameters(stderr, &arguments);
						break;
					case ExecuteProgram:
						if(ValidateInputs(&arguments)) {
							fprintf(stderr, "**** Input arguments look good! *****\n");
							fprintf(stderr, BREAK_LINE);
						}
						else {
							fprintf(stderr, "PrintError validating command-line inputs. Terminating!\n");
							exit(1);
						}
						PrintProgramParameters(stderr, &arguments);
						/* Execute Program */
						ReadReferenceGenome(arguments.rgListFileName,
								&rgList,
								arguments.startChr,
								arguments.startPos,
								arguments.endChr,
								arguments.endPos);
						/* Run the aligner */
						RunAligner(&rgList,
								arguments.matchesFileName,
								arguments.scoringMatrixFileName,
								arguments.algorithm,
								arguments.offsetLength,
								arguments.pairedEnd,
								arguments.outputID,
								arguments.outputDir);
						break;
					default:
						fprintf(stderr, "PrintError determining program mode. Terminating!\n");
						exit(1);
				}
				endTime = time(NULL);
				int seconds = endTime - startTime;
				int hours = seconds/3600;
				seconds -= hours*3600;
				int minutes = seconds/60;
				seconds -= minutes*60;

				fprintf(stderr, "Time elapsed: %d hours, %d minutes and %d seconds.\n",
						hours,
						minutes,
						seconds
					   );

				fprintf(stderr, "Terminating successfully!\n");
				fprintf(stderr, "%s", BREAK_LINE);

			}
			else {
				fprintf(stderr, "PrintError parsing command line arguments!\n");
				exit(1);
			}
	}
	else {
		GetOptHelp();
#ifdef HAVE_ARGP_H
		/* fprintf(stderr, "Type \"%s --help\" to see usage\n", argv[0]); */
#else
		/*     fprintf(stderr, "Type \"%s -h\" to see usage\n", argv[0]); */
#endif
	}
	return 0;
}

int ValidateInputs(struct arguments *args) {

	char *FnName="ValidateInputs";

	fprintf(stderr, BREAK_LINE);
	fprintf(stderr, "Checking input parameters supplied by the user ...\n");

	if(args->rgListFileName!=0) {
		fprintf(stderr, "Validating rgListFileName %s. \n",
				args->rgListFileName);
		if(ValidateFileName(args->rgListFileName)==0)
			PrintError(FnName, "rgListFileName", "Command line argument", Exit, IllegalFileName);
	}

	if(args->matchesFileName!=0) {
		fprintf(stderr, "Validating matchesFileName path %s. \n",
				args->matchesFileName);
		if(ValidateFileName(args->matchesFileName)==0)
			PrintError(FnName, "matchesFileName", "Command line argument", Exit, IllegalFileName);
	}

	if(args->scoringMatrixFileName!=0) {
		fprintf(stderr, "Validating scoringMatrixFileName path %s. \n",
				args->scoringMatrixFileName);
		if(ValidateFileName(args->scoringMatrixFileName)==0)
			PrintError(FnName, "scoringMatrixFileName", "Command line argument", Exit, IllegalFileName);
	}

	if(args->algorithm < MIN_ALGORITHM || args->algorithm > MAX_ALGORITHM) {
		PrintError(FnName, "algorithm", "Command line argument", Exit, OutOfRange);
	}

	if(args->startChr < 0) {
		PrintError(FnName, "startChr", "Command line argument", Exit, OutOfRange);
	}

	if(args->startPos < 0) {
		PrintError(FnName, "startPos", "Command line argument", Exit, OutOfRange);
	}

	if(args->endChr < 0) {
		PrintError(FnName, "endChr", "Command line argument", Exit, OutOfRange);
	}

	if(args->endPos < 0) {
		PrintError(FnName, "endPos", "Command line argument", Exit, OutOfRange);
	}

	if(args->offsetLength < 0) {
		PrintError(FnName, "offsetLength", "Command line argument", Exit, OutOfRange);
	}

	if(args->pairedEnd < 0 || args->pairedEnd > 1) {
		PrintError(FnName, "pairedEnd", "Command line argument", Exit, OutOfRange);
	}

	if(args->outputID!=0) {
		fprintf(stderr, "Validating outputID path %s. \n",
				args->outputID);
		if(ValidateFileName(args->outputID)==0)
			PrintError(FnName, "outputID", "Command line argument", Exit, IllegalFileName);
	}

	if(args->outputDir!=0) {
		fprintf(stderr, "Validating outputDir path %s. \n",
				args->outputDir);
		if(ValidateFileName(args->outputDir)==0)
			PrintError(FnName, "outputDir", "Command line argument", Exit, IllegalFileName);
	}

	return 1;
}

	int 
ValidateFileName(char *Name) 
{
	/* 
	   Checking that strings are good: FileName = [a-zA-Z_0-9][a-zA-Z0-9-.]+
	   FileName can start with only [a-zA-Z_0-9]
	   */

	char *ptr=Name;
	int counter=0;
	/*   fprintf(stderr, "Validating FileName %s with length %d\n", ptr, strlen(Name));  */

	assert(ptr!=0);

	while(*ptr) {
		if((isalnum(*ptr) || (*ptr=='_') || (*ptr=='+') || 
					((*ptr=='.') /* && (counter>0)*/) || /* FileNames can't start  with . or - */
					((*ptr=='/')) || /* Make sure that we can navigate through folders */
					((*ptr=='-') && (counter>0)))) {
			ptr++;
			counter++;
		}
		else return 0;
	}
	return 1;
}

	void 
AssignDefaultValues(struct arguments *args)
{
	/* Assign default values */

	args->programMode = ExecuteProgram;

	args->rgListFileName =
		(char*)malloc(sizeof(DEFAULT_FILENAME));
	assert(args->rgListFileName!=0);
	strcpy(args->rgListFileName, DEFAULT_FILENAME);

	args->matchesFileName =
		(char*)malloc(sizeof(DEFAULT_FILENAME));
	assert(args->matchesFileName!=0);
	strcpy(args->matchesFileName, DEFAULT_FILENAME);

	args->scoringMatrixFileName =
		(char*)malloc(sizeof(DEFAULT_FILENAME));
	assert(args->scoringMatrixFileName!=0);
	strcpy(args->scoringMatrixFileName, DEFAULT_FILENAME);

	args->algorithm = DEFAULT_ALGORITHM;
	args->startChr=0;
	args->startPos=0;
	args->endChr=0;
	args->endPos=0;
	args->offsetLength=0;
	args->pairedEnd = 0;

	args->outputID =
		(char*)malloc(sizeof(DEFAULT_FILENAME));
	assert(args->outputID!=0);
	strcpy(args->outputID, DEFAULT_FILENAME);

	args->outputDir =
		(char*)malloc(sizeof(DEFAULT_FILENAME));
	assert(args->outputDir!=0);
	strcpy(args->outputDir, DEFAULT_FILENAME);

	return;
}

	void 
PrintProgramParameters(FILE* fp, struct arguments *args)
{
	char programmode[3][64] = {"ExecuteGetOptHelp", "ExecuteProgram", "ExecutePrintProgramParameters"};
	fprintf(fp, BREAK_LINE);
	fprintf(fp, "Printing Program Parameters:\n");
	fprintf(fp, "programMode:\t\t\t\t%d\t[%s]\n", args->programMode, programmode[args->programMode]);
	fprintf(fp, "rgListFileName:\t\t\t\t%s\n", args->rgListFileName);
	fprintf(fp, "matchesFileName\t\t\t\t%s\n", args->matchesFileName);
	fprintf(fp, "scoringMatrixFileName\t\t\t%s\n", args->scoringMatrixFileName);
	fprintf(fp, "algorithm:\t\t\t\t%d\n", args->algorithm);
	fprintf(fp, "startChr:\t\t\t\t%d\n", args->startChr);
	fprintf(fp, "startPos:\t\t\t\t%d\n", args->startPos);
	fprintf(fp, "endChr:\t\t\t\t\t%d\n", args->endChr);
	fprintf(fp, "endPos:\t\t\t\t\t%d\n", args->endPos);
	fprintf(fp, "offsetLength:\t\t\t\t%d\n", args->offsetLength);
	fprintf(fp, "pairedEnd:\t\t\t\t%d\n", args->pairedEnd);
	fprintf(fp, "outputID:\t\t\t\t%s\n", args->outputID);
	fprintf(fp, "outputDir:\t\t\t\t%s\n", args->outputDir);
	fprintf(fp, BREAK_LINE);
	return;

}

void
GetOptHelp() {

	struct argp_option *a=options;
	fprintf(stderr, "\nUsage: danalyze [options]\n");
	while((*a).group>0) {
		switch((*a).key) {
			case 0:
				fprintf(stderr, "\n%s\n", (*a).doc); break;
			default:
				fprintf(stderr, "-%c\t%12s\t%s\n", (*a).key, (*a).arg, (*a).doc); break;
		}
		a++;
	}
	return;
}

void
PrintGetOptHelp() {

	struct argp_option *a=options;
	fprintf(stderr, "%s\n", argp_program_version);
	fprintf(stderr, "\nUsage: danalyze [options]\n");
	while((*a).group>0) {
		switch((*a).key) {
			case 0:
				fprintf(stderr, "\n%s\n", (*a).doc); break;
			default:
				fprintf(stderr, "-%c\t%12s\t%s\n", (*a).key, (*a).arg, (*a).doc); break;
		}
		a++;
	}
	fprintf(stderr, "\n%s\n", argp_program_bug_address);
	return;
}

#ifdef HAVE_ARGP_H
	static error_t
parse_opt (int key, char *arg, struct argp_state *state) 
{
	struct arguments *arguments = state->input;
#else
	int
		getopt_parse(int argc, char** argv, char OptionString[], struct arguments* arguments) 
		{
			char key;
			int OptErr=0;
			while((OptErr==0) && ((key = getopt (argc, argv, OptionString)) != -1)) {
				/*
				   fprintf(stderr, "Key is %c and OptErr = %d\n", key, OptErr);
				   */
#endif
				switch (key) {
					case '2':
						arguments->pairedEnd = 1;break;

					case 'a':
						arguments->algorithm=atoi(OPTARG);break;
					case 'd':
						if(arguments->outputDir) free(arguments->outputDir);
						arguments->outputDir = OPTARG;break;
					case 'e':
						arguments->endChr=atoi(OPTARG);break;
					case 'h':
						arguments->programMode=ExecuteGetOptHelp; break;
					case 'm':
						if(arguments->matchesFileName) free(arguments->matchesFileName);
						arguments->matchesFileName = OPTARG;break;
					case 'o':
						if(arguments->outputID) free(arguments->outputID);
						arguments->outputID = OPTARG;break;
					case 'p':
						arguments->programMode=ExecutePrintProgramParameters; break;
					case 'r':
						if(arguments->rgListFileName) free(arguments->rgListFileName);
						arguments->rgListFileName = OPTARG;break;
					case 's':
						arguments->startChr=atoi(OPTARG);break;
					case 'x':
						if(arguments->scoringMatrixFileName) free(arguments->scoringMatrixFileName);
						arguments->scoringMatrixFileName = OPTARG;break;
					case 'E':
						arguments->endPos=atoi(OPTARG);break;
					case 'O':
						arguments->offsetLength=atoi(OPTARG);break;
					case 'S':
						arguments->startPos=atoi(OPTARG);break;
					default:
#ifdef HAVE_ARGP_H
						return ARGP_ERR_UNKNOWN;
				} /* switch */
				return 0;
#else
				OptErr=1;
			} /* while */
		} /* switch */
	return OptErr;
#endif
}
