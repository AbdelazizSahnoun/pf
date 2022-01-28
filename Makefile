all: pf work

check: pf work
	bats check.bats
