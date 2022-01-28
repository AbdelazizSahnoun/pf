#!/usr/bin/env bats

load test_helper

@test "base work" {
	run ./pf ./work
	check 0 "1.00"
}

@test "base sleep" {
	run ./pf sleep 1.5
	check 0 "1.50"
}

@test "base false" {
	run ./pf false
	test "$status" != 0
	test "$output" = "0.00"
}

@test "error fail" {
	run ./pf fail
	test "$status" = 127
	test "$output" != ""
}

@test "-u work" {
	run ./pf -u ./work
	test "$status" = 0
}

@test "-u sleep" {
	run ./pf -u sleep 1
	check 0 "0.00"
}

@test "-c work" {
	run ./pf -c ./work
	test "$status" = 0
}

@test "-c sleep" {
	run ./pf -c sleep 1
	test "$status" = 0
}

@test "-a work" {
	run ./pf -a ./work
	test "$status" = 0
}

@test "-a sleep" {
	run ./pf -a sleep 1
	test "$status" = 0
}

@test "-n work" {
	run ./pf -n 5 ./work
	test "$status" = 0
}

@test "pf -n sleep" {
	run ./pf ./pf -n 2 sleep 1
	test "$status" = 0
}

@test "-n -a sleep" {
	run ./pf -a -n 3 sleep 1.1
	test "$status" = 0
}

@test "-s work sleep" {
	run ./pf -s './work; sleep .5'
	check 0 "1.50"
}

@test "-s -n -a work wok" {
	run ./pf -a -n 3 -s './work& ./work; wait'
	test "$status" = 0
}
