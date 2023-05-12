#!/bin/bash

# This extracts the data from 
# Set the input file
input_file="roms.ini"

# Initialize variables
name=""
preview=""
save_type=""
id=""
temp_file=$(mktemp)

# Loop over each line in the input file

while read -r line; do
  # Disregard these, not required
  if [[ $line == CleanSceneEnabled=* || $line == CleanSceneEnable=* || $line == DoubleDisplayEnabled=* || 
        $line ==  cleanSceneEnabled=* || $line == Comment=*  || $line == SkipPifIRQ=*  || $line == *=Unused ||
        $line == //* || $line == {} ]]; then
    # Do nothing for the ignored lines
    continue
  elif [[ $line == Name=* ]]; then
    # Extract the name
    name=${line#Name=}
  elif [[ $line == Preview=* ]]; then
    # Extract the preview
    preview=${line#Preview=}
  elif [[ $line == SaveType=* ]]; then
    # Extract the save type
    save_type=${line#SaveType=}
     save_type="${save_type/ESaveType::Eeprom4k/ESaveType::EEK}"
      save_type="${save_type/eeprom16k/EEP16K}"
  elif [[ $line != "" ]]; then
    # Extract the ID
    id="${line#{}"
    id="${id%-*}"

save_type="${save_type/Eeprom4k/EEP4K}"
save_type="${save_type/Eeprom16k/EEP16K}"
save_type="${save_type/FlashRam/FLASH}"
save_type="${save_type/""/NONE}"
    # Print the formatted output
    echo "{\"$id\", \"$name\", ESaveType::$save_type, \"$preview\"}," 


  fi
done < "$input_file" | sort -k2 #> roms_formatted.txt