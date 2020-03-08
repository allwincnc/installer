#!/bin/bash

# var list
      NAME="ARISC firmware"
   DST_DIR="/boot"
   SRC_DIR="./armbian/arisc_firmware"
 ALL_FILES=("arisc-fw.code" "fixup.cmd" "fixup.scr")




# greetings
echo "--- Installing '${NAME}' -------"




# check a folders
if [[ ! -d "${SRC_DIR}" ]]; then
    echo "Can't find the '${SRC_DIR}' folder."
    exit 1
fi

if [[ ! -d "${DST_DIR}" ]]; then
    echo "Can't find the '${DST_DIR}' folder."
    exit 1
fi




# check/copy files
for file in ${ALL_FILES[*]}; do
    if [[ ! -f "${SRC_DIR}/${file}" ]]; then
        echo "Can't find the '${SRC_DIR}/${file}' file."
        exit 1
    fi

    sudo cp -f "${SRC_DIR}/${file}" "${DST_DIR}/${file}"
    if [[ ! -f "${DST_DIR}/${file}" ]]; then
        echo "Can't create the '${DST_DIR}/${file}' file."
        exit 1
    fi
done




echo "--- The '${NAME}' successfuly installed -------"
