#!/bin/bash

# ./createHeaders.sh <wordlist> <path to voice WAV files>

while read -r line
do
    f=$2/$line
    echo $line
    xxd -i -n vocab_$line -s +$(python3 findDataSection.py $2/$line.wav) $2/$line.wav > $line.h.tmp
    cat $line.h.tmp | sed "s/unsigned/const unsigned/" > $line.h
    rm $line.h.tmp
done < $1

echo

cat *.h | grep len | cut -d " " -f6 | tr ";" " " | awk 'BEGIN{sum=0}{sum+=$1}END{print sum}'
