#!/bin/bash

PHP=php
SCRIPT=$(readlink -e './Process.php')
INFILE=$(readlink -e $1)
CONF_FILE=$(readlink -e './config/default.json')

INCLUDES=$(php -r 'echo get_include_path();')
INCLUDES=$INCLUDES:/opt/datapath/src/PHP/php:/opt/datapath/src/CodeGenerator/php:/opt/datapath/libs

[ -e './Generated' ] || mkdir './Generated'
cd ./Generated

$PHP -d include_path="$INCLUDES" $SCRIPT $INFILE $CONF_FILE
