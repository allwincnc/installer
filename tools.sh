#!/bin/bash

# var list
 logFILE="install.log"
   cNAME="\\\e[38;5;209m"
     cOK="\\\e[38;5;82m"
   cNOTE="\\\e[38;5;80m"
    cERR="\\\e[38;5;198m"
     cNO="\\\e[0m"




# touch the log file
touch $logFILE
echo "" >> $logFILE
echo "DATE: $(date)" >> $logFILE

# logging tool
function log() {
    local out="$1"
    if [[ $(echo "$out" | grep -P "(\*\*|\@\@|\!\!|\+\+)") ]]; then
        local out=$(echo "$out" | sed -e "s/\*\*\([^\*]*\)\*\*/${cNAME}\1${cNO}/g")
        local out=$(echo "$out" | sed -e "s/++\([^+]*\)++/${cOK}\1${cNO}/g")
        local out=$(echo "$out" | sed -e "s/\@\@\([^\@]*\)\@\@/${cNOTE}\1${cNO}/g")
        local out=$(echo "$out" | sed -e "s/\!\!\([^\!]*\)\!\!/${cERR}\1${cNO}/g")
    fi
    echo -e "$out"
    echo "$1" >> $logFILE 
}
