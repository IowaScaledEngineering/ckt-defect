#!/bin/bash

# ./createAudio.sh <wordlist> <path to voice WAV files>

while read -r line
do
    f=$2/$line
    echo $line
    xxd -i -n vocab_$line -s +$(python3 findDataSection.py $2/$line.wav) $2/$line.wav > include/$line.h.tmp
    cat include/$line.h.tmp | sed "s/unsigned/const unsigned/" > include/$line.h
    rm include/$line.h.tmp
done < $1
