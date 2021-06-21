/*
* Copyright Newcastle University, UK.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

// Open Movement File Synthesizer
// Dan Jackson

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "omsynth.h"


static double scale(const char *str)
{
	//fprintf(stderr, "SCALE: string=%s\n", str);
	char *end = NULL;
	double value = strtod(str, &end);
	while (end != NULL && *end != '\0' && *end == '/')
	{
		double divisor = strtod(end + 1, &end);
		if (divisor == 0) 
		{ 
			fprintf(stderr, "WARNING: Invalid scale has divide by zero.\n");
			value = 0.0;
		}
		else 
		{
			value /= divisor;
		}
	}
	//fprintf(stderr, "SCALE: value=%f\n", value);
	return value;
}


int main(int argc, char *argv[])
{
	int i;
	bool help = false;
	int positional = 0;
	int ret;
	omsynth_settings_t settings = { 0 };

	// Default settings
	memset(&settings, 0, sizeof(settings));
	settings.packed = false;
	settings.rate = 100;
	settings.range = 8;
	settings.scale = 1.0f;				// Use "1/256" if input is in raw units
	settings.gyro = -1;

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--help") == 0) { help = true; }
		else if (strcmp(argv[i], "-in") == 0) { settings.filename = argv[++i]; }
		else if (strcmp(argv[i], "-out") == 0) { settings.outFilename = argv[++i]; }
		else if (strcmp(argv[i], "-packed") == 0) { settings.packed = true; }
		else if (strcmp(argv[i], "-unpacked") == 0) { settings.packed = false; }
		else if (strcmp(argv[i], "-silent") == 0) { settings.silent = true; }
		else if (strcmp(argv[i], "-rate") == 0) { settings.rate = atoi(argv[++i]); }
		else if (strcmp(argv[i], "-range") == 0) { settings.range = atoi(argv[++i]); }
		else if (strcmp(argv[i], "-scale") == 0) { settings.scale = scale(argv[++i]); }
		else if (strcmp(argv[i], "-gyro") == 0)
		{
			i++;
			if (strcmp(argv[i], "none") == 0) { settings.gyro = -1; }
			else if (strcmp(argv[i], "off") == 0) { settings.gyro = 0; }
			else settings.gyro = atoi(argv[i]);
		}
		else if (argv[i][0] == '-')
		{
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			help = true;
		}
		else
		{
			if (positional == 0)
			{
				settings.filename = argv[i];
			}
			else
			{
				fprintf(stderr, "Unknown positional parameter (%d): %s\n", positional + 1, argv[i]);
				help = true;
			}
			positional++;
		}
	}

	if (settings.range != 2 && settings.range != 4 && settings.range != 8 && settings.range != 16)
	{
		fprintf(stderr, "Invalid range: %d\n", settings.range);
		help = true;
	}

	if (help)
	{
		fprintf(stderr, "omsynth OM File Synthesizer Tool\n");
		fprintf(stderr, "V1.10\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Usage: omsynth [options...]\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "options:\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "\t[-in] <input.csv>						Input file (defaults to stdin)\n");
		fprintf(stderr, "\t-out <output.synth.cwa>					Output file (defaults to stdout)\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "\t-unpacked|packed							Use 'unpacked' storage mode (default), or 'packed' storage mode (accel. only)\n");
		fprintf(stderr, "\t-scale <scale>							Accelerometer input scaling (default=1; e.g. for raw AX3 units: 1/256)\n");
		fprintf(stderr, "\t-rate 25|50|100|200|400|800|1600|3200    Configured sampling rate in Hz (default=100; sector-rate from data)\n");
		fprintf(stderr, "\t-range 2|4|8|16							Accelerometer range in +/- g (default=8)\n");
		fprintf(stderr, "\t-silent									No progress output\n");
		fprintf(stderr, "\t-gyro none|off|125|250|500|1000|2000     none/-1=AX3 (default); off/0=AX6 accel-only mode; 125/250/500/1000/2000 degrees/second\n");
		fprintf(stderr, "\n");
		ret = -1;
	}
	else
	{
		// Run
		ret = OmSynthRun(&settings);
	}

#if defined(_WIN32) && defined(_DEBUG)
	if (IsDebuggerPresent()) { fprintf(stderr, "\nPress [enter] to exit <%d>....", ret); getc(stdin); }
#endif

	return ret;
}

