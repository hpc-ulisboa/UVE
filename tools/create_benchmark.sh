#!/bin/bash -e

usage() {
    echo "New benchmark creator:"
    echo "$0 <name> <args...>"
    echo "$0 -h/--help"
    echo ""
    echo "Called with $0 $@"
    echo ""
}

correct_links() {
    link=$1
    # grab the target of the old link
    target=$(readlink -- "$1")
    # replace the first occurrence of oldusername with newusername in the target string
    target=${target/$2/$3}
    # Test the link creation
    # echo ln -s -- "$target" "$link"
    rm "$link"
    ln -s "$target" "$link"
}

if [[ -z $EMAIL ]]; then
    EMAIL=""
fi

ARGS=( "Arch" "Cpu" ${@:2} )
BENCHMARK=../benchmarks/$1
SYMLINK_CORRECT=../../tools/
BASE="benchmark_base"

if [[ -z $1 || $1 == "-h" || $1 == "--help" ]]; then
    usage $1 ${@:2}
    exit
fi

echo "Creating benchmark $1 ($BENCHMARK) with Args:${ARGS[@]}"
echo ""


echo "<-- Starting -->"

#Go to script directory, easier with symlinks
cd $(realpath $(dirname $0))

echo "Populating benchmark with default files.."
cp -r $BASE $BENCHMARK
CONFIGS=()
UUID=()
JINJA=()
echo "Templating configurations with provided arguments.."
for i in "${ARGS[@]}"; do
    if ! [[ $i =~ ^(Arch|Cpu)$ ]]; then        
        echo -e "${i^^}\n//Put values below" > $BENCHMARK/Args/$i
    fi
    CONFIGS+=("::: \$(spcom Args/$i)")
    UUID+=("{${i^^}}")
    JINJA+=("${i^^}={${i^^}}")
done


echo "Updating config strings with data.."
CONFIGS_STR=${CONFIGS[@]}
UUID_STR=$( IFS=$'_'; echo "${UUID[*]}" )
JINJA_STR=${JINJA[@]}
sed -i "/PROCESSED_CONFIGS=\"\"/c\\PROCESSED_CONFIGS=\"$CONFIGS_STR\"" $BENCHMARK/SpecificFiles/configs.sh
sed -i "/UUID_STRING=\"\"/c\\UUID_STRING=\"$UUID_STR\"" $BENCHMARK/SpecificFiles/configs.sh
sed -i "/JINJA_INPUT=\"\"/c\\JINJA_INPUT=\"$JINJA_STR\"" $BENCHMARK/SpecificFiles/configs.sh 
sed -i "/EMAIL=\"\"/c\\EMAIL=\"$EMAIL\"" $BENCHMARK/SpecificFiles/configs.sh 

#Correct symlinks
echo "Updating symlinks to match directory structure.."
cd $BENCHMARK
for link in $(find ! -name . -prune -type l); do
    correct_links $link ../ $SYMLINK_CORRECT
done

echo "<-- Finished -->"
echo "Created $BENCHMARK folder"
echo "Args configuration files are in $BENCHMARK/Args/{${ARGS[@]}}"
echo "Jinja templating variables: ${ARGS[@]^^}"
echo ""
echo "Please update the stats extraction configuration at $BENCHMARK/StaticFiles/ExtractTargets.csv"
echo "Change the files at $BENCHMARK/SpecificFiles/{main/kernel}.c"
echo "The results will be automatically forward to $EMAIL"
echo "Export new \$EMAIL to change the destination"



