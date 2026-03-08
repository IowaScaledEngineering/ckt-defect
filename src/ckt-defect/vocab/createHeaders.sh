#!/bin/bash

# ./createHeaders.sh <wordlist> <path to voice WAV files>

while read -r line
do
    f=$2/$line
    echo $line
    xxd -i -n vocab_$line -s +$(python3 findDataSection.py $2/$line.wav) $2/$line.wav > include/$line.h.tmp
    cat include/$line.h.tmp | sed "s/unsigned/const unsigned/" > include/$line.h
    rm include/$line.h.tmp
done < $1

echo

cat include/*.h | grep len | cut -d " " -f6 | tr ";" " " | awk 'BEGIN{sum=0}{sum+=$1}END{print sum}'
