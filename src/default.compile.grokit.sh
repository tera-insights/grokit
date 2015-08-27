#!/bin/bash

# Set to the path of the C++ compiler
CXX=clang++

# Parameters to tweak default segment number calculations

# Minimum and maximum ratios for the amount of total memory to use for
# the hash table. Affects the number of segments in the hash table.
MIN_HASH_MEM_PCT="0.4"
MAX_HASH_MEM_PCT="0.6"

# The number of bytes per slot in the hash table. This shouldn't be changed.
BYTES_PER_SLOT=12

# Get System Information

# Old versions of NUM_OF_PROCS and NUM_OF_THREADS
#NUM_OF_PROCS=$(cat /proc/cpuinfo | grep processor | wc -l);
#NUM_OF_THREADS=$(cat /proc/cpuinfo | grep "cpu cores" | awk '{SUM += $ 4} END {print SUM}');

# In order to find the number of physical processors, you need to find the
# number of distinct physical ids.
NUM_OF_PROCS=$(cat /proc/cpuinfo | grep "physical id" | awk 'BEGIN {MAX = -1} {if ($4 > MAX ) MAX = $ 4} END {print (MAX + 1)}');

# Note: the iine below actually finds the number of threads, not the number of
# physical cores.
NUM_OF_THREADS=$(cat /proc/cpuinfo | grep "processor" | wc -l);

# The line below uses the program in find_cores.awk to find the number of
# physical cores, if there is ever any use for this knowledge.
NUM_OF_CORES=$(cat /proc/cpuinfo | awk -f find_cores.awk);

TOTAL_MEMORY=$(cat /proc/meminfo | grep MemTotal | awk '{print $ 2}');
FREE_MEMORY=$(cat /proc/meminfo | grep MemFree | awk '{print $ 2}');

#echo $NUM_OF_PROCS, $NUM_OF_THREADS, $TOTAL_MEMORY, $FREE_MEMORY

if [ "$1" = "make" ]; then
    # Compile DataPath using Makefile
    echo "Compiling DataPath using Makefile"
    echo
    sleep 2
    make clean; make -j$NUM_OF_THREADS
    exit 1
fi


#otherwise

#Helper functions

# The general idea of this function is this:
# For any number X that is a combination of N powers of 2,
# Y = X & (X - 1) is a number smaller than X that is a combination of
# N-1 powers of 2.
# Therefore, we can check if a number contains 1 or 2 powers of 2 by checking
# if Y is 0 or if Y & (Y - 1) is 0.
#
# $1 is the minimum and $2 is the maximum. We simply iterate through the
# interval looking for the first number that is a combination of 1 or 2 powers
# of 2.
GetTwosExponent(){
    for((i = $1; i <= $2; i++))
    do
        if [ "$i" -ne 0 ]; then
            local previousPower=$(($i & ($i - 1)))
            # If the previous combination of powers of two ends up being zero,
            # then i was a power of 2
            if [ $previousPower -eq 0 ]; then
                echo "$i"
                break
            else
                # If the combination of powers of 2 before previousPower is 0,
                # then i is a combination of 2 powers of 2
                local temp=$(( $previousPower & ($previousPower - 1) ))
                if [ $temp -eq 0 ]; then
                    echo "$i"
                    break
                fi
            fi
        fi
    done
}

# The hash allocates some extra space at the end of every segment as scratch
# space.
HASH_ALLOCATE_FACTOR=1.08

# $1 = total memory (in kB)
# $2 = percentage of memory to use
# $3 = exponent for number of slots per segment
# #4 = number of bytes per slot
CalcNumSegs(){
bc <<EOF
scale=20
total_mem=($1 * 1024)
mem_to_use=(total_mem * $2)
slots_per_seg=((2 ^ $3) * $HASH_ALLOCATE_FACTOR)
bytes_per_slot=$4
bytes_per_seg=(slots_per_seg * bytes_per_slot)
num_segs=(mem_to_use / bytes_per_seg)
scale=0
num_segs / 1
EOF
}

# $1 = Number of segments
# $2 = Exponent for number of slots per segment
# $3 = Number of bytes per slot
#
# Output is the estimated size of the hash in MB
CalcHashSize(){
bc <<EOF
scale=20
num_segs=$1
slots_per_seg=((2 ^ $2) * $HASH_ALLOCATE_FACTOR)
bytes_per_slot=$3
bytes_per_seg=(slots_per_seg * bytes_per_slot)
hash_size=(num_segs * bytes_per_seg)
scale=0
hash_size / (1024 ^ 2)
EOF
}

# check for libraries if not found, ask user to install them

# Prepare dp_config.php file
CONFIG_FILE=CONFIG_PHP
if [ -e $CONFIG_FILE ]; then
    rm $CONFIG_FILE
fi
touch $CONFIG_FILE
echo "<?php" >> $CONFIG_FILE
echo >> $CONFIG_FILE

DEFAULT_NUM_OTHER=$(($NUM_OF_THREADS*3/2))
DEFAULT_NUM_SLOTS=24


# Set the defaults for the actual used values
USED_NUM_SLOTS=$DEFAULT_NUM_SLOTS
USED_NUM_EETHREADS=$DEFAULT_NUM_OTHER
USED_NUM_DISK_TOKENS=$DEFAULT_NUM_OTHER

# Get User Defined Settings, if any
if [ ! -e configure ]; then
    cp configure.example configure
fi

UD_NUM_SLOTS=$(grep 'USER_DEFINED_NUM_SLOTS_IN_SEGMENT_BITS' configure | awk '{if (index($0,"//") == 0) str=substr($0,index($0,"=")+1); else str=-1; print str}');
UD_NUM_SEGS=$(grep 'USER_DEFINED_NUM_SEGS' configure | awk '{if (index($0,"//") == 0) str=substr($0,index($0,"=")+1); else str=-1; print str}');
UD_NUM_EETHREADS=$(grep 'USER_DEFINED_NUM_EXEC_ENGINE_THREADS' configure | awk '{if (index($0,"//") == 0) str=substr($0,index($0,"=")+1); else str=-1; print str}');
UD_NUM_DISK_TOKENS=$(grep 'USER_DEFINED_NUM_DISK_TOKENS' configure | awk '{if (index($0,"//") == 0) str=substr($0,index($0,"=")+1); else str=-1; print str}');

#echo $UD_NUM_SEGS $UD_NUM_SLOTS $UD_NUM_EETHREADS $UD_NUM_DISK_TOKENS

#Set NUM_SLOTS_IN_SEGMENT_BITS
if [ $UD_NUM_SLOTS -le 0 ]; then
    echo "Using default NUM_SLOTS_IN_SEGMENT_BITS = $DEFAULT_NUM_SLOTS"
    echo "\$__grokit_config_segment_bits = $DEFAULT_NUM_SLOTS;" >> $CONFIG_FILE
    #sed "/#define NUM_SLOTS_IN_SEGMENT_BITS/c #define NUM_SLOTS_IN_SEGMENT_BITS $DEFAULT_NUM_SLOTS" -i CONSTANTS_M4
else
    if [ $UD_NUM_SLOTS -gt 0 ] && [ $UD_NUM_SLOTS -le 24 ]; then
        echo "Using NUM_SLOTS_IN_SEGMENT_BITS = $UD_NUM_SLOTS"
    #sed "/#define NUM_SLOTS_IN_SEGMENT_BITS/c #define NUM_SLOTS_IN_SEGMENT_BITS $UD_NUM_SLOTS" -i CONSTANTS_M4
    USED_NUM_SLOTS=$UD_NUM_SLOTS
    else
        echo "Warning: NUM_SLOTS_IN_SEGMENT_BITS should not be over 24 bits"
        echo "Do you want to continue with NUM_SLOTS_IN_SEGMENT_BITS = $UD_NUM_SLOTS? y/n"
        read resp
        if [ "$resp" = "n" -o "$resp" = "N" ]; then
            echo "Aborting compiling DataPath."
            rm $CONFIG_FILE
            exit 1
        fi
        echo "Using NUM_SLOTS_IN_SEGMENT_BITS = $UD_NUM_SLOTS"
        #sed "/#define NUM_SLOTS_IN_SEGMENT_BITS/c #define NUM_SLOTS_IN_SEGMENT_BITS $UD_NUM_SLOTS" -i CONSTANTS_M4
        USED_NUM_SLOTS=$UD_NUM_SLOTS
    fi
fi
echo "\$__grokit_config_segment_bits = $USED_NUM_SLOTS;" >> $CONFIG_FILE

# We have to calculate the number of segments here, because only now do we know
# the actual number of bits used for slots per segment
MIN_NUM_SEGS=$(CalcNumSegs $TOTAL_MEMORY $MIN_HASH_MEM_PCT $USED_NUM_SLOTS $BYTES_PER_SLOT)
MAX_NUM_SEGS=$(CalcNumSegs $TOTAL_MEMORY $MAX_HASH_MEM_PCT $USED_NUM_SLOTS $BYTES_PER_SLOT)
#echo "Min Segs: $MIN_NUM_SEGS Max Segs: $MAX_NUM_SEGS"

DEFAULT_NUM_SEGS=$(GetTwosExponent $MIN_NUM_SEGS $MAX_NUM_SEGS)
USED_NUM_SEGS=$DEFAULT_NUM_SEGS

#Set NUM_SEGS
if [ $UD_NUM_SEGS -le 0 ]; then
    echo "Using default NUM_SEGS = $DEFAULT_NUM_SEGS"
    #sed "/#define NUM_SEGS/c #define NUM_SEGS $DEFAULT_NUM_SEGS" -i CONSTANTS_M4
else
    if [ $UD_NUM_SEGS -ge $MIN_NUM_SEGS ] && [ $UD_NUM_SEGS -le $MAX_NUM_SEGS ]; then
    echo "Using NUM_SEGS = $UD_NUM_SEGS"
    #sed "/#define NUM_SEGS/c #define NUM_SEGS $UD_NUM_SEGS" -i CONSTANTS_M4
    USED_NUM_SEGS=$UD_NUM_SEGS
    else
    echo "Warning: For this system, NUM_SEGS should be between $MIN_NUM_SEGS and $MAX_NUM_SEGS, ideally $DEFAULT_NUM_SEGS"
    echo "Do you want to continue with NUM_SEGS = $UD_NUM_SEGS? y/n"
    read resp
    if [ "$resp" = "n" -o "$resp" = "N" ]; then
        echo "Aborting compiling DataPath."
        rm $CONFIG_FILE
        exit 1
    fi
    echo "Using NUM_SEGS = $UD_NUM_SEGS"
    #sed "/#define NUM_SEGS/c #define NUM_SEGS $UD_NUM_SEGS" -i CONSTANTS_M4
    USED_NUM_SEGS=$UD_NUM_SEGS
    fi
fi
echo "\$__grokit_config_num_segs = $USED_NUM_SEGS;" >> $CONFIG_FILE

#Set NUM_EXEC_ENGINE_THREADS and MAX_CLEANER_CPU_WORKERS
if [ $UD_NUM_EETHREADS -le 0 ] || [ $UD_NUM_EETHREADS -eq $DEFAULT_NUM_OTHER ]; then
    echo "Using default NUM_EETHREADS = $DEFAULT_NUM_OTHER"
    #sed "/#define NUM_EXEC_ENGINE_THREADS/c #define NUM_EXEC_ENGINE_THREADS $DEFAULT_NUM_OTHER" -i CONSTANTS_M4
    #sed "/#define MAX_CLEANER_CPU_WORKERS/c #define MAX_CLEANER_CPU_WORKERS $DEFAULT_NUM_OTHER" -i CONSTANTS_M4
else
    echo "Warning: For this system, NUM_EXEC_ENGINE_THREADS should be $DEFAULT_NUM_OTHER)"
    echo "Do you want to continue with NUM_EXEC_ENGINE_THREADS = $UD_NUM_EETHREADS? y/n"
    read resp
    if [ "$resp" = "n" -o "$resp" = "N" ]; then
        echo "Aborting compiling DataPath."
        rm $CONFIG_FILE
        exit 1
    fi
    echo "Using NUM_EXEC_ENGINE_THREADS = $UD_NUM_EETHREADS"
    #sed "/#define NUM_EXEC_ENGINE_THREADS/c #define NUM_EXEC_ENGINE_THREADS $UD_NUM_EETHREADS" -i CONSTANTS_M4
    #sed "/#define MAX_CLEANER_CPU_WORKERS/c #define MAX_CLEANER_CPU_WORKERS $UD_NUM_EETHREADS" -i CONSTANTS_M4
    USED_NUM_EETHREADS=$UD_NUM_EETHREADS
fi
echo "\$__grokit_config_exec_threads = $USED_NUM_EETHREADS;" >> $CONFIG_FILE
echo "\$__grokit_config_cpu_cleaners = $USED_NUM_EETHREADS;" >> $CONFIG_FILE

#Set NUM_DISK_TOKENS and MAX_CLEANER_DISK_REQUESTS
if [ $UD_NUM_DISK_TOKENS -le 0 ] || [ $UD_NUM_DISK_TOKENS -eq $DEFAULT_NUM_OTHER ]; then
    echo "Using default NUM_DISK_TOKENS = $DEFAULT_NUM_OTHER"
    #sed "/#define NUM_DISK_TOKENS/c #define NUM_DISK_TOKENS $DEFAULT_NUM_OTHER" -i CONSTANTS_M4
    #sed "/#define MAX_CLEANER_DISK_REQUESTS/c #define MAX_CLEANER_DISK_REQUESTS $DEFAULT_NUM_OTHER" -i CONSTANTS_M4

else
    echo "Warning: For this system, NUM_DISK_TOKENS should be $DEFAULT_NUM_OTHER"
    echo "Do you want to continue with NUM_DISK_TOKENS = $UD_NUM_DISK_TOKENS? y/n"
    read resp
    if [ "$resp" = "n" -o "$resp" = "N" ]; then
        echo "Aborting compiling DataPath."
        rm $CONFIG_FILE
        exit 1
    fi
    echo "Using NUM_DISK_TOKENS = $UD_NUM_DISK_TOKENS"
    #sed "/#define NUM_DISK_TOKENS/c #define NUM_DISK_TOKENS $UD_NUM_DISK_TOKENS" -i CONSTANTS_M4
    #sed "/#define MAX_CLEANER_DISK_REQUESTS/c #define MAX_CLEANER_DISK_REQUESTS $UD_NUM_DISK_TOKENS" -i CONSTANTS_M4
    USED_NUM_DISK_TOKENS=$UD_NUM_DISK_TOKENS
fi
echo "\$__grokit_config_disk_tokens = $USED_NUM_DISK_TOKENS;" >> $CONFIG_FILE
echo "\$__grokit_config_cleaner_disk_requests = $USED_NUM_DISK_TOKENS;" >> $CONFIG_FILE

#cat CONSTANTS_M4
#grep 'USER_DEFINED_' configure |  \
#awk \
#'{print "m4_define(</"$1"/>,</"$NF"/>)"}' \
#> userdefined;

#sed "$LINE_NUMBER r userdefined" -i  CONSTANTS_M4;

# Confirmation prompt. Nothing in the source is changed until this is prompt is
# accepted.
echo
echo "Configured DataPath with the following parameters:"
echo "Slots per Segment:        2^$USED_NUM_SLOTS"
echo "Segments:                 $USED_NUM_SEGS"
echo "Approx. Hash Table Size:  $(CalcHashSize $USED_NUM_SEGS $USED_NUM_SLOTS $BYTES_PER_SLOT) MB"
echo "EE Threads:               $USED_NUM_EETHREADS"
echo "Disk Tokens:              $USED_NUM_DISK_TOKENS"
echo

echo "?>" >> $CONFIG_FILE
mv $CONFIG_FILE PHP/php/grokit_config.php
#mv CONSTANTS_M4 Global/m4/Constants.h.m4

# Remake the parser
echo "Making parser."
./parserMake.sh

if (( $? != 0 )); then
    echo "Failed to compile parser. Aborting compilation."
    exit 1
fi

echo
# Clean old stuff
if [ ! -f maker ]; then
    #Compile maker.cc
    echo "No maker exists. Compiling maker.cc"
    echo
    sleep 2
    $CXX -o maker maker.cc
fi

echo "Creating Makefile"
echo
# Prepare Makefile using maker
sleep 2
./maker executables.lemon Makefile_prelude

if [ $? -ne 0 ]; then
    # Sometimes the maker fails the first time. Run it again
    # if that is the case
    ./maker executables.lemon Makefile_prelude
fi

# Compile DataPath using Makefile
echo "Compiling DataPath using Makefile"
echo
sleep 2
make clean; make -j$NUM_OF_THREADS

exit 0
