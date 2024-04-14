#!/bin/bash

# Define the sketch directory and sketch name
SKETCH_DIRECTORY="/home/cg/CG2111A/w8s2pi/Alex"
SKETCH_NAME="Alex.ino"

# Define the FQBN (Fully Qualified Board Name)
FQBN="arduino:avr:mega"

# Define the port your Arduino is connected to
PORT="/dev/ttyACM0"  # This might be /dev/ttyUSB0 or similar; check with `arduino-cli board list`

# Compile the sketch
arduino-cli compile --fqbn $FQBN $SKETCH_DIRECTORY/$SKETCH_NAME --verbose

# Upload the sketch to the board
arduino-cli upload -p $PORT --fqbn $FQBN $SKETCH_DIRECTORY/$SKETCH_NAME --verbose

echo "Upload complete!"