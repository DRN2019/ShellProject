#ifndef command_hh
#define command_hh

#include "simpleCommand.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/wait.h>


// Command Data Structure

struct Command {
  std::vector<SimpleCommand *> _simpleCommands;
  std::string * _outFile;
  std::string * _inFile;
  std::string * _errFile;
  bool _background;
  bool _append;

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  void clear();
  void print();
  void execute();
  bool isBuiltIn(int i);
  bool printenvCheck(int i);
  bool sourceCheck(int i);

  static SimpleCommand *_currentSimpleCommand;
};

#endif
