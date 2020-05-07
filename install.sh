#!/bin/bash

source tools.sh

# var list
   NAME="orangecnc"
    ERR="0"
  TASKS=( \
        "cmd:./install_system_tweaks.sh,    do:1,   arg:" \
        "cmd:./install_system_lang.sh,      do:1,   arg:" \
        "cmd:./install_rt_kernel.sh,        do:1,   arg:" \
        "cmd:./install_linuxcnc.sh,         do:1,   arg:2.7 en" \
        "cmd:./install_machinekit.sh,       do:0,   arg:" \
        "cmd:./install_gpio_driver.sh,      do:1,   arg:linuxcnc" \
        "cmd:./install_gpio_configs.sh,     do:1,   arg:linuxcnc" \
        "cmd:./install_arisc_driver.sh,     do:1,   arg:linuxcnc" \
        "cmd:./install_arisc_configs.sh,    do:1,   arg:linuxcnc" \
        "cmd:./install_arisc_firmware.sh,   do:1,   arg:" \
        )




# greetings
log ""
log "--------------------------------------------------------------"
log "--- Installing **${NAME}** -------"
log "--------------------------------------------------------"
log ""




# installation
for task in "${TASKS[@]}"; do
    do=$(echo "$task" | sed -e "s/.*do:\([^,]*\).*/\1/")
    if [[ $do == "1" || $do == "y" || $do == "Y" ]]; then
        cmd=$(echo "$task" | sed -e "s/.*cmd:\([^,]*\).*/\1/")
        arg=$(echo "$task" | sed -e "s/.*arg:\([^,]*\).*/\1/")
        "$cmd" "$arg"
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
