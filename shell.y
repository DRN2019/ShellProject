
/*
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 *
 */

%code requires 
{
#include <string>
#include <sys/types.h>
#include <string.h>
#include <regex.h>
#include <dirent.h>
#include <algorithm>

#define MAXFILENAME 1024

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD
%token NOTOKEN GREAT NEWLINE PIPE LESS GREATGREAT GREATAMPERSAND GREATGREATAMPERSAND AMPERSAND TWOGREAT EXIT

%{
//#define yylex yylex
#include <cstdio>
#include "shell.hh"
#include <string>

void yyerror(const char * s);
int yylex();
void expandWildcard(char * prefix, char * suffix);
bool cmpfunction (char * i, char * j);

static std::vector<char *> _sortArgument = std::vector<char *>();
static bool wildCard;

%}

%%

goal:
  commands
  ;

commands:
  command
  | commands command
  ;

command: simple_command
       ;

simple_command:	
  pipe_list iomodifier_list background_opt NEWLINE {
    // printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();
  }
  | NEWLINE {
    Shell::prompt();
  }
  | error NEWLINE { yyerrok; }
  ;

command_and_args:
  command_word argument_list {
    Shell::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  WORD {
    // printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
    wildCard = false;
    char *p = (char *)"";
    expandWildcard(p, (char *)$1->c_str());
    std::sort(_sortArgument.begin(), _sortArgument.end(), cmpfunction);
    for (auto a: _sortArgument) {
      std::string * argToInsert = new std::string(a);
      Command::_currentSimpleCommand->insertArgument(argToInsert);
    }
    _sortArgument.clear();
  }
  ;

pipe_list:
  pipe_list PIPE command_and_args
  | command_and_args
  ;

command_word:
  EXIT {
    Shell::exitShell();
  }
  | WORD {
    // printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

iomodifier_list:
  iomodifier_list iomodifier_opt
  | iomodifier_opt
  | /* can be empty */
  ;

iomodifier_opt:
  GREAT WORD {
    // printf("   Yacc: redirect output \"%s\"\n", $2->c_str());
    if (Shell::_currentCommand._outFile != NULL ){ 
      fprintf(stderr, "Ambiguous output redirect.\n");
      exit(0);
    }
    Shell::_currentCommand._append = false;
    Shell::_currentCommand._outFile = $2;
  }

  | LESS WORD {
    // printf("   Yacc: redirect input \"%s\"\n", $2->c_str() );
    if (Shell::_currentCommand._inFile != NULL ){ 
      fprintf(stderr, "Ambiguous output redirect.\n");
      exit(0);
    }
    Shell::_currentCommand._inFile = $2;
  }

  | GREATGREAT WORD {
    // printf("   Yacc: append output \"%s\"\n", $2->c_str());
    if (Shell::_currentCommand._outFile != NULL ){ 
      fprintf(stderr, "Ambiguous output redirect.\n");
      exit(0);
    }
    Shell::_currentCommand._append = true;
    Shell::_currentCommand._outFile = $2;
  }

  | GREATAMPERSAND WORD {
    // printf("   Yacc: redirect output and error \"%s\"\n", $2->c_str());
    if (Shell::_currentCommand._outFile != NULL ){ 
      fprintf(stderr, "Ambiguous output redirect.\n");
      exit(0);
    }
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._errFile = $2;
  }

  | GREATGREATAMPERSAND WORD {
    // printf("   Yacc: append output and error \"%s\"\n", $2->c_str());
    if (Shell::_currentCommand._outFile != NULL ){ 
      fprintf(stderr, "Ambiguous output redirect.\n");
      exit(0);
    }
    Shell::_currentCommand._append = true;
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._errFile = $2;
  }

  | TWOGREAT WORD {
    // printf("   Yacc: redirect error \"%s\"\n", $2->c_str());
    Shell::_currentCommand._errFile = $2;
  }
  ;

background_opt:
  AMPERSAND {
    // printf("   Yacc: Background set to true\n");
    Shell::_currentCommand._background = true;
  }
  |
  ;



%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

bool cmpfunction (char * i, char * j) { return strcmp(i,j)<0; }

void expandWildcard(char * prefix, char * suffix) {
  if (suffix[0] == 0) {
    _sortArgument.push_back(strdup(prefix));
    return;
  }
  char Prefix[MAXFILENAME];
  if (prefix[0] == 0) {
    if (suffix[0] == '/') {suffix += 1; sprintf(Prefix, "%s/", prefix);}
    else strcpy(Prefix, prefix);
  }
  else
    sprintf(Prefix, "%s/", prefix);

  char * s = strchr(suffix, '/');
  char component[MAXFILENAME];
  if (s != NULL) {
    strncpy(component, suffix, s-suffix);
    component[s-suffix] = 0;
    suffix = s + 1;
  }
  else {
    strcpy(component, suffix);
    suffix = suffix + strlen(suffix);
  }

  char newPrefix[MAXFILENAME];
  if (strchr(component,'?')==NULL & strchr(component,'*')==NULL) {
    if (Prefix[0] == 0) strcpy(newPrefix, component);
    else sprintf(newPrefix, "%s/%s", prefix, component);
    expandWildcard(newPrefix, suffix);
    return;
  }
  
  char * reg = (char*)malloc(2*strlen(component)+10);
  char * r = reg;
  *r = '^'; r++;
  int i = 0;
  while (component[i]) {
    if (component[i] == '*') {*r='.'; r++; *r='*'; r++;}
    else if (component[i] == '?') {*r='.'; r++;}
    else if (component[i] == '.') {*r='\\'; r++; *r='.'; r++;}
    else {*r=component[i]; r++;}
    i++;
  }
  *r='$'; r++; *r=0;

  regex_t re;
  int expbuf = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);
  
  char * dir;
  if (Prefix[0] == 0) dir = (char*)"."; else dir = Prefix;
  DIR * d = opendir(dir);
  if (d == NULL) {
    return;
  }
  struct dirent * ent;
  bool find = false;
  while ((ent = readdir(d)) != NULL) {
    if(regexec(&re, ent->d_name, 1, NULL, 0) == 0) {
      find = true;
      if (Prefix[0] == 0) strcpy(newPrefix, ent->d_name);
      else sprintf(newPrefix, "%s/%s", prefix, ent->d_name);

      if (reg[1] == '.') {
        if (ent->d_name[0] != '.') expandWildcard(newPrefix, suffix);
      } else 
        expandWildcard(newPrefix, suffix);
    }
  }
  if (!find) {
    if (Prefix[0] == 0) strcpy(newPrefix, component);
    else sprintf(newPrefix, "%s/%s", prefix, component);
    expandWildcard(newPrefix, suffix);
  }
  closedir(d);
  regfree(&re);
  free(reg);
}

#if 0
main()
{
  yyparse();
}
#endif
