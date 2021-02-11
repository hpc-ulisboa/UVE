#!/bin/bash

if [ "$#" -ne 1 ] || [[ ! $1 =~ -j[1-9] ]] ; then
    echo -e "-j# must be passed with # as the maximum number of jobs"
    exit 1
fi

JOBS=${1:2}
NAME=${PWD##*/}
NEW_UUID=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 5 | head -n 1)
JOBS_FILE=jobs_file_$NEW_UUID.parallel
echo -e "\n Starting Run $NEW_UUID\n\n"
echo $NEW_UUID > last.uuid
echo $JOBS > $JOBS_FILE
IDS=$(parallel --header : "echo {arch}_{pftch}_{nelms}_{nelmsst}_{icount}" :::: Args/arch.txt :::: Args/n_elements.txt :::: Args/prefetcher.txt :::: Args/iter_count.txt :::: Args/n_elements_start.txt )
parallel --bar --progress --results builds/${NEW_UUID}/parallel -P $JOBS_FILE --header : "./thread.sh ${NEW_UUID}_{arch}_{pftch}_{nelms}_{nelmsst}_{icount} {arch} N_ELEMENTS={nelms} ARCH={arch} PREFETCHER={pftch} N_ELEMENTS_START={nelmsst} ITER_COUNT={icount}" :::: Args/arch.txt :::: Args/n_elements.txt :::: Args/prefetcher.txt :::: Args/iter_count.txt :::: Args/n_elements_start.txt


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
echo -e "In annex you can found the latest results from your run!\n The following runs were executed:\n$IDS\nFrom your dearest... elsa" | mail -s "Results $NAME/$NEW_UUID" -a ./$OutFileName joao.mario@tecnico.ulisboa.pt
