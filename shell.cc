#include <cstdio>

#include "shell.hh"

int yyparse(void);
void yyrestart(FILE * file);

void Shell::prompt() {
	if (isatty(0)) {
		char* value;
		value = getenv("PROMPT");
		if (value == NULL) {
			printf("myshell>");
			fflush(stdout);
		}
		else {
			printf("%s", value);
			fflush(stdout);
		}
		
	}
}

void Shell::exitShell() {
  printf("\nGood bye!!\n");
  fflush(stdout);
  exit(1);
}

extern "C" void disp( int sig )
{
  printf("\n");
	Shell::prompt();
}

extern "C" void zombieHand(int sig) {
	int pid = wait3(0, 0, NULL);

	while (waitpid(-1, NULL, WNOHANG) > 0) {};
	//printf("[%d] exited.\n", pid);
}


int main() {
  struct sigaction sa;
  sa.sa_handler = disp;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if(sigaction(SIGINT, &sa, NULL)){
    perror("sigaction");
    _exit(2);
  }

	struct sigaction zombie;
	zombie.sa_handler = zombieHand;
	sigemptyset(&zombie.sa_mask);
	zombie.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &zombie, NULL)) {
		perror("sigaction");
		exit(2);
	}


  FILE* fileptr = fopen(".shellrc", "r");
	if (fileptr) {

		yyrestart(fileptr);
		yyparse();
		yyrestart(stdin);
		fclose(fileptr);

	}
	else {
    if (isatty(0)) {
		  Shell::prompt();
    }
	}

  yyparse();
}

Command Shell::_currentCommand;
