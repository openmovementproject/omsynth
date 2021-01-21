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

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define gmtime_r(_time_t, _tm) gmtime_s(_tm, _time_t)
#else
#define _BSD_SOURCE // before <time.h> for gmtime_r()
#endif
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cwa_writer.h"

#define SECTOR_SIZE 512

#define OM_DATETIME_FROM_YMDHMS(year, month, day, hours, minutes, seconds) \
                ( (((unsigned int)((year) % 100) & 0x3f) << 26) \
                | (((unsigned int)(month)        & 0x0f) << 22) \
                | (((unsigned int)(day)          & 0x1f) << 17) \
                | (((unsigned int)(hours)        & 0x1f) << 12) \
                | (((unsigned int)(minutes)      & 0x3f) <<  6) \
                | (((unsigned int)(seconds)      & 0x3f)      ) \
)

// Accelerometer sampling rate codes
#define OM_ACCEL_RATE_FROM_CONFIG(_f)   (3200 / (1 << (15-((_f) & 0x0f))))
#define OM_ACCEL_RATE_3200  0x0f
#define OM_ACCEL_RATE_1600  0x0e
#define OM_ACCEL_RATE_800   0x0d
#define OM_ACCEL_RATE_400   0x0c
#define OM_ACCEL_RATE_200   0x0b
#define OM_ACCEL_RATE_100   0x0a
#define OM_ACCEL_RATE_50    0x09
#define OM_ACCEL_RATE_25    0x08
#define OM_ACCEL_RATE_12_5  0x07
#define OM_ACCEL_RATE_6_25  0x06

// Top two bits of the sampling rate value are used to determine the acceleromter range
#define OM_ACCEL_RANGE_FROM_CONFIG(_f)  (16 >> ((_f) >> 6))
#define OM_ACCEL_RANGE_16G  0x00
#define OM_ACCEL_RANGE_8G   0x40
#define OM_ACCEL_RANGE_4G   0x80
#define OM_ACCEL_RANGE_2G   0xC0


static int ConfigCode(int rate, int range)
{
  int value = 0;

  // Rate code
  if (rate <= 9) { value |= OM_ACCEL_RATE_6_25; }
  else if (rate <= 18) { value |= OM_ACCEL_RATE_12_5; }
  else if (rate <= 37) { value |= OM_ACCEL_RATE_25; }
  else if (rate < 75)  { value |= OM_ACCEL_RATE_50; }
  else if (rate < 150) { value |= OM_ACCEL_RATE_100; }
  else if (rate < 300) { value |= OM_ACCEL_RATE_200; }
  else if (rate < 600) { value |= OM_ACCEL_RATE_400; }
  else if (rate < 1200) { value |= OM_ACCEL_RATE_800; }
  else if (rate < 2400) { value |= OM_ACCEL_RATE_1600; }
  else { value |= OM_ACCEL_RATE_3200; }

  // Range
  if (range <= 2) { value |= OM_ACCEL_RANGE_2G; }
  else if (range <= 4) { value |= OM_ACCEL_RANGE_4G; }
  else if (range <= 8) { value |= OM_ACCEL_RANGE_8G; }
  else { value |= OM_ACCEL_RANGE_16G; }
    
  return value;
}


// Packs a buffer of short (x,y,z) values into an output buffer (4 bytes per entry)
static void AccelPackData(short *input, unsigned char *output, unsigned char rateCode)
{
  int accelRange = 0x3 - ((rateCode >> 6) & 0x3); // 0b00=2, 0b01=4, 0b10=8, 0b11=16
  unsigned short wordL, wordH;
  // Calculate low and high words of packet data
       if (accelRange == 3) { wordL = ((input[0] >> 3) & 0x03ff) | ((input[1] <<  7) & 0xfc00); wordH = 0xc000 | ((input[2] << 1) & 0x3ff0) | ((input[1] >> 9) & 0x000f); }
  else if (accelRange == 2) { wordL = ((input[0] >> 2) & 0x03ff) | ((input[1] <<  8) & 0xfc00); wordH = 0x8000 | ((input[2] << 2) & 0x3ff0) | ((input[1] >> 8) & 0x000f); }
  else if (accelRange == 1) { wordL = ((input[0] >> 1) & 0x03ff) | ((input[1] <<  9) & 0xfc00); wordH = 0x4000 | ((input[2] << 3) & 0x3ff0) | ((input[1] >> 7) & 0x000f); }
  else                      { wordL = ((input[0]     ) & 0x03ff) | ((input[1] << 10) & 0xfc00); wordH =          ((input[2] << 4) & 0x3ff0) | ((input[1] >> 6) & 0x000f); }
  // Output
  ((unsigned short*)output)[0] = wordL;
  ((unsigned short*)output)[1] = wordH;
  return;
}

// Checksum - 16-bit word-size addition, returns two's compliment of sum (bitwise NOT, then add 1) -- then total sum of words including checksum will be zero.
static unsigned short checksum(const void *buffer, size_t len)
{
  const unsigned short *data = (const unsigned short *)buffer;
  size_t words = (len >> 1);
  unsigned short value = 0x0000;						// Initial sum of zero
  for (; words; --words) { value += *data++; }		// Sum data words
  if (len & 1) { value += ((unsigned char *)buffer)[len - 1]; }          // Add odd byte
  return (~value) + 1;								// ...take bitwise NOT of sum, then add 1 (total sum of words including checksum will be zero)
}

static bool CwaWriterWriteHeader(cwa_writer_t *writer)
{
  unsigned char buffer[SECTOR_SIZE];
  memset(buffer, 0xff, sizeof(buffer));

  // @ 0  +2   packetHeader: ASCII "MD", little-endian (0x444D)
  buffer[0] = 0x4d; buffer[1] = 0x44;

  // @ 2  +2   packetLength: Packet length (1020 bytes, with header (4) = 1024 bytes total)
  buffer[2] = 0xfc; buffer[3] = 0x03;

  // @ 4  +1   hardwareType: Hardware type (0x00/0xff/0x17 = AX3, 0x64 = AX6)
  buffer[4] = writer->hardwareType;     // 0x00=AX3, 0x64=AX6

  // @ 5  +2   deviceId: Device identifier
  buffer[5] = (unsigned char)writer->settings.deviceId; buffer[6] = (unsigned char)(writer->settings.deviceId >> 8);

  // @ 7  +4   sessionId: Unique session identifier
  buffer[7] = (unsigned char)writer->settings.sessionId; buffer[8] = (unsigned char)(writer->settings.sessionId >> 8);
  buffer[9] = (unsigned char)(writer->settings.sessionId >> 8); buffer[10] = (unsigned char)(writer->settings.sessionId >> 8);

  // @11  +2   upperDeviceId: Upper word of device identifier
  buffer[11] = (unsigned char)(writer->settings.deviceId >> 16); buffer[12] = (unsigned char)(writer->settings.deviceId >> 24);

  // @13  +4   loggingStartTime: Start time for delayed logging (0 = always start)
  buffer[13] = (unsigned char)writer->settings.loggingStartTime; buffer[14] = (unsigned char)(writer->settings.loggingStartTime >> 8);
  buffer[15] = (unsigned char)(writer->settings.loggingStartTime >> 8); buffer[16] = (unsigned char)(writer->settings.loggingStartTime >> 8);

  // @17  +4   loggingEndTime: Stop time for delayed logging (0xffffffff = never stop)
  buffer[17] = (unsigned char)writer->settings.loggingEndTime; buffer[18] = (unsigned char)(writer->settings.loggingEndTime >> 8);
  buffer[19] = (unsigned char)(writer->settings.loggingEndTime >> 8); buffer[20] = (unsigned char)(writer->settings.loggingEndTime >> 8);

  // @21  +4   loggingCapacity: (Deprecated: write as 0)
  buffer[21] = 0; buffer[22] = 0; buffer[23] = 0; buffer[24] = 0; 

  // @26  +1   flashLed: Flash LED during recording
  buffer[26] = 0x00;

  // @35  +1   sensorConfig: Fixed rate sensor configuration, 0xff (or 0x00) means accel only, otherwise bottom nibble is gyro range (8000/2^n dps): 2=2000, 3=1000, 4=500, 5=250, 6=125, top nibble non-zero is magnetometer enabled.
  buffer[35] = writer->sensorConfig;    // 0xff = accel-only

  // @36  +1   samplingRate: Sampling rate code, frequency (3200/(1<<(15-(rate & 0x0f)))) Hz, range (+/-g) (16 >> (rate >> 6)).
  buffer[36] = writer->lastRateCode; // 0x4A = 74 = (100,8)

  // @37  +4   lastChangeTime: Last change metadata time
  buffer[37] = 0; buffer[38] = 0; buffer[39] = 0; buffer[40] = 0; 

  // @41  +1   firmwareRevision: Firmware revision number
  buffer[41] = 0x00;  // 0=not written by firmware

  // @64  +448 annotation: meta-data (448 ASCII characters, ignore trailing 0x20/0x00/0xff bytes, url-encoded UTF-8 name-value pairs)
  memset(buffer + 64, 0x20, 448);

  // Write first sector: header
  if (fwrite(buffer, 1, sizeof(buffer), writer->fp) != sizeof(buffer)) { return false; }

  // Write second sector: spare area is 0xff
  memset(buffer, 0xff, sizeof(buffer));
  if (fwrite(buffer, 1, sizeof(buffer), writer->fp) != sizeof(buffer)) { return false; }

  return true;
}

int CwaWriterOpen(cwa_writer_t *writer, const char *filename, const cwa_writer_settings_t *settings)
{
  memset(writer, 0, sizeof(cwa_writer_t));
  writer->settings = *settings;
  if (filename == NULL || filename[0] == '\0') writer->fp = stdout;
  else writer->fp = fopen(filename, "wb");
  if (writer->fp == NULL) { return -1; }
  writer->lastRateCode = ConfigCode(writer->settings.rate, writer->settings.range);

  // TODO: When support gyro, choose hardware type, sensor config and scale mask
  writer->hardwareType = 0x00;      // 0x00=AX3, 0x64=AX6
  writer->sensorConfig = 0xff;      // 0xff = accel-only
  writer->lastScaleMask = 0;        // 0xAAAGGG0000000000, 0x00 for AX3

  if (!CwaWriterWriteHeader(writer)) { return -2; }
  return 0;
}

static bool CwaWriterWriteBlock(cwa_writer_t * writer)
{
  if (writer->sampleCount > 0)
  {
    // TODO: Support 6 channels
    unsigned char numAxesBPS = writer->settings.packed ? 0x30 : 0x32;
    unsigned short deviceFractional;
    unsigned int timestamp;
    short timestampOffset;

    int channels = numAxesBPS >> 4;

    double elapsedTime = writer->lastTime - writer->firstSampleTime;
    // Calculate sample rate, re-use last if only a single sample has been written in the sector
    double sampleRate;
    if (writer->sampleCount > 1 && elapsedTime > 0)
    {
      sampleRate = (writer->sampleCount - 1) / elapsedTime;
    }
    else
    {
      sampleRate = OM_ACCEL_RATE_FROM_CONFIG(writer->lastRateCode);
    }
    writer->lastRateCode = ConfigCode((int)sampleRate, writer->settings.range);

    // Use the timestamp of the first sample
    timestampOffset = 0;
    int timeFractional = (int)(65536 * (writer->firstSampleTime - (int)writer->firstSampleTime));
    time_t t = (int)writer->firstSampleTime;
    struct tm timeTm;
    struct tm *tm = &timeTm;
    gmtime_r(&t, tm);
    timestamp = OM_DATETIME_FROM_YMDHMS(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    // Backwards-compatible shift (undone by reader)
    timestampOffset = timestampOffset - (short)(((unsigned long)timeFractional * OM_ACCEL_RATE_FROM_CONFIG(writer->lastRateCode)) >> 16);

    // Fill sector buffer
    unsigned char buffer[SECTOR_SIZE];
    memset(buffer, 0x00, sizeof(buffer));

    // @ 0  +2   packetHeader: ASCII "AX", little-endian (0x5841)
    buffer[0] = 0x41; buffer[1] = 0x58;

    // @ 2  +2   packetLength: Packet length (508 bytes, with header (4) = 512 bytes total)
    buffer[2] = 0xfc; buffer[3] = 0x01;

    // @ 4  +2   deviceFractional: Top bit set: 15-bit fraction of a second for the time stamp, the timestampOffset was already adjusted to minimize this assuming ideal sample rate; Top bit clear: 15-bit device identifier, 0 = unknown;
    deviceFractional = 0x8000 | (timeFractional >> 1);
    buffer[4] = (unsigned char)deviceFractional; buffer[5] = (unsigned char)(deviceFractional >> 8);

    // @ 6  +4   sessionId: Unique session identifier, 0 = unknown
    buffer[6] = (unsigned char)writer->settings.sessionId; buffer[7] = (unsigned char)(writer->settings.sessionId >> 8);
    buffer[8] = (unsigned char)(writer->settings.sessionId >> 16); buffer[9] = (unsigned char)(writer->settings.sessionId >> 24);

    // @10  +4   sequenceId: Sequence counter (0-indexed), each packet has a new number (reset if restarted)
    buffer[10] = (unsigned char)writer->sequenceId; buffer[11] = (unsigned char)(writer->sequenceId >> 8);
    buffer[12] = (unsigned char)(writer->sequenceId >> 16); buffer[13] = (unsigned char)(writer->sequenceId >> 24);

    // @14  +4   timestamp: Last reported RTC value, 0 = unknown
    buffer[14] = (unsigned char)timestamp; buffer[15] = (unsigned char)(timestamp >> 8);
    buffer[16] = (unsigned char)(timestamp >> 16); buffer[17] = (unsigned char)(timestamp >> 24);

    // @18  +2   lightScale: AAAGGGLLLLLLLLLL Bottom 10 bits is last recorded light sensor value in raw units, 0 = none; top three bits are unpacked accel scale (1/2^(8+n) g); next three bits are gyro scale	(8000/2^n dps)
    unsigned short light = writer->lastScaleMask | 0;
    buffer[18] = (unsigned char)light; buffer[19] = (unsigned char)(light >> 8);

    // @20  +2   temperature: Last recorded temperature sensor value in raw units, 0 = none
    buffer[20] = 0; buffer[21] = 0;

    // @22  +1   events: Event flags since last packet, b0 = resume logging, b1 = reserved for single-tap event, b2 = reserved for double-tap event, b3 = reserved, b4 = reserved for diagnostic hardware buffer, b5 = reserved for diagnostic software buffer, b6 = reserved for diagnostic internal flag, b7 = reserved)
    buffer[22] = (writer->sequenceId == 0) ? 0x01 : 0x00;   // Fake resume logging at start

    // @23  +1   battery: Last recorded battery level in raw units, 0 = unknown
    buffer[23] = 0;

    // @24  +1   sampleRate: Sample rate code, frequency (3200/(1<<(15-(rate & 0x0f)))) Hz, range (+/-g) (16 >> (rate >> 6)).
    buffer[24] = writer->lastRateCode;

    // @25  +1   numAxesBPS: 0x32 (top nibble: number of axes, 3=Axyz, 6=Gxyz/Axyz, 9=Gxyz/Axyz/Mxyz; bottom nibble: packing format - 2 = 3x 16-bit signed, 0 = 3x 10-bit signed + 2-bit exponent)
    buffer[25] = numAxesBPS;

    // @26  +2   timestampOffset: Relative sample index from the start of the buffer where the whole-second timestamp is valid
    buffer[26] = (unsigned char)timestampOffset; buffer[27] = (unsigned char)(timestampOffset >> 8);

    // @28  +2   sampleCount: Number of accelerometer samples (80 or 120 if this sector is full)
    buffer[28] = (unsigned char)writer->sampleCount; buffer[29] = (unsigned char)(writer->sampleCount >> 8);

    // @30  +480 rawSampleData: Raw sample data.  Each sample is either 6x 16-bit signed values (gx, gy, gz, ax, ay, az); 3x 16-bit signed values (x, y, z); or one 32-bit packed value (The bits in bytes [3][2][1][0]: eezzzzzz zzzzyyyy yyyyyyxx xxxxxxxx, e = binary exponent, lsb on right)
    for (int i = 0; i < writer->sampleCount; i++)
    {
      cwa_writer_sample_t *sample = &writer->samples[i];
      short intSample[6];

      // TODO: Support different accelerometer scales and gyro-scale
      int scale = 256;

      // TODO: Support 6-axis, put gyro data first
      intSample[0] = (int)(sample->x * scale);
      intSample[1] = (int)(sample->y * scale);
      intSample[2] = (int)(sample->z * scale);

      // Clamp to the configured g-range
      int clamp = 256 * OM_ACCEL_RANGE_FROM_CONFIG(writer->lastRateCode);
      for (int j = 0; j < channels; j++)
      {
        if (intSample[j] < -clamp) { intSample[j] = -clamp; }
        if (intSample[j] > clamp - 1) { intSample[j] = clamp - 1; }
      }

      if (writer->settings.packed)
      {
        unsigned char *p = buffer + 30 + (i * 4);
        AccelPackData(intSample, p, writer->lastRateCode);
      }
      else 
      {
        unsigned char *p = buffer + 30 + (i * channels * 2);
        for (int j = 0; j < channels; j++)
        {
          p[2 * j] = (unsigned char)intSample[j]; p[2 * j + 1] = (unsigned char)(intSample[j] >> 8); 
        }
      }

    }

    // @510 +2   checksum: Checksum of packet (16-bit word-wise sum of the whole packet should be zero)
    unsigned short check = checksum(buffer, 510);
    buffer[510] = (unsigned char)check; buffer[511] = (unsigned char)(check >> 8);
    
    if (fwrite(buffer, 1, sizeof(buffer), writer->fp) != sizeof(buffer)) { return false; }

    writer->sequenceId++;
  }

  // Start new block
  writer->sampleCount = 0;
  writer->firstSampleTime = 0;
  memset(writer->samples, 0x00, sizeof(cwa_writer_sample_t) * CWA_MAX_SAMPLES);

  return true;
}

bool CwaWriterWriteSample(cwa_writer_t *writer, const cwa_writer_sample_t *sample)
{
  // Check time is contiguous
  double elapsed = sample->t - writer->lastTime;
  // Just look for anything non-continuous. TODO: consider actual sample rate
  if (writer->lastTime != 0 && (elapsed < 0 || elapsed > 5))
  {
    // Flush current block because there is a break in the recording
    CwaWriterWriteBlock(writer);
    writer->sequenceId = 0;
  }
  writer->lastTime = sample->t;

  // Append sample
  writer->samples[writer->sampleCount] = *sample;
  if (writer->sampleCount == 0)
  {
    writer->firstSampleTime = sample->t;
  }
  writer->sampleCount++;

  int maxSamples = writer->settings.packed ? 120 : 80;
  if (writer->sampleCount >= maxSamples)
  {
    // Flush current block because it is full
    if (!CwaWriterWriteBlock(writer)) { return false; }
  }
  return true;
}

void CwaWriterClose(cwa_writer_t *writer)
{
  // Flush current block because we are closing the file
  CwaWriterWriteBlock(writer);
  if (writer->fp != NULL)
  {
    if (writer->fp != stdout)
    {
      fclose(writer->fp);
    }
    writer->fp = NULL;
  }
}
