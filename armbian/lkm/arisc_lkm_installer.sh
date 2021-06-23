#!/bin/bash

# var list
      NAME="ARISC kernel module"
   CUR_DIR=$(pwd)
   SRC_DIR="/boot/allwincnc/"
 SRC_DIR_E="\\/boot\\/allwincnc/"
 ALL_FILES=("Makefile" "arisc_admin.c")




# go into our folder
cd "${SRC_DIR}"
source tools.sh




# greetings
log ""
log "--- Installing **${NAME}** -------"




# check files
for file in ${ALL_FILES[*]}; do
    if [[ ! -f "${file}" ]]; then
        log "!!ERROR!!: Can't find the **${file}** file [**${0}:${LINENO}**]."
        exit 1
    fi
done




# build the module
sudo make clean

if [[ -f "/dev/arisc" ]]; then
    sudo make remove
fi

sudo make all

if [[ ! -f "arisc_admin.ko" ]]; then
    log "!!ERROR!!: Failed to build the kernel module [**${0}:${LINENO}**]."
    exit 1
fi




# install the module
sudo make install

if [[ ! -f "/lib/modules/$(uname -r)/kernel/drivers/arisc/arisc_admin.ko" || \
      ! -f "/etc/modules-load.d/arisc.conf" ]]; then
    log "!!ERROR!!: Failed to install the kernel module [**${0}:${LINENO}**]."
    exit 1
fi

sudo make clean




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




# start ARISC firmware loader on every boot
if [[ ! -f "/etc/rc.local" ]]; then
    log "!!ERROR!!: Can't find the **/etc/rc.local** file [**${0}:${LINENO}**]."
    exit 1
fi

if [[ ! $(cat /etc/rc.local | grep arisc_lkm_installer) && \
      ! $(cat /etc/rc.local | grep arisc_fw_loader) ]]; then
    sudo sed -i -e "s/^exit 0/${SRC_DIR_E}\/arisc_fw_loader\.sh\nexit 0/" "/etc/rc.local"
elif [[ ! $(cat /etc/rc.local | grep arisc_fw_loader) ]]; then
    sudo sed -i -e "s/arisc_lkm_installer/arisc_fw_loader/" "/etc/rc.local"
fi

if [[ ! $(cat /etc/rc.local | grep arisc_fw_loader) ]]; then
    log "!!ERROR!!: Can't change the **/etc/rc.local** file [**${0}:${LINENO}**]."
    exit 1
fi




# go back
cd "${CUR_DIR}"




log "--- **${NAME}** ++successfully installed++ -------"
log ""

