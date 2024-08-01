#!/bin/bash

if [ "$#" -lt 11 ]; then
    echo "Invalid number of arguments passed to the script. Usage: <num_runs> <n_states_min> <n_states_max> <n_alph_in_min> <n_alph_in_max> <n_alph_out_min> <n_alph_out_max> <n_trans_out_min> <n_trans_out_max> <path_len> <output_dir>"
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
path_len=${11}


mkdir -p "$output_dir/jsons"
mkdir -p "$output_dir/DOTs"
mkdir -p "$output_dir/imgs"
mkdir -p "$output_dir/sequences"

generate_seed() {
    echo $RANDOM
}

for ((i=0; i<num_runs; i++)); do
    seed=$(generate_seed)
    json_file="${output_dir}/jsons/${seed}.json"
    dot_file="${output_dir}/DOTs/${seed}.DOT"
    img_file="${output_dir}/imgs/${seed}.png"
    seq_file="${output_dir}/sequences/${seed}"

    echo "Generating machine with seed $seed..."
    ../Task_2/build/pseudorandom_machine_generator --seed $seed --n_states_min $n_states_min --n_states_max $n_states_max --n_alph_in_min $n_alph_in_min --n_alph_in_max $n_alph_in_max --n_alph_out_min $n_alph_out_min --n_alph_out_max $n_alph_out_max --n_trans_out_min $n_trans_out_min --n_trans_out_max $n_trans_out_max --out "$json_file"
    if [ $? -ne 0 ]; then
        echo "Error generating machine with seed $seed"
        exit 1
    fi

    echo "Generating DOT file..."
    ../Task_1/build/converter --input="$json_file" --output="$dot_file"
    if [ $? -ne 0 ]; then
        echo "Error converting to DOT with seed $seed"
        exit 1
    fi

    echo "Generating image from DOT file..."
    dot -Tpng "$dot_file" -o "$img_file"
    if [ $? -ne 0 ]; then
        echo "Error generating image from DOT file with seed $seed"
        exit 1
    fi

    echo "Generating sequences for coverage checking..."
    ../Task_3/build/sequence_formation --mode=states --out="${seq_file}_s.txt" "$json_file"
    if [ $? -eq 0 ]; then
        echo "  for states mode: generated"
    else
        echo "Error generating sequences for states mode"
        exit 1
    fi

    ../Task_3/build/sequence_formation --mode=transitions --out="${seq_file}_t.txt" "$json_file"
    if [ $? -eq 0 ]; then
        echo "  for transitions mode: generated"
    else
        echo "Error generating sequences for transitions mode"
        exit 1
    fi

    ../Task_3/build/sequence_formation --mode=paths --path-len="$path_len" --out="${seq_file}_p.txt" "$json_file"
    if [ $? -eq 0 ]; then
        echo "  for paths mode: generated"
    else
        echo "Error generating sequences for paths mode"
        exit 1
    fi

    echo "Checking coverage..."
    ./build/coverage_checking --mode states --seq "${seq_file}_s.txt" "$json_file"
    if [ $? -eq 0 ]; then
        echo "  for states mode: coveraged"
    else
        echo "Coverage check failed for states mode"
        exit 1
    fi

    ./build/coverage_checking --mode transitions --seq "${seq_file}_t.txt" "$json_file"
    if [ $? -eq 0 ]; then
        echo "  for transitions mode: coveraged"
    else
        echo "Coverage check failed for transitions mode"
        exit 1
    fi

    ./build/coverage_checking --mode paths --path-len "$path_len" --seq "${seq_file}_p.txt" "$json_file"
    if [ $? -eq 0 ]; then
        echo "  for paths mode: coveraged"
    else
        echo "Coverage check failed for paths mode"
        exit 1
    fi

    echo "----------"

    done

    echo "All machines generated and coverage checked successfully!!!"
