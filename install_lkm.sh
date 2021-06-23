#!/bin/bash

source tools.sh

# var list
      NAME="ARISC kernel module"
   CUR_DIR=$(pwd)
   DST_DIR="/boot/allwincnc"
 DST_DIR_E="\\/boot\\/allwincnc"
   SRC_DIR="./armbian/lkm"
 ALL_FILES=("Makefile" "arisc_admin.c" "arisc_lkm_installer.sh" "arisc_fw_loader.sh")




# greetings
log ""
log "--- Installing **${NAME}** -------"




# check/create folders
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




# start LKM installer on reboot
if [[ ! -f "/etc/rc.local" ]]; then
    log "!!ERROR!!: Can't find the **/etc/rc.local** file [**${0}:${LINENO}**]."
    exit 1
fi

sudo sed -i -e "s/^exit 0/${DST_DIR}\/arisc_lkm_installer\.sh\nexit 0/" "/etc/rc.local"




log "--- **${NAME}** ++successfully installed++ -------"
log ""
