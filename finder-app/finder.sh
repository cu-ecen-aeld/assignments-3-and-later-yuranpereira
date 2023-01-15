#!/bin/sh

## Error messages
ARGS_ERROR_MSG="Two arguments must be provided."
ARGS_ERROR_NOT_DIR=" is not a directory."

if test -z $1 || test -z $2; then
    echo $ARGS_ERROR_MSG;
    return 1;
fi

# Argument variables
filesdir=$1
searchstr=$2

if !(test -d $filesdir); then
    echo \'$filesdir\' $ARGS_ERROR_NOT_DIR;
    exit 1;
fi

# Get number of files
no_of_files=$(ls -R -p $filesdir |grep -v /| wc -l)
# Get number of lines containing "searchstr"
no_of_lines=$(grep $searchstr $filesdir -r | wc -l)

echo "The number of files are" $no_of_files "and the number of matching lines are" $no_of_lines

