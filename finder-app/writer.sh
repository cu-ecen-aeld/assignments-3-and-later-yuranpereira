#!/bin/sh

# Error messages
ARGS_ERROR_MSG="Two arguments must be provided."
FILE_CREAT_ERR="could not create file "

if test -z $1 || test -z $2; then
    echo $ARGS_ERROR_MSG;
    return 1;
fi

# Argument variables
writefile=$1
writestr=$2

mkdir -p $writefile
rmdir $writefile
echo $writestr > $writefile

if !(test -e $writefile); then
    echo "File could not be created"
    return 1;
fi

