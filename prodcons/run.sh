#!/bin/bash

set -o errexit -o nounset

nit="10000"

nntimes="5"

ncores=`cat /proc/cpuinfo | grep processor | wc -l`
echo "ncores $ncores"

lastcore=$(($ncores - 1))
echo "lastcore $lastcore"

for nsize in 100 250 500 750 1000 1500 2000 2500 3000 4000 5000 6000 7000 8000 9000 10000 11000 12000 13000 14000 15000
do
	folder="results/stats-nsize.$nsize-nit.$nit"
	mkdir -p $folder

	for core in `seq 1 $lastcore`
	do
		echo "core $core"
		export GOMP_CPU_AFFINITY=0,$core

		for i in `seq 0 $nntimes`
		do
			echo "core $core"
			fname="$folder/prodcons-nsize.$nsize-nit.$nit-core.0.$core-$i.txt"
			./prodcons-omp $nsize $nit 1 1 |& tee $fname
		done
	done
done
