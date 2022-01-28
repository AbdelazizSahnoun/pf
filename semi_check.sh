#!/bin/bash

# Le test du TP est compliqué, le plus simple est sans doute un simple comparaison visuelle entre pf et les sorties de time et perf.

set -x

./pf ./work
./fpf ./work

./pf sleep 1.5
./fpf sleep 1.5

./pf -u ./work
./fpf -u ./work

./pf -u sleep 1
./fpf -u sleep 1

./pf -c ./work
./fpf -c ./work

./pf -c sleep 1
./fpf -c sleep 1

./pf -a ./work
./fpf -a ./work

./pf -a sleep 1
./fpf -a sleep 1

./pf -n 5 ./work
./fpf ./work

./pf ./pf -n 2 sleep 1
./fpf ./pf -n 2 sleep 1

./pf -a -n 3 sleep 1.1
./fpf -a sleep 1.1

./pf -s './work; sleep .5'
./fpf sh -c './work; sleep .5'

./pf -a -n 3 -s './work& ./work; wait'
./fpf -a sh -c './work& ./work; wait'
