#!/bin/bash

# ./createHeaders.sh <wordlist>

cp vocab-template.cpp ../vocab.cpp
while read -r line
do
    echo \#include \"vocab/include/$line.h\" >> ../vocab.cpp
done < $1

echo >> ../vocab.cpp
echo "void loadInternalVocab(void)" >> ../vocab.cpp
echo { >> ../vocab.cpp

while read -r line
do
    echo -e "\tvocab.push_back(new MemSound(\"$line\", vocab_${line}, vocab_${line}_len, 16000));" >> ../vocab.cpp
done < $1

echo } >> ../vocab.cpp

echo

cat include/*.h | grep len | cut -d " " -f6 | tr ";" " " | awk 'BEGIN{sum=0}{sum+=$1}END{print sum}'
