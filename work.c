#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void die(int i) {
	exit(0);
}

int main(int argc, char **argv) {
	int time = 1;
	if (argc>1) time = atoi(argv[1]);
	signal(SIGALRM, die);
	alarm(time);
	for(;;) getpid();
}
