#!/bin/bash

# var list
   NAME="orangecnc"
    ERR="0"
  TASKS=( \
        "cmd:./install_system_tweaks.sh,    do:1,   arg:" \
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
echo ""
echo "--------------------------------------------------------------"
echo "--- Installing '${NAME}' -------"
echo "--------------------------------------------------------"
echo ""




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
echo     ""
echo     "--------------------------------------------------------"
if [[ $ERR == "0" ]]; then
    echo "--- The '${NAME}' successfully installed -------"
else
    echo "--- The '${NAME}' installed with errors -------"
fi
echo     "--------------------------------------------------------------"
echo     ""
