#!/bin/bash
#This file sets the per execution specific args and their handling

#SPCOM removes commented lines from any text input
function spcom () {
    cat $1 | sed -e "/\/\/.*/d" -e '/\/\*.*\*\//d'
}

#Processed configs must be set in a gnu parallel arguments string style
# PROCESSED_CONFIGS="::: $(spcom Args/Arch) ::: $(spcom Args/CPU) ::: $(spcom Args/Prefetcher) ::: $(spcom Args/Size1) ::: $(spcom Args/Size2)"
PROCESSED_CONFIGS="::: $(spcom Args/Arch) ::: $(spcom Args/Cpu) ::: $(spcom Args/sn) ::: $(spcom Args/steps)"

echo "CONFIGS: $PROCESSED_CONFIGS"

#UUID execution string (Will be prepended with a 6 char random ID)
# UUID_STRING="{ARCH}_{CPU}_{prftch}_{Size1}_{Size2}"
UUID_STRING="{ARCH}_{CPU}_{SN}_{STEPS}"

#INPUT for the JINJA templating engine
# JINJA_INPUT="ARCH={ARCH} CPU={CPU} PREFETCHER={prftch} Size1={Size1} Size2={Size2}"
JINJA_INPUT="ARCH={ARCH} CPU={CPU} SN={SN} STEPS={STEPS}"

#Get global configurations
. StaticFiles/global_configs.sh
