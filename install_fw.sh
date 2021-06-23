#!/bin/bash

source tools.sh

# var list
      NAME="ARISC core firmware"
      CHIP=""
   DST_DIR="/boot/allwincnc"
   SRC_DIR="./armbian/fw"
 ALL_FILES=("arisc-fw.code")




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




# check/create folders
if [[ ! -d "${SRC_DIR}" ]]; then
    log "!!ERROR!!: Can't find the **${SRC_DIR}** folder [**${0}:${LINENO}**]."
    exit 1
fi

if [[ ! -d "${DST_DIR}" ]]; then
    sudo mkdir "${DST_DIR}"
fi




# check/copy files
for file in ${ALL_FILES[*]}; do
    if [[ ! -f "${SRC_DIR}/${file}" ]]; then
        log "!!ERROR!!: Can't find the **${SRC_DIR}/${file}** file [**${0}:${LINENO}**]."
        exit 1
    fi

    sudo cp -f "${SRC_DIR}/${file}" "${DST_DIR}/${file}"
    if [[ ! -f "${DST_DIR}/${file}" ]]; then
        log "!!ERROR!!: Can't create the **${DST_DIR}/${file}** file [**${0}:${LINENO}**]."
        exit 1
    fi
done

log "@@NOTE@@: You must reboot the system to complete the installation"




log "--- **${NAME}** ++successfully installed++ -------"
log ""
