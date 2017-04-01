#!/bin/bash

tmap="$1"
types="$2"

ncores=`cat /proc/cpuinfo | grep processor | wc -l`
echo "ncores $ncores"

j=0

for i in `seq 1 1 227`
do
	bestpus[$j]=$i
	j=$(($j + 1))
done

bestpus[$j]=0

base=100000000

if [ "$types" == "1" ]; then
	ntypes=1
	cmd="$base"
elif [ "$types" == "2" ]; then
	ntypes=2
	half=$(($base / 2))
	cmd="$base $half"
elif [ "$types" == "4.1" ]; then
	ntypes=4
	quarter=$(($base / 4))
	th=$(($quarter * 3))
	half=$(($quarter * 2))
	cmd="$base $th $half $quarter"
elif [ "$types" == "4.2" ]; then
	ntypes=4
	quarter=$(($base / 4))
	th=$(($quarter * 3))
	half=$(($quarter * 2))
	cmd="$base $half $half $quarter"
elif [ "$types" == "4.3" ]; then
	ntypes=4
	cmd="$base 0 0 0"
else
	echo "2nd param can be 1, 2, 4.1, 4.2"
	exit
fi

if [ "$tmap" == "good" ]; then
	for i in `seq 0 1 227`
	do
		map[$i]=${bestpus[$i]}
	done
elif [ "$tmap" == "bad" ]; then
	tt=$(($ntypes - 1))
	
	j=0
	for t in `seq 0 1 $tt`
	do
		for i in `seq $t $ntypes 227`
		do
			map[$i]=${bestpus[$j]}
			j=$(($j + 1))
		done
	done
else
	echo "1st param can be bad, good"
	exit
fi

mapstr=""
for i in `seq 0 1 227`
do
	mapstr="$mapstr""${map[$i]}"
	if [ "$i" != "227" ]; then
		mapstr="$mapstr,"
	fi
done

export KMP_AFFINITY=proclist=[$mapstr],explicit

echo "KMP_AFFINITY=$KMP_AFFINITY"
echo "./pi $cmd"
./pi $cmd

