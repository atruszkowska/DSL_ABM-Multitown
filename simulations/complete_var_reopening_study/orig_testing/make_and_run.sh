#!/bin/bash

# Shell script for automated simulations of ABM with a 
# fixed vaccination rate and variable reopening rates 
# Creates directories for each reopening rate, copies all
# the files and directories from templates to the new one,
# runs sub_rate.py to substitute target reopening rate,
# compiles, runs the simulations through MATLAB wrapper
# All this is done in one screen per rate

# This script also opens matlab and adjusts glibc path

# To shut down all screens later
# screen -ls | grep Detached | cut -d. -f1 | awk '{print $1}' | xargs kill

# Run as
# ./make_and_run

# Hardcoded reopeoning rates to consider
declare -a Rrates=(0.00001 0.0001 0.001 0.01 0.02 0.03 0.04 0.05 0.06 0.07 0.08 0.09 0.1)

for i in "${Rrates[@]}";
do
	echo "Processing rate $i"
	
	# Make the directory and copy all the neccessary files
    mkdir "dir_$i"
    cp -r templates/input_data "dir_$i/"
	cp -r templates/output "dir_$i/"
	cp templates/*.* "dir_$i/"
	cd "dir_$i/"

	# Pre-process input
	# Substitute the correct value for reopeoning rate
	mv input_data/infection_parameters.txt input_data/temp.txt

    python3.6 sub_rate.py input_data/temp.txt input_data/infection_parameters.txt "reopening rate" $i

	# Compile
	python3.6 compilation.py  
	
	# Run (starts and exits screen)
	screen -d -m ./run_matlab.sh
	
	cd ..
done
