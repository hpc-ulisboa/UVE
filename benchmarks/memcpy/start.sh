#!/bin/bash

if [ "$#" -lt 1 ] || [[ ! $1 =~ -j[0-9] ]] ; then
    echo -e "-j# must be passed with # as the maximum number of jobs"
    exit 1
fi
JOBS=${1:2}
NAME=${PWD##*/}
NEW_UUID=$2$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 5 | head -n 1)
JOBS_FILE=jobs_file_$NEW_UUID.parallel
JOBS_LOG=jobs_log_$NEW_UUID.parallel
echo -e "\n Starting Run $NEW_UUID\n\n"
echo $JOBS > $JOBS_FILE
echo "$NEW_UUID" > last.uuid

IDS=$(parallel --header : "echo {arch}_{pftch}_{nelms}" :::: Args/arch.txt :::: Args/n_elements.txt :::: Args/prefetcher.txt)
parallel --progress --bar --joblog $JOBS_LOG --results builds/${NEW_UUID}/parallel -P $JOBS_FILE --header : "./thread.sh ${NEW_UUID}_{arch}_{pftch}_{nelms} {arch} N_ELEMENTS={nelms} ARCH={arch} PREFETCHER={pftch}" :::: Args/arch.txt :::: Args/n_elements.txt :::: Args/prefetcher.txt


OutFileName="results_${NAME}_$NEW_UUID.csv"
i=0
for id in $IDS; do
    filename=./builds/$NEW_UUID/$id/m5out/data_extracted.csv
    if [[ $i -eq 0 ]] ; then 
        head -1  "$filename" >   "$OutFileName" # Copy header if it is the first file
    fi
    tail -n +2  "$filename" >>  "$OutFileName" # Append from the 2nd line each file
    i=$(( $i + 1 ))                            # Increase the counter
done

echo -e "\n\nFINISHED: build ${NAME} ${NEW_UUID}"
rm $JOBS_FILE
