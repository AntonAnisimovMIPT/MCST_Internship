#!/bin/bash

if [ "$#" -ne 5 ]; then
    echo "Invalid number of arguments passed to the script. Usage: <CACHE_DESCR> <NUM_TESTS> <NUM_OPERATIONS> <PROB> <OUTPUT_DIR>"
    exit 1
fi

CACHE_DESCR="$1"             
NUM_TESTS="$2"          
NUM_OPERATIONS="$3"     
PROB="$4"          
OUTPUT_DIR="$5"         

if [ ! -f "$CACHE_DESCR" ]; then
    echo "Error: Config file $CACHE_DESCR does not exist!!!"
    exit 1
fi

# могу убрать, сделал для своего удобства
if [ -d "$OUTPUT_DIR" ]; then
    rm -rf "$OUTPUT_DIR"
fi

mkdir -p "$OUTPUT_DIR"
TRACE_DIR="${OUTPUT_DIR}/traces"
mkdir -p "$TRACE_DIR"

for i in $(seq 1 "$NUM_TESTS"); do
    SEED=$((RANDOM))
    OUTPUT_FILE_GEN="${OUTPUT_DIR}/test_${i}_seed_${SEED}.txt"
    OUTPUT_FILE_TRACE="${TRACE_DIR}/trace_${i}_seed_${SEED}.txt"
    
    ./build/cache_generator --config "$CACHE_DESCR" --seed "$SEED" --num_operations "$NUM_OPERATIONS" --read_prob "$PROB" --output "$OUTPUT_FILE_GEN"
    
    if [ $? -ne 0 ]; then
        echo "Test $i error: Failed to generate test $i with seed $SEED."
        continue  
    fi

    ../Task_6/build/cache_modelling --config "$CACHE_DESCR" --input "$OUTPUT_FILE_GEN" > "$OUTPUT_FILE_TRACE"
    
    if [ $? -ne 0 ]; then
        echo "Test $i error: Cache simulator failed on test $i with seed $SEED."
        continue 
    fi

    echo "Test $i with seed $SEED completed successfully."
done

echo "End."
