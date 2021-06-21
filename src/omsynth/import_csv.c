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

#include "import_csv.h"
#include "timestamp.h"


int ImportCsvOpen(import_csv_t *importer, const char *filename, double scale)
{
	memset(importer, 0, sizeof(import_csv_t));
	importer->scale = scale;
	importer->colTime = -1;
	importer->colAX = -1;
	importer->colAY = -1;
	importer->colAZ = -1;
	importer->colGX = -1;
	importer->colGY = -1;
	importer->colGZ = -1;

	//fprintf(stderr, "Processing data: %s\n", filename);
	int headerCells = CsvOpen(&importer->csv, filename, CSV_HEADER_DETECT_NON_NUMERIC, CSV_SEPARATORS);
	if (headerCells == 0)
	{
		fprintf(stderr, "WARNING: No header line -- default columns will be used.\n");
		importer->colTime = 0;
		importer->colAX = 1;
		importer->colAY = 2;
		importer->colAZ = 3;
		importer->colGX = 4;
		importer->colGY = 5;
		importer->colGZ = 6;
	}
	else
	{
		// Parse header cells
		int i;
		for (i = 0; i < headerCells; i++)
		{
			const char *heading = CsvTokenString(&importer->csv, i);
			while (*heading == ' ') heading++;	// left-trim
			if (!_strcasecmp(heading, "Time")) { importer->colTime = i; }
			else if (!_strcasecmp(heading, "Accel-X (g)")) { importer->colAX = i; }
			else if (!_strcasecmp(heading, "Accel-Y (g)")) { importer->colAY = i; }
			else if (!_strcasecmp(heading, "Accel-Z (g)")) { importer->colAZ = i; }
			else if (!_strcasecmp(heading, "Gyro-X (d/s)")) { importer->colGX = i; }
			else if (!_strcasecmp(heading, "Gyro-Y (d/s)")) { importer->colGY = i; }
			else if (!_strcasecmp(heading, "Gyro-Z (d/s)")) { importer->colGZ = i; }
			else
			{
				fprintf(stderr, "WARNING: Unknown data column %d heading: '%s'.\n", i + 1, heading);
			}
		}
	}

	if (importer->colTime < 0 || importer->colAX < 0 || importer->colAY < 0 || importer->colAZ < 0)
	{
		fprintf(stderr, "ERROR: One or more of the required columns are missing.\n");
		return -1;
	}
	
	return 0;
}

int ImportCsvNextSample(import_csv_t *importer, double *txyz)
{
	int tokens;
	if ((tokens = CsvReadLine(&importer->csv)) >= 0)
	{
		if (tokens > importer->colTime && tokens > importer->colAX && tokens > importer->colAY && tokens > importer->colAZ)
		{
			txyz[0] = TimeParse(CsvTokenString(&importer->csv, importer->colTime));
			txyz[1] = CsvTokenFloat(&importer->csv, importer->colAX) * importer->scale;
			txyz[2] = CsvTokenFloat(&importer->csv, importer->colAY) * importer->scale;
			txyz[3] = CsvTokenFloat(&importer->csv, importer->colAZ) * importer->scale;	

			if (tokens > importer->colGX && tokens > importer->colGY && tokens > importer->colGZ)
			{
				txyz[4] = CsvTokenFloat(&importer->csv, importer->colGX);
				txyz[5] = CsvTokenFloat(&importer->csv, importer->colGY);
				txyz[6] = CsvTokenFloat(&importer->csv, importer->colGZ);
				return 1 + 3 + 3;	// time + 6-axis data
			}
			else
			{
				txyz[4] = 0;
				txyz[5] = 0;
				txyz[6] = 0;
			}
			return 1 + 3;	// time + 3-axis data
		}
		else if (tokens > 0)	// Ignore completely blank lines
		{
			fprintf(stderr, "WARNING: Too-few columns, ignoring row on line %d.\n", CsvLineNumber(&importer->csv));
			return 0;	// not full data
		}
	}
	return -1;	// end of stream
}

void ImportCsvClose(import_csv_t *importer)
{
	CsvClose(&importer->csv);
}
