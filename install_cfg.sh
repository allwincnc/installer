#!/bin/bash

source tools.sh

# var list
             NAME="ARISC configs for the LinuxCNC"
          SRC_DIR="linuxcnc/cfg"
          DST_DIR="${HOME}/linuxcnc/configs"
         ALL_DIRS=("3A_test" "3T_test")
        ALL_FILES=("config.hal" "config.ini" "tool.tbl")
          DSK_TPL="link.desktop"
          DSK_DIR="${HOME}/Desktop"
 DSK_CFG_DIR_LINK="${DSK_DIR}/configs"




# greetings
log ""
log "--- Installing **$NAME** -------"




# check folders
if [[ ! -d "${SRC_DIR}" ]]; then
    log "!!ERROR!!: Can't find the **./${SRC_DIR}** folder [**${0}:${LINENO}**]."
    exit 1
fi

if [[ ! -d "${HOME}/linuxcnc" ]]; then
    mkdir "${HOME}/linuxcnc"
fi
if [[ ! -d "${HOME}/linuxcnc" ]]; then
    log "!!ERROR!!: Can't create the **${HOME}/linuxcnc** folder [**${0}:${LINENO}**]."
    exit 1
fi

if [[ ! -d "${DST_DIR}" ]]; then
    mkdir "${DST_DIR}"
fi
if [[ ! -d "${DST_DIR}" ]]; then
    log "!!ERROR!!: Can't create the **~/${DST_DIR}** folder [**${0}:${LINENO}**]."
    exit 1
fi

if [[ ! -d "${DSK_DIR}" ]]; then
    DSK_DIR="${XDG_DESKTOP_DIR}"
fi




# check/copy/process config folders/files
DSK_TPL_FILE="${SRC_DIR}/${DSK_TPL}"

for config in ${ALL_DIRS[*]}; do
    SRC_CFG_DIR="${SRC_DIR}/${config}"
    DST_CFG_DIR="${DST_DIR}/${config}"

    # check folders
    if [[ ! -d "${SRC_CFG_DIR}" ]]; then
        log "!!ERROR!!: Can't find the **$SRC_CFG_DIR** folder [**${0}:${LINENO}**]."
        exit 1
    fi
    if [[ ! -d "${DST_CFG_DIR}" ]]; then
        mkdir "${DST_CFG_DIR}"
    fi
    if [[ ! -d "${DST_CFG_DIR}" ]]; then
        log "!!ERROR!!: Can't create the **$DST_CFG_DIR** folder [**${0}:${LINENO}**]."
        exit 1
    fi

    # check/copy/process config files
    for file in ${ALL_FILES[*]}; do
        SRC_CFG_FILE="${SRC_CFG_DIR}/${file}"
        DST_CFG_FILE="${DST_CFG_DIR}/${file}"

        if [[ ! -f "${SRC_CFG_FILE}" ]]; then
            log "!!ERROR!!: Can't find the **$SRC_CFG_FILE** file [**${0}:${LINENO}**]."
            exit 1
        fi

        # copy file
        cp -f "${SRC_CFG_FILE}" "${DST_CFG_FILE}"
        if [[ ! -f "${DST_CFG_FILE}" ]]; then
            log "!!ERROR!!: Can't create the **$DST_CFG_FILE** file [**${0}:${LINENO}**]."
            exit 1
        fi

        # process file
        sed -i -e "s/__TARGET__/linuxcnc/"  "${DST_CFG_FILE}"
        sed -i -e "s/__CONFIG__/${config}/" "${DST_CFG_FILE}"
        sed -i -e "s/__USER__/$(whoami)/"   "${DST_CFG_FILE}"
    done

    # make a desktop link for this config
    if [[ "${DSK_DIR}" && -f "${DSK_TPL_FILE}" ]]; then
        DSK_LINK_FILE="${DSK_DIR}/${config}.desktop"

        # copy link template
        cp -f "${DSK_TPL_FILE}" "${DSK_LINK_FILE}"
        if [[ ! -f "${DSK_LINK_FILE}" ]]; then
            log "!!ERROR!!: Can't create the **$DSK_LINK_FILE** file [**${0}:${LINENO}**]."
            exit 1
        fi
        chmod +x "${DSK_LINK_FILE}"

        # process link file
        sed -i -e "s/__TARGET__/linuxcnc/"  "${DSK_LINK_FILE}"
        sed -i -e "s/__CONFIG__/${config}/" "${DSK_LINK_FILE}"
        sed -i -e "s/__USER__/$(whoami)/"   "${DSK_LINK_FILE}"
    fi
done




# create a desktop link to the configs folder
if [[ ! -L "${DSK_CFG_DIR_LINK}" ]]; then
    ln -s -f "${DST_DIR}" "${DSK_CFG_DIR_LINK}"
fi

if [[ ! -L "${DSK_CFG_DIR_LINK}" ]]; then
    log "!!ERROR!!: Can't create the **$DSK_CFG_DIR_LINK** link [**${0}:${LINENO}**]."
fi




# success
log "--- **$NAME** ++successfully installed++ -------"
log ""
