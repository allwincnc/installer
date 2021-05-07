#!/bin/bash

source tools.sh

# var list
      NAME="ARISC driver for the LinuxCNC"
      CHIP=""
   CUR_DIR=$(pwd)
   SRC_DIR="linuxcnc/drv"
 TARGET_ID="0"
 ALL_FILES=("api.h" "arisc.c")
   C_FILES=("arisc.c")




# greetings
log ""
log "--- Installing **${NAME}** -------"




# select a SoC from the arguments list
if [[ $# != 0 ]]; then
    for arg in $*; do
        case $arg in
            "h3") CHIP="h3"; ;;
            "H3") CHIP="h3"; ;;
            "h5") CHIP="h5"; ;;
            "H5") CHIP="h5"; ;;
            "h6") CHIP="h6"; ;;
            "H6") CHIP="h6"; ;;
        esac
    done
fi

# if no SoC selected yet
while [[ "${CHIP}" != "1"  && "${CHIP}" != "2"  && "${CHIP}" != "3"  && \
         "${CHIP}" != "h3" && "${CHIP}" != "H3" && \
         "${CHIP}" != "h5" && "${CHIP}" != "H5" && \
         "${CHIP}" != "h6" && "${CHIP}" != "H6" ]]; do
    log     "Please select your SoC:"
    log     "  1: Allwinner H3"
    log     "  2: Allwinner H5"
    log     "  3: Allwinner H6"
    read -p "SoC: " CHIP
done

# set SoC
case "${CHIP}" in
    "1")  CHIP="h3"; ;;
    "2")  CHIP="h5"; ;;
    "3")  CHIP="h6"; ;;
    "h3") CHIP="h3"; ;;
    "H3") CHIP="h3"; ;;
    "h5") CHIP="h5"; ;;
    "H5") CHIP="h5"; ;;
    "h6") CHIP="h6"; ;;
    "H6") CHIP="h6"; ;;
    *)    CHIP="h3"; ;;
esac

# setup sources folder based on the SoC name
SRC_DIR="${SRC_DIR}/${CHIP}"




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
