#!/bin/bash

source tools.sh

# var list
      NAME="ARISC driver for the LinuxCNC"
   CUR_DIR=$(pwd)
   SRC_DIR="linuxcnc/drv"
 TARGET_ID="0"
 ALL_FILES=("api.h" "arisc.c")
   C_FILES=("arisc.c")




# greetings
log ""
log "--- Installing **${NAME}** -------"




# check a folder with sources
if [[ ! -d "${SRC_DIR}" ]]; then
    log "!!ERROR!!: Can't find the **./${SRC_DIR}** folder [**${0}:${LINENO}**]."
    cd "${CUR_DIR}"
    exit 1
fi

cd "${SRC_DIR}"

for file in ${ALL_FILES[*]}; do
    if [[ ! -f "${file}" ]]; then
        log "!!ERROR!!: Can't find the **./${SRC_DIR}/${file}** file [**${0}:${LINENO}**]."
        cd "${CUR_DIR}"
        exit 1
    fi
done




# find a compiler
if [[ $(halcompile --help | grep Usage) ]]; then
    COMPILER="halcompile"
elif [[ $(comp --help | grep Usage) ]]; then
    COMPILER="comp"
else
    log "!!ERROR!!: Can't find a components compiler for the **${TARGET}** [**${0}:${LINENO}**]."
    exit 1
fi




# compiling the driver
log "Compiling the drivers ..."

for file in ${C_FILES[*]}; do
    if [[ ! $(sudo "${COMPILER}" --install "${file}" | grep Linking) ]]; then
        log "!!ERROR!!: Failed to compile the **${file}** file [**${0}:${LINENO}**]."
        exit 1
    fi
done

cd "${CUR_DIR}"

log "--- **${NAME}** ++successfully installed++ -------"
log ""
