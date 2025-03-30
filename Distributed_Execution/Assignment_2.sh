#!/bin/bash

# Activate OMNeT++ environment
OMNETPP_PATH="/home/sumeet/Downloads/omnetpp-6.1"
PROJECT_PATH="/home/sumeet/Downloads/Computer_Networks/Distributed_Execution"

source "$OMNETPP_PATH/setenv"

# Navigate to the project directory
cd "$PROJECT_PATH" || { echo "Error: Project directory not found!"; exit 1; }

python3 ned_file_generator.py

# Generate Makefile with correct target placement
opp_makemake -f --deep -I src -n src -O out

# Clean previous builds and compile
make clean
make -j$(nproc)

# Locate the compiled binary
EXEC_PATH=$(find out/ -type f -executable -name "Distributed_Execution" | head -n 1)

if [ -z "$EXEC_PATH" ]; then
    echo "Error: Compiled binary 'Distributed_Execution' not found!"
    exit 1
fi

# Run the simulation
"$EXEC_PATH" -u Qtenv -f omnetpp.ini