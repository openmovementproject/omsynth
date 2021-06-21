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


// Open Movement CWA File Writer
// Dan Jackson

#ifndef CWA_WRITER_H
#define CWA_WRITER_H

#include <stdio.h>
#include <stdbool.h>

typedef struct
{
  unsigned int deviceId;            // 0 is default
  unsigned int sessionId;           // 0 is default
  unsigned int loggingStartTime;    // 0 = always start
  unsigned int loggingEndTime;      // 0xffffffff = never end
  unsigned int rate;                // configured sample rate (default 100)
  unsigned char range;              // 2/4/8/16 (default 8)
  int gyro;							// Gyro range: -1=AX3 (default); 0=AX6 accel-only mode; 125/250/500/1000/2000 degrees/second
  bool packed;                      // default false
} cwa_writer_settings_t;

#define CWA_MAX_SAMPLES 120
typedef struct
{
  double t, ax, ay, az, gx, gy, gz;
} cwa_writer_sample_t;


typedef struct
{
  cwa_writer_settings_t settings;
  FILE *fp;

  unsigned int sequenceId;

  cwa_writer_sample_t samples[CWA_MAX_SAMPLES];
  int sampleCount;
  double firstSampleTime;
  double lastTime;

  unsigned char lastRateCode;
  unsigned char hardwareType;       // 0x00=AX3, 0x64=AX6
  unsigned char sensorConfig;       // 0xff = accel-only
  unsigned short lastScaleMask;     // 0xAAAGGG0000000000, 0x00 for AX3
} cwa_writer_t;

int CwaWriterOpen(cwa_writer_t *writer, const char *filename, const cwa_writer_settings_t *settings);
bool CwaWriterWriteSample(cwa_writer_t *writer, const cwa_writer_sample_t *sample);
void CwaWriterClose(cwa_writer_t *writer);

#endif
