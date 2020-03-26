#!/bin/bash

source tools.sh

# var list
      NAME="Machinekit"
  SRC_ARCH=("armhf")
  DST_ARCH=("armv7")
    DST_OS=("jessie" "stretch")
        OS=""
  SRC_REPO="http://deb.machinekit.io/debian"
  DST_FILE="/etc/apt/sources.list.d/machinekit.list"




# greetings
log ""
log "--- Installing **${NAME}** -------"




# check CPU arch
supported=""

for item in ${DST_ARCH[*]}; do
    if [[ $(uname -m | grep "${item}") ]]; then
        supported="1"
        break
    fi
done

if [[ ! $supported ]]; then
    log "Supported CPU types: **${DST_ARCH[*]}**"
    log "!!ERROR!!: Your CPU type (**$(uname -m)**) isn't supported [**${0}:${LINENO}**]."
    exit 1
fi




# check OS type
supported=""

for item in ${DST_OS[*]}; do
    if [[ $(lsb_release -c | grep "${item}") ]]; then
        supported="1"
        OS="$item"
        break
    fi
done

if [[ ! $supported ]]; then
    log "Supported OS types: ${DST_OS[*]}"
    log "!!ERROR!!: Your OS type (**$(lsb_release -c)**) isn't supported [**${0}:${LINENO}**]."
    exit 1
fi




# add repository links
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 43DDF224
sudo rm -f $DST_FILE
sudo touch $DST_FILE
sudo sh -c "echo 'deb $SRC_REPO $OS main' >> $DST_FILE"
sudo apt update




# installing packages
sudo apt install machinekit-rt-preempt -qq

if [[ ! $(machinekit -help | grep Usage) ]]; then
    log "!!ERROR!!: Failed to install **machinekit-rt-preempt** package [**${0}:${LINENO}**]."
    exit 1
fi

sudo apt install machinekit-manual-pages -qq




log "--- **${NAME}** ++successfully installed++ -------"
log ""
