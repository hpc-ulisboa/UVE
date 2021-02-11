#!/bin/bash
set -e

trap 'last_command=$current_command; current_command=$BASH_COMMAND' DEBUG
trap 'rm $JOBS_FILE; echo "\"${last_command}\" command filed with exit code $?".' EXIT

if [ "$#" -ne 1 ] || [[ ! $1 =~ -j[1-9] ]] ; then
    echo -e "-j# must be passed with # as the maximum number of jobs"
    exit 1
fi

if [ -z "$GEM5_RV" ] ; then
    echo -e "\n>>>>>>>>>>>>>>Please export GEM5 locations first!\n\n"
    exit 1
fi

JOBS=${1:2}
NAME=${PWD##*/}
NEW_UUID=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 5 | head -n 1)
mkdir -p logs
mkdir -p outputs
mkdir -p results
JOBS_FILE=jobs_file_$NEW_UUID.parallel
JOBS_LOG=logs/jobs_log_$NEW_UUID.parallel
echo -e "\n Starting Run $NEW_UUID\n\n"
echo $JOBS > $JOBS_FILE
echo "$NEW_UUID" > last.uuid

# DRYRUN=--dryrun

# Get configuration specific variables
source ./SpecificFiles/configs.sh


IDS=$(parallel $DRYRUN  --header : "echo $UUID_STRING" $PROCESSED_CONFIGS )
parallel $DRYRUN --gnu --bar --joblog $JOBS_LOG --halt soon,fail=1 --results builds/${NEW_UUID}/parallel -P $JOBS_FILE --header : "./utils/thread.sh ${NEW_UUID}_$UUID_STRING $JINJA_INPUT" $PROCESSED_CONFIGS

[[ -n $DRYRUN ]] && exit 0

OutFileName="results/results_${NAME}_$NEW_UUID.csv"
i=0
for id in $IDS; do
    filename=./builds/$NEW_UUID/$id/m5out/data_extracted.csv
    if [[ $i -eq 0 ]] ; then 
        head -1  "$filename" >   "$OutFileName" # Copy header if it is the first file
    fi
    tail -n +2  "$filename" >>  "$OutFileName" # Append from the 2nd line each file
    i=$(( $i + 1 ))                            # Increase the counter
done

# Tar.gz the results into outputs folder
TARGZ="tar -cz"
ZIPa="zip -rq"
TAR_OUTPUT=outputs/${NEW_UUID}.tar.gz 
ZIP_OUTPUT=outputs/${NEW_UUID}.zip 
$TARGZ $OutFileName ./builds/$NEW_UUID  -f $TAR_OUTPUT
$ZIPa  $ZIP_OUTPUT $OutFileName ./builds/$NEW_UUID

echo -e "\n\nFINISHED: build ${NAME} ${NEW_UUID}"

ATTACH="--attach="
if [[ "$(grep "^ID=" /etc/os-release)" == 'ID="centos"' ]]; then
    ATTACH="-a "
fi
if [[ $(command -v mail) ]]; then
    echo -e "In annex you can found the latest results from your run!\n The following runs were executed:\n$IDS\n From:$(hostname)" | mail -s "[$(hostname)]: Results $NAME/$NEW_UUID" ${ATTACH}$OutFileName ${ATTACH}$TAR_OUTPUT ${ATTACH}$ZIP_OUTPUT $EMAIL
fi
