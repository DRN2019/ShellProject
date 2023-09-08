#ifndef shell_hh
#define shell_hh

#include "command.hh"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

struct Shell {

  static void prompt();
  static void exitShell();
  static Command _currentCommand;
};

#endif
