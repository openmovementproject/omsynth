# Open Movement File Synthesizer

## Obtaining the tool

This repository contains the source code for the *omsynth* tool, which can be used to synthesize Open Movement *.CWA* files.

A pre-built binary for Windows is available from (extract the contents of the *.ZIP* file to a directory):

* [omsynth.zip](bin/omsynth.zip?raw=true)

<!--
* [OpenMovement GitHub](https://github.com/digitalinteraction/openmovement/blob/master/Downloads/AX3/omsynth.zip?raw=true).
-->

On Linux, Mac (*XCode* required) or Windows WSL, you can use this single line command to build an `omsynth` binary in the current directory:

```bash
mkdir omsynth.build && curl -L https://github.com/digitalinteraction/omsynth/archive/master.zip -o omsynth.build/master.zip && unzip omsynth.build/master.zip -d omsynth.build && make -C omsynth.build/omsynth-master/src/omsynth && cp omsynth.build/omsynth-master/src/omsynth/omsynth .
```

## Operation

From a source file `$DATASET.csv` you can synthesize an Open Movement *CWA* file `$DATASET.cwa` as follows:

```bash
./omsynth $DATASET.csv -out $DATASET.cwa
```

Alternatively, drag the `$DATASET.csv` over the `_omsynth-auto` script ([Windows batch file](bin/_omsynth-auto.cmd?raw=true) or [Bash script](bin/_omsynth-auto.sh?raw=true)).  This will run the `omsynth` executable with the required parameters, and will generate the output file `$DATASET.synth.cwa`.

Options:

  * `-packed` if you would like a "packed" CWA file (the default is `-unpacked`).
  * `-scale 1` can be used to change the input scaling factor (e.g. for raw AX3 units, "1/256")
  * `-range 8` can be changed to a different sensor range (2/4/8/16 _g_).
  * `-gyro -1` creates a file produced from a device with no gyroscope (e.g. AX3), while `-gyro 0` creates a file with a disabled gyroscope, or set to a specific gyroscope range (125/250/500/1000/2000 dps).

## File format

The input file format has an optional header line of either:

	Time,Accel-X (g), Accel-Y (g), Accel-Z (g)
	
...or...

	Time,Accel-X (g), Accel-Y (g), Accel-Z (g), Gyro-X (d/s), Gyro-Y (d/s), Gyro-Z (d/s)

...and subsequent data lines of either:

	T,Ax,Ay,Az
	
...or...

	T,Ax,Ay,Az,Gx,Gy,Gz

Where *T* is a timestamp in the format *YYYY-MM-DD hh:mm:ss.fff*, *Ax*/*Ay*/*Az* are the accelerometer axes in units of *g*, and *Gx*/*Gy*/*Gz* are the gyroscope axes in units of degrees/second.

The output file format is an Open Movement *.CWA* file, the format of which is specified at: [cwa.h](https://github.com/digitalinteraction/openmovement/blob/master/Docs/ax3/cwa.h).  
