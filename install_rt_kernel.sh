#!/bin/bash

source tools.sh

# var list
      NAME="PREEMPT RT kernel"
   SRC_DIR="./armbian/preempt_rt_kernel"
  SRC_ARCH=("armhf" "arm64")
  DST_ARCH=("armv7" "aarch64")
 ALL_FILES=("linux-image-current-sunxi_20.02.3_armhf.deb" \
            "linux-headers-current-sunxi_20.02.3_armhf.deb" \
            "linux-image-current-sunxi64_20.05.0-trunk_arm64.deb" \
            "linux-headers-current-sunxi64_20.05.0-trunk_arm64.deb" \
            )




# greetings
log ""
log "--- Installing **${NAME}** -------"




# check folders/files
if [[ ! -d "${SRC_DIR}" ]]; then
    log "!!ERROR!!: Can't find the **${SRC_DIR}** folder [**${0}:${LINENO}**]."
    exit 1
fi

for file in ${ALL_FILES[*]}; do
    if [[ ! -f "${SRC_DIR}/${file}" ]]; then
        log "!!ERROR!!: Can't find the **${SRC_DIR}/${file}** file [**${0}:${LINENO}**]."
        exit 1
    fi
done




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




# get packages data
PKG_ARCH=""
arch_cnt=${#DST_ARCH[@]}

for (( a=0; a < $arch_cnt; a++ )); do
    if [[ $(uname -m | grep "${DST_ARCH[$a]}") ]]; then
        PKG_ARCH="${SRC_ARCH[$a]}"
        break
    fi
done

ARCH_PKGS=$(ls $SRC_DIR | grep "$PKG_ARCH.deb")

MAIN_PKG=$SRC_DIR"/"$(echo "$ARCH_PKGS" | grep "image")
HEAD_PKG=$SRC_DIR"/"$(echo "$ARCH_PKGS" | grep "headers")




# installing packages
log "Installing packages ..."

log "Installing **$MAIN_PKG** ..."

if [[ -f $MAIN_PKG ]]; then
    sudo apt install $MAIN_PKG -qq
fi

version=$(dpkg-deb -I "$MAIN_PKG" | grep "Source:" | grep -P -o "[0-9]+\.[0-9]+\.[0-9]+\-\w+\-\w+")

if [[ ! -f "/boot/vmlinuz-${version}" ]]; then
    log "!!ERROR!!: Failed to install **${MAIN_PKG}** package [**${0}:${LINENO}**]."
    exit 1
fi

log "Installing **$HEAD_PKG** ..."

if [[ -f $HEAD_PKG ]]; then
    sudo apt install $HEAD_PKG -qq
fi

version=$(dpkg-deb -I "$HEAD_PKG" | grep "Source:" | grep -P -o "[0-9]+\.[0-9]+\.[0-9]+\-\w+\-\w+")

if [[ ! -d "/usr/src/linux-headers-${version}" ]]; then
    log "WARNING: Failed to install **${HEAD_PKG}** package [**${0}:${LINENO}**]."
fi

log "@@NOTE@@: You must reboot the system to complete the installation"




log "--- **${NAME}** ++successfully installed++ -------"
log ""
