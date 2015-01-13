#!/bin/bash
#
#  Copyright 2013 Christopher Dudley
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

PROFILE='default'
if (( $# > 0 )); then
    echo "Setting profile to $1"
    PROFILE=$1
fi


SCRIPT_FILE=${ROOT}${PREFIX}/lib/grokit/${PROFILE}.sh

# Remove $ROOT (if defined) from the install path
MY_DIR=$(readlink -f ..)
INSTALL_DIR=${MY_DIR#$ROOT}

# Remove any temporary files
[ -e temp.sh ] && rm temp.sh

echo "export GROKIT_INSTALL_PATH=${INSTALL_DIR}" >> temp.sh
echo 'export GROKIT_LOCK_FILE=$GROKIT_INSTALL_PATH/lock' >> temp.sh
echo 'export GROKIT_EXEC_PATH=$GROKIT_INSTALL_PATH/src/Tool_DataPath/executable' >> temp.sh
echo 'export GROKIT_HEADER_PATH=$GROKIT_INSTALL_PATH/src/Headersdp' >> temp.sh
echo 'export GROKIT_LIBRARY_PATH=$GROKIT_INSTALL_PATH/Libs' >> temp.sh
echo 'export GROKIT_PKGCONFIG_PATH=$GROKIT_INSTALL_PATH/pkgconfig' >> temp.sh
echo 'export GROKIT_PHP_PATH=$GROKIT_INSTALL_PATH/src/PHP/php:$GROKIT_INSTALL_PATH/src/CodeGenerator/php' >> temp.sh
echo 'export GROKIT_INSTALLED_LIBRARY_PATH=$GROKIT_INSTALL_PATH/installed_libs' >> temp.sh

install temp.sh $SCRIPT_FILE

rm temp.sh
