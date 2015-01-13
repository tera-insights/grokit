#!/bin/bash

# Copyright 2013 Tera Insights, LLC. All Rights Reserved.


PHP_INC=$(php -r 'echo get_include_path();')
PHP_INC=$PHP_INC:$GROKIT_PHP_PATH

SCRIPT=$(readlink -e ./Process.php)
GEN_DIR=$(readlink -f $1)
JSON=$(readlink -e $2)

GROKIT_ERR_FILE=$(readlink -f ./grokit_error.json)
ERR_FILE=$GEN_DIR/grokit_php_error.json

[ -e $ERR_FILE ] && rm $ERR_FILE

CONFIG_FILE=$(readlink -e "config/${GROKIT_CODEGEN_CONFIG}.json")
if [ $? != 0 ]; then
    echo "Unable to find configuration file for code generation profile ${GROKIT_CODGEN_CONFIG}. Aborting."
    errText=$(cat <<EOT
{
    "__type__"  : "error",
    "message"   : "Unable to find configuration file for code generation profile ${GROKIT_CODEGEN_CONFIG}"
}
EOT
)

    echo $errText > $GEN_DIR/grokit_php_error.json
    exit 1
fi

numParallelJobs=32

[ -e $GEN_DIR ] || mkdir $GEN_DIR
cd $GEN_DIR

# Remove the old Generated.so
[ -e ./Generated.so ] && rm ./Generated.so

php --define include_path=$PHP_INC $SCRIPT $JSON $CONFIG_FILE
if [ $? != 0 ]; then
    echo 'Failed to generate C++ code. Aborting'
    if [ ! -e $ERR_FILE ]; then
        errText='{
            "__type__" : "error",
            "message"  : "An unknown error occured while attempting to generate the C++ code. Please check the console."
        }'

        echo $errText > $ERR_FILE
    fi

    cp $ERR_FILE $GROKIT_ERR_FILE
    exit 1
fi
