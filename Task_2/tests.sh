#!/bin/bash

if [ "$#" -lt 10 ]; then
    echo "Invalid number of arguments passed to the script. Usage: <num_runs> <n_states_min> <n_states_max> <n_alph_in_min> <n_alph_in_max> <n_alph_out_min> <n_alph_out_max> <n_trans_out_min> <n_trans_out_max> <output_dir>"
    exit 1
fi

num_runs=$1
n_states_min=$2
n_states_max=$3
n_alph_in_min=$4
n_alph_in_max=$5
n_alph_out_min=$6
n_alph_out_max=$7
n_trans_out_min=$8
n_trans_out_max=$9
output_dir=${10}

mkdir -p $output_dir
mkdir -p "${output_dir}/jsons"
mkdir -p "${output_dir}/DOTs"
mkdir -p "${output_dir}/imgs"

generate_seed() {
    echo $RANDOM
}

for ((i=0; i<num_runs; i++)); 
do
    seed=$(generate_seed)
    output_file="${output_dir}/jsons/${seed}.json"
    output_file2="${output_dir}/DOTs/${seed}.DOT"
    output_file3="${output_dir}/imgs/${seed}.png"

    ./build/pseudorandom_machine_generator --seed $seed --n_states_min $n_states_min --n_states_max $n_states_max --n_alph_in_min $n_alph_in_min --n_alph_in_max $n_alph_in_max --n_alph_out_min $n_alph_out_min --n_alph_out_max $n_alph_out_max --n_trans_out_min $n_trans_out_min --n_trans_out_max $n_trans_out_max --out $output_file
    returned=$?
    if [ $returned -ne 0 ]; then
        echo "Error during pseudorandom_machine_generator execution with seed $seed"
        exit $returned
    fi

    ../Task_1/build/converter --input=$output_file --output=$output_file2
    returned=$?
    if [ $returned -ne 0 ]; then
        echo "Error during converter execution with seed $seed"
        exit $returned
    fi

    dot -Tpng $output_file2 -o $output_file3
    returned=$?
    if [ $returned -ne 0 ]; then
        echo "Error during dot execution with seed $seed"
        exit $returned
    fi

done
