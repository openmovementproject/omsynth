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
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#define _strcasecmp _stricmp
#else
#include <strings.h>
#define _strcasecmp strcasecmp
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "omsynth.h"
#include "import_csv.h"
#include "cwa_writer.h"


int OmSynthRun(omsynth_settings_t *settings)
{
	// Open input and parse any CSV header
	import_csv_t importer;
	if (!settings->silent)
	{
		if (settings->filename == NULL || settings->filename[0] == '\0')
		{
			fprintf(stderr, "INPUT: (stdin), Ctrl+C to cancel.\n");
		}
		else
		{
			fprintf(stderr, "INPUT: %s\n", settings->filename);
		}
	}
	if (ImportCsvOpen(&importer, settings->filename, settings->scale) != 0)
	{
		fprintf(stderr, "ERROR: Problem opening input .CSV file: %s\n", settings->filename);
		return -1;
	}
	
	// Start output file
	cwa_writer_settings_t cwaSettings = {0};
	cwaSettings.deviceId = 0;
	cwaSettings.sessionId = 0;
	cwaSettings.loggingStartTime = 0;  				// 0 = always start
	cwaSettings.loggingEndTime = 0xffffffff;  // 0xffffffff = never end
	cwaSettings.rate = 100;
	cwaSettings.range = settings->range;
	cwaSettings.packed = settings->packed;

	cwa_writer_t cwaWriter;
	if (!settings->silent)
	{
		if (settings->filename == NULL || settings->filename[0] == '\0')
		{
			fprintf(stderr, "OUTPUT: (stdout)\n");
#ifdef _WIN32
			_setmode(_fileno(stdout), _O_BINARY);
#endif
		}
		else
		{
			fprintf(stderr, "OUTPUT: %s\n", settings->outFilename);
		}
	}
	if (CwaWriterOpen(&cwaWriter, settings->outFilename, &cwaSettings) != 0)
	{
		fprintf(stderr, "ERROR: Problem opening output .CWA file: %s\n", settings->outFilename);
		return -2;
	}

	// All values in the input
	double values[4];
	for (unsigned int line = 0; ; line++)
	{
		int samples = ImportCsvNextSample(&importer, values);
		if (samples < 0) break;		// End of file
		if (samples < 4) continue;	// ignore non-full data lines
		
		// Append to output file
		if (!CwaWriterWriteSample(&cwaWriter, (const cwa_writer_sample_t *)&values[0]))
		{
			printf("ERROR: Problem writing output\n");
			break;
		}

		// Progress report (approximately one per hour at 100Hz)
		if (!settings->silent && line % (100 * 60 * 24) == 0) { fprintf(stderr, "."); }
	}	

	// Cleanly end output file
	CwaWriterClose(&cwaWriter);

	// End of file, close input
	ImportCsvClose(&importer);

	if (!settings->silent) fprintf(stderr, "DONE\n");
	return 0;
}

