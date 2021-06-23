#!/bin/bash

source tools.sh

# var list
      NAME="ARISC kernel module"
   CUR_DIR=$(pwd)
   DST_DIR="/boot/allwincnc/arisc_lkm_tmp"
   SRC_DIR="/boot/allwincnc/"
 ALL_FILES=("Makefile" "arisc_admin.c")




# greetings
log ""
log "--- Installing **${NAME}** -------"




# create TMP folder
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




# build the module
cd "${DST_DIR}"
sudo make remove
sudo make -C "${DST_DIR}" all
cd "${CUR_DIR}"

if [[ ! -f "${DST_DIR}/arisc_admin.ko" ]]; then
    log "!!ERROR!!: Failed to build the kernel module [**${0}:${LINENO}**]."
    exit 1
fi




# install the module
cd "${DST_DIR}"
sudo make -C "${DST_DIR}" install
cd "${CUR_DIR}"

if [[ ! -f "/lib/modules/$(uname -r)/kernel/drivers/arisc/arisc_admin.ko" || \
      ! -f "/etc/modules-load.d/arisc.conf" ]]; then
    log "!!ERROR!!: Failed to install the kernel module [**${0}:${LINENO}**]."
    exit 1
fi




# check the module
if [[ ! $(cat /proc/devices | grep arisc) ]]; then
    log "!!ERROR!!: Can't find the kernel module device [**${0}:${LINENO}**]."
    exit 1
fi

if [[ ! $(ls /sys/class | grep arisc) ]]; then
    log "!!ERROR!!: Can't find the kernel module class [**${0}:${LINENO}**]."
    exit 1
fi

if [[ ! $(ls /dev | grep arisc) ]]; then
    log "!!ERROR!!: Can't find the kernel module device file [**${0}:${LINENO}**]."
    exit 1
fi




# remove TMP folder
if [[ -d "${DST_DIR}" ]]; then
    sudo rm -fr "${DST_DIR}"
fi




# start ARISC firmware loader on every boot
if [[ ! -f "/etc/rc.local" ]]; then
    log "!!ERROR!!: Can't find the **/etc/rc.local** file [**${0}:${LINENO}**]."
    exit 1
fi

sudo sed -i -e "s/arisc_lkm_installer/arisc_fw_loader/" "/etc/rc.local"

if [[ ! $(cat /etc/rc.local | grep arisc_fw_loader) ]]; then
    log "!!ERROR!!: Can't change the **/etc/rc.local** file [**${0}:${LINENO}**]."
    exit 1
fi




log "--- **${NAME}** ++successfully installed++ -------"
log ""

