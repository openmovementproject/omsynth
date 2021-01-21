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
mkdir omsynth.build; curl https://openlab.ncl.ac.uk/gitlab/dan.jackson/omsynth/repository/archive.tar.gz?ref=master | tar xvz -C omsynth.build --strip-components=1 && make -C omsynth.build/src/omsynth && cp omsynth.build/src/omsynth/omsynth .
```

## Operation

From a source file `$DATASET.csv` you can synthesize an Open Movement *CWA* file `$DATASET.cwa` as follows:

```bash
./omsynth $DATASET.csv -out $DATASET.cwa
```

Alternatively, drag the `$DATASET.csv` over the `_omsynth-auto` script ([Windows batch file](bin/_omsynth-auto.cmd?raw=true) or [Bash script]([_omsynth-auto](bin/_omsynth-auto.sh?raw=true))).  This will run the `omsynth` executable with the required parameters, and will generate the output file `$DATASET.synth.cwa`.

## File format

The input file format has an optional header line:

	Time,Accel-X (g), Accel-Y (g), Accel-Z (g)

...and subsequent data lines:

	T,Ax,Ay,Az

Where *T* is a timestamp in the format *YYYY-MM-DD hh:mm:ss.fff*, *Ax*/*Ay*/*Az* are the accelerometer axes in units of *g*.

The output file format is an Open Movement *.CWA* file, the format of which is specified at: [cwa.h](https://github.com/digitalinteraction/openmovement/blob/master/Docs/ax3/cwa.h).
