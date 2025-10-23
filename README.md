# Open Movement File Synthesizer

## About

This repository contains the source code for the *omsynth* tool, which can be used to synthesize Open Movement [`.CWA` files](https://github.com/digitalinteraction/openmovement/blob/master/Docs/ax3/ax3-technical.md#measurement-data).

The *omsynth* tool is intended for use to generate synthetic data for testing purposes.


## Obtaining the tool

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


## Note: Editing .CWA files

[`.CWA` files](https://github.com/digitalinteraction/openmovement/blob/master/Docs/ax3/ax3-technical.md#measurement-data) are designed to be written once by [AX Devices](https://github.com/digitalinteraction/openmovement/wiki/AX3) to be an accurate record of what was recorded -- and are not designed to be edited.  

While it should be possible to export `.CWA` data to `.CSV`, edit the `.CSV` data, then synthesize a new `.CWA` file with the *omsynth* tool, this process is not lossless, and is not recommended (the *omsynth* tool was really only written to create small, synthetic sample files for testing purposes).

If you really must edit a `.CWA` file (e.g. to remove a portion of sensitive data), then the best approach would be to *splice* the original file (see next section).


### Splicing a .CWA file

The simplest way to *splice* a .CWA file (e.g. to remove a portion of sensitive data, or remove an unwanted trailing part of data) is to use `cwa-splice` (see next section) or, alternatively, the manual option described below may also be used.

#### Splice Option 1: cwa-splice

Download and extract the archive [cwa-splice.zip](https://github.com/digitalinteraction/omsynth/raw/master/bin/cwa-splice.zip) and, at the command line, run the `cwa-splice.cmd` script (Windows only), which has the following usage:

> ```
> cwa-splice <input.cwa> [--output spliced.cwa] [--overwrite] [first-block last-block]...
> ```

Where the *first-block* and *last-block* numbers can be determined by pressing <kbd>Shift</kbd>+<kbd>Alt</kbd> while moving the cursor over the preview graph in the *OmGui* software.  The *last-block* can be omitted on the final segment to include the remainder of the input.

#### Splice Option 2: Manually splicing a .CWA file

You can splice your own file with the Unix `dd` tool (available on Windows through [UnxUtils](https://sourceforge.net/projects/unxutils/), extract the archive, the binary is at `/usr/local/wbin/dd.exe`).  Note that:

* The header is in the first 1024 bytes (0-1023 inclusive), and should be preserved.
* Data is encapsulated in chunks of 512 bytes ("sectors"), and must only be spliced between sectors.

For example, the following commands will extract sector 10 to 20 from `cwa-data.cwa`:

```bash
# Extract the file header (2 sectors)
dd bs=512 if=cwa-data.cwa of=cwa-data.cwa-header count=2

# Extract the file data
# e.g. from data offset 10 (start) to 30 (end) inclusive
#      skip=12  (start+2 to move it after the header)
#      count=21 (end-start+1 to be an inclusive range)
dd bs=512 if=cwa-data.cwa of=cwa-data.cwa-data skip=12 count=21

# Concatenate the header and spliced data on a non-Windows computer
cat cwa-data.cwa-header cwa-data.cwa-data cwa-data.spliced.cwa

# Concatenate the header and spliced data on a Windows computer
copy cwa-data.cwa-header /b + cwa-data.cwa-data /b cwa-data.spliced.cwa /b
```

To detemine the *Block Number* to use, you can use press <kbd>Shift</kbd>+<kbd>Alt</kbd> while moving the cursor over the preview graph in the *OmGui* software.  Remember to add `2` to calculate the `skip` value to account for the file header, and to calculate the `count=` value by subtracting the first block number from the last block number, and add `1` to make it a range inclusive of the last block.

If you want all of the remaining data after a specific offset, omit the `count=` argument.


## Note: Outputting .CSV data for specific time ranges

Depending on your requirements:

* If exporting *Raw CSV*:  Using [OmGui](https://github.com/openmovementproject/openmovement/wiki/AX3-GUI), select a downloaded `.CWA` file in the lower *Data Files* panel.  In the middle data preview window, click the `⇼` *Selection* mode in the *Options*.  Left-click and drag to choose a selection window.  Choose *Export* / *Export Raw CSV*, and choose your exported file name.  The *Export raw data* options will now include the chosen *Selected Time Slice*.

* As above, you could [splice](https://github.com/openmovementproject/omsynth/#splicing-a-cwa-file) a `.CWA` file into parts, then [export to .CSV](https://github.com/openmovementproject/openmovement/blob/master/Docs/ax3/ax3-research.md#raw-data) each part as required.

* For more advanced .CSV exporting:  You can load the raw data into one of many programming languages/analysis environments, see: [AX Research: Raw Data](https://github.com/openmovementproject/openmovement/blob/master/Docs/ax3/ax3-research.md#raw-data).
