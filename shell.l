
/*
 * shell.l: lexical analyzer for shell
 *
 */

%{

#include <cstring>
#include "y.tab.hh"
#include <unistd.h>

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

extern "C" char * read_line();

int mygetc(FILE * f) {
	static char *p;
	char ch;

	if (!isatty(0)) {
	 // stdin is not a tty. Call real getc
	 return getc(f);
	}

	// stdin is a tty. Call our read_line.

	if (p==NULL || *p == 0) {
	 char * s = read_line();
	 p = s;
	}

	ch = *p;
	p++;

	return ch;
}

#undef getc
#define getc(f) mygetc(f)

%}

%option noyywrap

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

"exit" {
  return EXIT;
}

">" {
  return GREAT;
}

"|" {
  return PIPE;
}

"<" {
  return LESS;
}

">>" {
  return GREATGREAT;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

"2>" {
  return TWOGREAT;
}

"&" {
  return AMPERSAND;
}

[`][^\n][`] {
  // Get rid of wrapper
  std::string input = yytext;
  std::string subinput1 = input.substr(1);
  std::size_t pos = subinput1.find("`");
  std::string str = subinput1.substr(0, pos-1);

	char *subshell = const_cast<char *> (str.c_str());

  int tempin = dup(0);
	int tempout = dup(1);

	int pin[2];	//subshell
  int pout[2];	//outshell

	pipe(pin);
	pipe(pout);

  //subshell command in
	write(pin[1], subshell, strlen(subshell));
	write(pin[1], "\n", 1);
	close(pin[1]);

  // Get outputs of subshell
	dup2(pin[0], 0);	//pin[0] to input
  dup2(pout[1], 1);	//
	close(pout[1]);
  close(pin[0]);	

  int pid = fork();

  // Check for child process
	if (pid == 0) {	
		execvp("/proc/self/exe", NULL);
		_exit(1);
	} else if (pid < 0) {
		perror("fork");
		_exit(1);
	}

	dup2(tempin, 0);
	dup2(tempout, 1);
	close(tempin);
	close(tempout);

  char c;
	char * buffer = (char *) malloc (2048);
	int i = 0;
	
	while (read(pout[0], &c, 1)) {
		if (c == '\n') {	//if \n then put a space
			buffer[i++] = ' ';
		}
		else {
			buffer [i++] = c;
		}
	}
	buffer[i] = '\0';

	//read backward
	int j = i - 1;
	for (j = i - 1; j >= 0; j--) {	
		myunputc(buffer[j]);
	}

}

\$\([^\n]*\) {
  // Get rid of wrapper
  std::string input = yytext;
  std::string subinput1 = input.substr(2);
  std::size_t pos = subinput1.find(")");
  std::string str = subinput1.substr(0, pos);

  char *subshell = const_cast<char *> (str.c_str());

  int tempin = dup(0);
	int tempout = dup(1);

	int pin[2];	//subshell
  int pout[2];	//outshell

	pipe(pin);
	pipe(pout);

	write(pin[1], subshell, strlen(subshell));
	write(pin[1], "\n", 1);
	close(pin[1]);

  // Get outputs of subshell
	dup2(pin[0], 0);	//pin[0] to input
  dup2(pout[1], 1);	//
	close(pout[1]);
  close(pin[0]);	

  int pid = fork();

  // Check for child process
	if (pid == 0) {	
		execvp("/proc/self/exe", NULL);
		_exit(1);
	} else if (pid < 0) {
		perror("fork");
		_exit(1);
	}

	dup2(tempin, 0);
	dup2(tempout, 1);
	close(tempin);
	close(tempout);

  char c;
	char * buf = (char *) malloc (2048);
	int i = 0;
	
	while (read(pout[0], &c, 1)) {
		if (c == '\n') {	
			// Put a space in place of newline
			buf[i++] = ' ';
		}
		else {
			buf [i++] = c;
		}
	}
	buf[i] = '\0';

	// Read in reverse
	int j;
	for (j = i - 1; j >= 0; j--) {	
		myunputc(buf[j]);
	}

}

$\{[^ \}\n]*\}([^ \n]*|$\{[^ \}\n]*\})* {
  std::string str = std::string(yytext);
  std::string result = "";
  std::size_t found = str.find('$');
  while(found!=-1) {
    std::string component;
    result += str.substr(0, found);
    if (str[found+1] == '{') {
      std::size_t end = str.find('}');
      if (end != -1) {
        component = str.substr(found+2, end-found-2);
        if (getenv(component.c_str())) result += getenv(component.c_str());
        else result += component;      
        str = str.substr(end+1,-1);
      }
    }
    found = str.find('$');
  }
  result += str;
  /*result = result + str.substr(2,str.size()-3);
  if (getenv(str.c_str()))
    yylval.cpp_string = new std::string(getenv(str.c_str()));
  else
    yylval.cpp_string = new std::string(str);*/
  yylval.cpp_string = new std::string(result);
  return WORD;
}

~[^ \n\t]* {
  std::string str = std::string(yytext);
  std::string home = getenv("HOME");
  if (str.size() == 1) {
    yylval.cpp_string = new std::string(home);
  } 
  else {
    if (str[1] == '/') {
      str = home + str.substr(1, -1);
    } 
    else {
      str = std::string("/homes/") + str.substr(1,-1);
    }
    yylval.cpp_string = new std::string(str);
  }
  return WORD;
}


["][^\n\"]*["] {
	yylval.cpp_string = new std::string(yytext);
	int len = yylval.cpp_string->length();
	*yylval.cpp_string = yylval.cpp_string->substr(1, len - 2);

	return WORD;
}


[^ \t\n|><&]*\\[^ \t\n]* {
  // Escaping
	char * str = strdup(yytext);
	char * newstr = (char*) malloc (200);

	int i = 0;
	while (*str != NULL){
		if (*str == '\\'){
			if (*(str + 1) == '\\'){
				str += 1;
				newstr[i] = '\\';
				i++;
			}
			else{
				newstr[i] = *(str + 1);
				str += 1;
				i++;
			}
		} 
		else{
			newstr[i] = *str;
			i++;
		}
		str += 1;
	}
	newstr[i] = '\0';

	yylval.cpp_string = new std::string(newstr);

	return WORD;
}


[^ \t\n|><][^ \t\n|><]*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

