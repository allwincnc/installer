#!/bin/bash

source tools.sh

# var list
      NAME="ARISC firmware"
   CUR_DIR=$(pwd)
 LKMD_NAME="/dev/arisc"
 ALL_FILES=("/boot/arisc-fw.code")




# greetings
log ""
log "--- Starting **${NAME}** -------"




# check files
for file in ${ALL_FILES[*]}; do
    if [[ ! -f "${file}" ]]; then
        log "!!ERROR!!: Can't find the **${file}** file [**${0}:${LINENO}**]."
        exit 1
    fi
done




# run the ARISC core with desired firmware
if [[ ! -f "${LKMD_NAME}" ]]; then
    log "!!ERROR!!: Can't find the **${LKMD_NAME}** system device [**${0}:${LINENO}**]."
    exit 1
fi

echo "stop erase /boot/arisc-fw.code upload start status" > "${LKMD_NAME}"
sleep 1

if [[ $(cat ${LKMD_NAME} | grep error) ]]; then
    log "!!ERROR!!: ARISC core's firmware upload and run failed [**${0}:${LINENO}**]."
    exit 1
fi




log "--- **${NAME}** ++successfully running++ -------"
log ""
