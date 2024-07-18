#!/bin/bash

for seed in {0..100}
do

    output_file="./tests/jsons/${seed}.json"
    output_file2="./tests/DOTs/${seed}.DOT"
    output_file3="./tests/imgs/${seed}.png"

    ./build/pseudorandom_machine_generator --seed $seed --n_states_min 10 --n_states_max 10 --n_alph_in_min 10 --n_alph_in_max 10 --n_alph_out_min 2 --n_alph_out_max 2 --n_trans_out_min 0 --n_trans_out_max 1 --out $output_file

    ../Task_1/build/converter --input=$output_file --output=$output_file2

    dot -Tpng $output_file2 -o $output_file3

done
