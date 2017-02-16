#!/bin/sh

echo_t()
{
	echo "`date +"%y%m%d-%T.%6N"` $1"
}
#For printing in EPOCH time format
echo_et()
{
        echo "`date +"%s"` $1"
}
