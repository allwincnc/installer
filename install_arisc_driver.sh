#!/bin/bash

# var list
      NAME="ARISC drivers"
  BASE_URL="https://cnc32.ru/orangecnc"
   CUR_DIR=$(pwd)
   TMP_DIR="tmp"
 TARGET_ID=0
 ALL_FILES=("allwinner_CPU.h" "arisc.gpio.h" "arisc.gpio.c" "arisc.stepgen.h" \
            "arisc.stepgen.c" "gpio_api.h"   "msg_api.h"    "stepgen_api.h")
   C_FILES=("arisc.gpio.c" "arisc.stepgen.c")




# greetings
echo "--- Installing '"$NAME"' -------"




# select the target from the arguments list
if [ $# != 0 ]; then
  for arg in $*; do
    case $arg in
      machinekit) TARGET_ID=1; ;;
      linuxcnc)   TARGET_ID=2; ;;
    esac
  done
fi

# if no target selected yet
while [ $TARGET_ID != 1 ] && [ $TARGET_ID != 2 ]; do
  echo    "Please select the target:"
  echo    "  1: for Machinekit"
  echo    "  2: for LinuxCNC"
  read -p "Target: " TARGET_ID
done

# set target name
case $TARGET_ID in
  1) TARGET="machinekit"; ;;
  2) TARGET="linuxcnc"; ;;
  *) TARGET="machinekit"; ;;
esac




# find a compiler
if [ $(halcompile --help | grep Usage) ]; then
  COMPILER="halcompile"
elif [ $(comp --help | grep Usage) ]; then
  COMPILER="comp"
else
  echo "Can't find a "$TARGET" components compiler."
  echo "Are "$TARGET" with it's DEV package are installed?"
  exit 1
fi




# temporary folder
if [ ! -d $TMP_DIR ]; then
  mkdir $TMP_DIR >> /dev/null
fi

if [ ! -d $TMP_DIR ]; then
  echo "Can't create a temporary folder './"$TMP_DIR"'."
  echo "Do you have an access rights for the current folder?"
  cd $CUR_DIR
  exit 1
fi

cd $TMP_DIR




# downloading files
DWN_URL=$BASE_URL"/"$TARGET"/drivers/arisc"

echo "Downloading sources from"
echo "  '"$DWN_URL/"' ..."

for file in ${ALL_FILES[*]}; do
  rm -f $file
  wget -q $file $DWN_URL"/"$file 
  if [ ! -f $file ]; then
    echo "Failed to download the file '$file'."
    echo "Check the link manually:"
    echo "  '"$DWN_URL"/"$file"'"
    echo "Check your Internet connection."
    echo "And try to start this script again."
    cd $CUR_DIR
    exit 1
  fi
done




# compiling the driver
echo "Compiling the drivers ..."

for file in ${C_FILES[*]}; do
  if [[ ! $(sudo $COMPILER --install $file | grep Linking) ]]; then
    echo "Failed to compile the '"$file"' file."
    echo "Fix these errors and try again."
    exit 1
  fi
done

echo "--- The '"$NAME"' successfuly installed -------"
echo




# back to the previous folder
cd $CUR_DIR
