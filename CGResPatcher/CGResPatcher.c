#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/*
	Usage:
		-r: Read current exe values. No writing.
			-i: specify ini for input.
			-e: specify exe to use
			-o: outputs results to an ini file.
			-c: complete dump, including hidden values.
		-w: Patch exe.
			-i: Specify ini file for input.
			-e: Specify the exe to patch.
			//-F: force overwrite. Does not prompt. Does NOT imply -w.
		-g: Generate default ini file
			-o: specifics name of ini file.
			-c: complete dump, including hidden values.
		-h: help: prints usage.
*/

// Magic numbers defined

#define DEFAULT_INI "CGResPatcher.ini"
#define NUM_RESOLUTIONS 9
#define OFFSET_RESOLUTIONS 0x000C15F6	// This assumes version 1.00.00. No patching of the exe, changing its length.

// Typedefs and Data structures

typedef struct
{
	int x;
	int y;
} sResolution;

typedef enum
{
	MODE_HELP,
	MODE_READ,
	MODE_WRITE,
	MODE_GENERATE
} eMode;

typedef enum
{
	ERROR_NOERROR,
	ERROR_NOCODE,
	ERROR_NOINI,
	ERROR_NOEXE,
	ERROR_FAILWRITE,
	ERROR_BADPARAMS
} eErrors;

// Globals

sResolution g_resolutionEntries[NUM_RESOLUTIONS] = { 0 };
unsigned long g_offset = OFFSET_RESOLUTIONS;
char* g_szExePath = NULL;
bool g_bCheckCurrentPath = false;

// Function prototypes

void PrintUsage(void);
eErrors ParseParameters(int argc, char **argv, eMode* mode, char** input, char** output, char** exeLocation, bool* bComplete, bool* bForce);
const char* GetErrorMsgFromCode(eErrors code);
eErrors WriteGlobalVariablesToIni(char *, bool, bool);
eErrors ReadGlobalVariablesFromIni(char *);
int PatchExe(void);
void ReadResolution(FILE* f, int *r);
void WriteResolution(FILE* f, int r);

// Function definitions

void PrintUsage(void)
{
	const char * szUsage = \
	"*****************************************\n" \
	"*** CHResPatcher (C)2021 BibleClinger ***\n" \
	"*****************************************\n" \
		"-r : Read current exe values. No writing.\n" \
			"\t-i : specify ini for input.\n" \
			"\t-o : outputs results to an ini file.\n" \
		"-w : Patch the exe.\n" \
			"\t-i : Specify ini file for input.\n" \
			"\t-e: Specify the exe to patch.\n" \
			"\t-F : force overwrite. Does not prompt.Does NOT imply -w.\n" \
		"-g : Generate default ini file\n" \
			"\t- o : specifics name of ini file.\n" \
			"\t- c : complete dump, including hidden values.\n" \
		"-h : Help : Prints usage.\n";

	puts(szUsage);
}

eErrors ParseParameters(int argc, char **argv, eMode *mode, char **input, char **output, char **exeLocation, bool *bComplete, bool *bForce)
{
	bool bModeSet = false;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-r") == 0)
		{
			if (bModeSet)
			{
				return ERROR_BADPARAMS;
			}
			*mode = MODE_READ;
			bModeSet = true;
		}
		else if (strcmp(argv[i], "-w") == 0)
		{
			if (bModeSet)
			{
				return ERROR_BADPARAMS;
			}
			*mode = MODE_WRITE;
			bModeSet = true;
		}
		else if (strcmp(argv[i], "-h") == 0)
		{
			if (bModeSet)
			{
				return ERROR_BADPARAMS;
			}
			*mode = MODE_HELP;
			bModeSet = true;
			break;
		}
		else if (strcmp(argv[i], "-g") == 0)
		{
			if (bModeSet)
			{
				return ERROR_BADPARAMS;
			}
			*mode = MODE_GENERATE;
			bModeSet = true;
		}
		else if (strcmp(argv[i], "-i") == 0)
		{
			if (i + 1 < argc)
			{
				*input = argv[i + 1];
				i++;
			}
			else return ERROR_BADPARAMS;
		}
		else if (strcmp(argv[i], "-o") == 0)
		{
			if (i + 1 < argc)
			{
				*output = argv[i + 1];
				i++;
			}
			else return ERROR_BADPARAMS;
		}
		else if (strcmp(argv[i], "-F") == 0)
		{
			*bForce = true;
		}
		else if (strcmp(argv[i], "-c") == 0)
		{
			*bComplete = true;
		}
		else if (strcmp(argv[i], "-e") == 0)
		{
			*exeLocation = argv[i + 1];
		}
	}
	return ERROR_NOERROR;
}

eErrors WriteGlobalVariablesToIni(char* szFile, bool bUseInternalDefaults, bool bWriteComplete)
{
	FILE* f;
	const char* const szPayloadFormat = \
		"[Paths]\n" \
		"exePath = %s\n\n" \
		"[Resolutions]\n" \
		"Free1 = %dx%d; [A:0!| Z=0] 320x240:  Free Window Slot\n" \
		"Free2 = %dx%d; [A:1!| Z=1] 400x300:  Free Window Slot\n" \
		"Free3 = %dx%d; [A:2!| Z=2] 512x384:  Free Window Slot\n" \
		"Main = %dx%d; [A:3!| Z=3] 640x480:  Main Slot : This also functions as your main window-mode resolution!\n" \
		"Expanded1 = %dx%d; [A:4!| Z=4] 800x600:  First Expanded slot for fullscreen.\n" \
		"Expanded2 = %dx%d; [A:5!| Z=5] 1024x768: First Expanded slot for fullscreen.\n" \
		"Expanded3 = %dx%d; [A:6!| Z=6] 1280x1024: Third Expanded slot for fullscreen.Note : Default value is non-4:3. The 5:4 resolution does not fit.\n" \
		"Stereo1 = %dx%d; [A:7!| Z=7] 320x400: Not recommended.\n" \
		"Stereo2 = %dx%d; [AA:0!| Z=8] 320x480: Not recommended.\n";
	if(!szFile)
	{
		szFile = DEFAULT_INI;
	}

	printf("szFile = %s\n", szFile);

	if (fopen_s(&f, szFile, "w"))
	{
		return ERROR_FAILWRITE; // Failed to open the file.
	}
	if (bUseInternalDefaults)
	{
		fprintf(f, szPayloadFormat, "..\\wc3.exe", 320, 240, 400, 300, 512, 384, 640, 480, 800, 600, 1024, 768, 1280, 960, 320, 400, 320, 480); // We write.
	}
	else
	{
		fprintf(f, szPayloadFormat, g_szExePath,g_resolutionEntries[0].x, g_resolutionEntries[0].y,\
												g_resolutionEntries[1].x, g_resolutionEntries[1].y,\
												g_resolutionEntries[2].x, g_resolutionEntries[2].y,\
												g_resolutionEntries[3].x, g_resolutionEntries[3].y,\
												g_resolutionEntries[4].x, g_resolutionEntries[4].y,\
												g_resolutionEntries[5].x, g_resolutionEntries[5].y,\
												g_resolutionEntries[6].x, g_resolutionEntries[6].y,\
												g_resolutionEntries[7].x, g_resolutionEntries[7].y,\
												g_resolutionEntries[8].x, g_resolutionEntries[8].y); // We write.
	}
	if(bWriteComplete)
	{
		fprintf(f, "[Secret]\nOffset=%lx", g_offset);
	}
	fclose(f); // We close the file responsibly.
	return ERROR_NOERROR;
}

eErrors PseudoGlobalVariablesFromIni(char* szFile)
{
	g_resolutionEntries[0] = (sResolution) {320, 240};
	g_resolutionEntries[1] = (sResolution) {400, 300},
	g_resolutionEntries[2] = (sResolution) {512, 384},
	g_resolutionEntries[3] = (sResolution) {640, 480},
	g_resolutionEntries[4] = (sResolution) {800, 600},
	g_resolutionEntries[5] = (sResolution) {1024, 768},
	g_resolutionEntries[6] = (sResolution) {1280, 1024},
	g_resolutionEntries[7] = (sResolution) {320, 400},
	g_resolutionEntries[8] = (sResolution) {320, 480},
	g_offset = OFFSET_RESOLUTIONS;
	g_szExePath = "..\\wc3.exe";
	g_bCheckCurrentPath = true;

	return ERROR_NOERROR;
}

eErrors ReadGlobalVariablesFromIni(char *szFile)
{
	//TODO Finish for real.
  	if(!szFile)
	{
		szFile = DEFAULT_INI;
	}
	//return 0;
	return PseudoGlobalVariablesFromIni(szFile);
}

const char* GetErrorMsgFromCode(eErrors code)
{
	const char * const a_szMessages[] = {
						"No errro. Why are you getting this error code?",		// ERROR_NOERROR
						"Bad error code. Something went seriously wrong.",		// ERROR_NOCODE
						"Unable to find INI file. Did you copy the exe without the ini file?",	// ERROR_NOINI
						"Cannot open the exe to patch. Check your ini or path to the exe, and make sure it's writeable.", // ERROR_NOEXE
						"Could not write to the file in question.", // ERROR_FAILWRITE
						"Bad parameters. Check your command line arguments like -i and -o." // ERROR_BADPARAMS
	};

	if ((code >= 0) && (code < (sizeof(a_szMessages) / sizeof(*a_szMessages))))
	{	// We have a code within scope.
		return a_szMessages[code];
	}
	else return a_szMessages[ERROR_NOCODE];
}

int ReadExe()
{
	FILE* f;

	if (fopen_s(&f, g_szExePath, "r"))
	{
		return ERROR_NOEXE;
	}

	fseek(f, g_offset, 0);
	for (int i = 0; i < NUM_RESOLUTIONS; i++)
	{
		// First we print all the x values.
		ReadResolution(f, &(g_resolutionEntries[i].x));
	}
	for (int i = 0; i < NUM_RESOLUTIONS; i++)
	{
		// Secondly, we print all the y values.
		ReadResolution(f, &(g_resolutionEntries[i].y));
	}
	fclose(f); // We responsibly close the file.
	return 0;
}

int PatchExe()
{	//TODO
	FILE *f;
	
	if(fopen_s(&f, g_szExePath, "w"))
	{
		return ERROR_NOEXE;
	}

	fseek(f, g_offset, 0);
	for (int i = 0; i < NUM_RESOLUTIONS; i++)
	{
		// First we print all the x values.
		WriteResolution(f, g_resolutionEntries[i].x);
	}
	for (int i = 0; i < NUM_RESOLUTIONS; i++)
	{
		// Secondly, we print all the y values.
		WriteResolution(f, g_resolutionEntries[i].y);
	}
	fclose(f); // We responsibly close the file.
	return 0;
}

void ReadResolution(FILE* f, int *r)
{
	fread(r, 4, 1, f);
}

void WriteResolution(FILE *f, int r)
{
	/*
	r = r & 0xFF;
	fwrite(&r, 1, 1, f);
	r = r >> 8 & 0xFF;
	fwrite(&r, 1, 1, f);
	r = r >> 8 & 0xFF;
	fwrite(&r, 1, 1, f);
	r = r >> 8 & 0xFF;
	fwrite(&r, 1, 1, f);
	*/
	fwrite(&r, 4, 1, f);
}

int main(int argc, char *argv[])
{
	int ret;
	eErrors error;
	eMode mode = MODE_HELP;
	char* input = NULL, *output = NULL, *exeLocation = NULL;
	bool bComplete = 0, bForce = 0;

	if((error = ParseParameters(argc, argv, &mode, &input, &output, &exeLocation, &bComplete, &bForce)) != ERROR_NOERROR)
	{
		printf("Error: %s", GetErrorMsgFromCode(error));
		return error;
	}

	printf("Mode: %d\n", mode);
	
	if(mode == MODE_HELP)
	{
			PrintUsage();
			return 0;
	}

	if(mode == MODE_GENERATE)
	{
		printf("About to write...\n");
		if((error = WriteGlobalVariablesToIni(output, true, bComplete)) != ERROR_NOERROR)
		{
			printf("Error: %s", GetErrorMsgFromCode(error));
			return error;
		}
		printf("Done generating ini file.\n");
		return 0;
	}
	
	if (ReadGlobalVariablesFromIni(input) != ERROR_NOERROR)
	{
		printf("Error: %s", GetErrorMsgFromCode(ERROR_NOINI));
		return ERROR_NOINI;
	}

	if (exeLocation != NULL)
	{
		g_szExePath = exeLocation; // Override exe Location.
	}
	
	switch(mode)
	{
		case MODE_READ:
			if((error = ReadExe()) != ERROR_NOERROR)
			{
				printf("Error: %s", GetErrorMsgFromCode(error));
				return error;
			}
			for (int i = 0; i < NUM_RESOLUTIONS; i++)
			{
				printf("Resolution %d: %dx%d\n", i + 1, g_resolutionEntries[i].x, g_resolutionEntries[i].y);
			}
			if (output != NULL)
			{
				if ((error = WriteGlobalVariablesToIni(output, false, bComplete)) != ERROR_NOERROR)
				{
					printf("Error: %s", GetErrorMsgFromCode(error));
					return error;
				}
			}
		break;
		case MODE_WRITE:
			// Write
			if ((error = PatchExe()) != ERROR_NOERROR)
			{
				printf("Error: %s", GetErrorMsgFromCode(error));
				return error;
			}
		break;
	}
	
	return 0;
}