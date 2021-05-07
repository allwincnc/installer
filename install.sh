#!/bin/bash

source tools.sh

# var list
   NAME="allwincnc"
    ERR="0"
   CHIP=""
  TASKS=( \
        "cmd:./install_sys_tweaks.sh,   do:1,  arg:" \
        "cmd:./install_sys_lang.sh,     do:1,  arg:" \
        "cmd:./install_kernel.sh,       do:1,  arg:" \
        "cmd:./install_linuxcnc.sh,     do:1,  arg:" \
        "cmd:./install_drv.sh,          do:1,  arg:" \
        "cmd:./install_cfg.sh,          do:1,  arg:" \
        "cmd:./install_fw.sh,           do:1,  arg:" \
        )




# greetings
log ""
log "--------------------------------------------------------------"
log "--- Installing **${NAME}** -------"
log "--------------------------------------------------------"
log ""




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




# system update
sudo apt update
sudo apt upgrade




# installation
for task in "${TASKS[@]}"; do
    do=$(echo "$task" | sed -e "s/.*do:\([^,]*\).*/\1/")
    if [[ $do == "1" || $do == "y" || $do == "Y" ]]; then
        cmd=$(echo "$task" | sed -e "s/.*cmd:\([^,]*\).*/\1/")
        arg=$(echo "$task" | sed -e "s/.*arg:\([^,]*\).*/\1/")
        "$cmd" "${CHIP}" "$arg"
        if [[ $? != "0" ]]; then ERR="1"; fi
    fi
done




# results
log     ""
log     "--------------------------------------------------------"
if [[ $ERR == "0" ]]; then
    log "--- **${NAME}** ++successfully installed++ -------"
    log "--- reboot the device to complete the installation -------"
else
    log "--- **${NAME}** installed with !!errors!! -------"
    log "--- see the **$logFILE** file for details -------"
fi
log     "--------------------------------------------------------------"
log     ""
