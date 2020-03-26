#!/bin/bash

source tools.sh

# var list
      NAME="ARISC firmware"
   DST_DIR="/boot"
   SRC_DIR="./armbian/arisc_firmware"
 ALL_FILES=("arisc-fw.code" "fixup.cmd" "fixup.scr")




# greetings
log ""
log "--- Installing **${NAME}** -------"




# check a folders
if [[ ! -d "${SRC_DIR}" ]]; then
    log "!!ERROR!!: Can't find the **${SRC_DIR}** folder [**${0}:${LINENO}**]."
    exit 1
fi

if [[ ! -d "${DST_DIR}" ]]; then
    log "!!ERROR!!: Can't find the **${DST_DIR}** folder [**${0}:${LINENO}**]."
    exit 1
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
