#!/bin/bash

# var list
      NAME="System tweaks"
    F_FILE="/etc/default/cpufrequtils"
     F_APP="cpufreq-set"
 F_APP_CHK="cpufrequtils"
 F_APP_CMD="${F_APP} -g performance -r"




# greetings
echo "--- Installing '${NAME}' -------"




# set max CPU frequency
if [[ ! $($F_APP --help | grep $F_APP_CHK) ]]; then
    echo "ERROR: Can't find the '${F_APP}' program (${0}:${LINENO})."
    exit 1
fi

sudo $F_APP_CMD

if [[ ! -f $F_FILE ]]; then
    echo "ERROR: Can't find the '${F_FILE}' file (${0}:${LINENO})."
    exit 1
fi

sudo sed -i -e "s/.*GOVERNOR=.*/GOVERNOR=performance/" $F_FILE




echo "--- The '${NAME}' successfully installed -------"
