#!/bin/bash

# Set the input file
if [[ -f roms_formatted.txt ]]; then
  rm roms_formatted.txt 
fi

input_file="roms.ini"

# Loop over each line in the input file
IFS=$'\r\n'
while read -r line; do

  # Handle ID
  if [[ $line == *"{"* ]]; then
    id=$(echo $line | tr -d '{}-')
    id_done=$id
  fi

  # Handle Name / SaveType / Preview
  if [[ $line == *"Name="* ]]; then
    name="$(echo $line | cut -d'=' -f2)"
  fi

  if [[ $line == *"Preview="* ]]; then
    preview="$(echo $line | cut -d'=' -f2)"
  fi

  if [[ $line == *"SaveType="* ]]; then
    save_type="$(echo $line | cut -d'=' -f2)"
  fi

  # Print the formatted rom info if all required fields are present
  if [[ ! -z "$name" && ! -z "$preview" ]]; then
    if [[ -z "$save_type" ]]; then
      save_type="NONE"
    fi

    save_type="${save_type/Eeprom4k/EEP4K}"
  save_type="${save_type/Eeprom16k/EEP16K}"
  save_type="${save_type/FlashRam/FLASH}"
    save_type="${save_type/FlashRAM/FLASH}"

    printf 'updateValues.emplace_back("%s", "%s", ESaveType::%s, "%s");\n' "$id_done" "$name" "$save_type" "$preview"
    # Reset the variables to empty for next iteration
    id_done=""
    name=""
    preview=""
    save_type=""
  fi

done < "$input_file" > roms_formatted.txt
