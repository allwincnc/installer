#!/bin/bash

source tools.sh

# var list
      NAME="System language"
       LNG=""
  LNG_CODE=""




# greetings
log ""
log "--- Installing **${NAME}** -------"




# select language from the arguments list
if [[ $# != 0 ]]; then
    for arg in $*; do
        case $arg in
            "en") LNG="en"; ;;
            "ru") LNG="ru"; ;;
        esac
    done
fi

# if no language selected yet
while [[ "${LNG}" != "1"  && "${LNG}" != "2" && \
         "${LNG}" != "en" && "${LNG}" != "ru" ]]; do
    log     "Please select the system language:"
    log     "  1: English"
    log     "  2: Russian"
    read -p "Language: " LNG
done

# set language
case "${LNG}" in
    "1") LNG="en"; ;;
    "2") LNG="ru"; ;;
esac

# set language code
case "${LNG}" in
    "en") LNG_CODE="${LNG}_US"; ;;
    "ru") LNG_CODE="${LNG}_RU"; ;;
esac




log "Setting up the '${LNG_CODE}.UTF-8' as system language..."

sudo sed -i -e "s/# ${LNG_CODE}.UTF/${LNG_CODE}.UTF/" /etc/locale.gen

touch _locale_tmp_file_
echo LANG=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LANGUAGE=${LNG_CODE}:${LNG} >> _locale_tmp_file_
echo LC_CTYPE=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LC_NUMERIC=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LC_TIME=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LC_COLLATE=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LC_MONETARY=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LC_MESSAGES=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LC_PAPER=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LC_NAME=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LC_ADDRESS=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LC_TELEPHONE=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LC_MEASUREMENT=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LC_IDENTIFICATION=${LNG_CODE}.UTF-8 >> _locale_tmp_file_
echo LC_ALL=${LNG_CODE}.UTF-8 >> _locale_tmp_file_

sudo mv /etc/default/locale /etc/default/locale.bak.$(date -d now +%s)
sudo mv _locale_tmp_file_ /etc/default/locale
sudo chown root:root /etc/default/locale
sudo dpkg-reconfigure --frontend=noninteractive locales
sudo update-locale LANG=${LNG_CODE}.UTF-8

log "@@NOTE@@: You must relogin or reboot the system to complete the installation"




log "--- **${NAME}** ++successfully installed++ -------"
log ""
