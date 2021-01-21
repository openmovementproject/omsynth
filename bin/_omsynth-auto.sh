#!/bin/bash
# by Dan Jackson
for filename in "$@"
do
    if [[ ${filename: -4} == ".csv" ]]
	then
		echo Processing: "$filename"
		./omsynth "$filename" -out "$(basename "$filename" .csv).synth.cwa"
	else
		echo ERROR: Input file does not have expected type: "$filename"
	fi	
done
