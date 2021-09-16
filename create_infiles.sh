#!/bin/bash

args=("$@")

if [ $# -eq 2 ]; then
        echo "No all arguments supplied"
        exit   
fi            

dir=${args[1]}
if [ -d "$dir" ]; then
    echo "$dir exists.Delete old Directory and create new"
    rm -rf $dir
    mkdir $dir
else
    echo "Create $dir Directory"
    mkdir $dir  
fi 

FILE=${args[0]}
i=0
N=4
counter=1;
numFiles=${args[2]}
counter3=0
declare -a array
while read line; do
    cd $dir
    Country=$(echo $line | cut -d " " -f $N)
    if [ -d "$Country" ]; then

        echo "Waiting....." 
        
    else  
        mkdir $Country
        cd $Country
        array[i]=$Country
        ((i=i+1))
        while [ "$counter" -le "$numFiles" ]
        do
            touch $Country-$counter.txt
            ((counter=counter+1))
        done 
        counter=1
        cd ..
    fi
    ((counter3=counter3+1))
    cd ..
done < $FILE

echo $counter3
count=1
    for tmpCountry in ${array[@]}
    do

    echo $tmpCountry    
        while read line; do
        
        cd $dir
        Country=$(echo $line | cut -d " " -f $N)
        if [ "$Country" = "$tmpCountry" ]; then
        cd $Country
            echo $line
            echo "$line" >> $Country-$count.txt
            if [ "$count" -eq "$numFiles" ]; then
            count=0
            fi
            ((count=count+1)) 
        cd ..    
        fi
        cd ..
        done < $FILE
    echo
    count=1
    done    