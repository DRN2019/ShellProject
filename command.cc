
#include <cstdio>
#include <cstdlib>

#include <iostream>

#include "command.hh"
#include "shell.hh"

void myunputc(int c); 


Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _append = false;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if ( _outFile ) {
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile ) {
        delete _errFile;
    }
    _errFile = NULL;

    _background = false;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

bool Command::isBuiltIn(int i) {

    // Check for unsetenv
    if( strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "unsetenv") == 0 ) {
        char * env1 = const_cast<char*>(_simpleCommands[i]->_arguments[1]->c_str());

		if (unsetenv(env1)) {
			perror("unsetenv");
		}
		clear();
		if (isatty(0)){
		    Shell::prompt();
        }
		return true;
	}


    // Check for setenv
    if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "setenv") == 0) {
        char * env1 = const_cast<char*>(_simpleCommands[i]->_arguments[1]->c_str());
        char * env2 = const_cast<char*>(_simpleCommands[i]->_arguments[2]->c_str());

        if (setenv(env1, env2, 1)) {
            perror("setenv");
        }

        clear();
        if (isatty(0)){
		    Shell::prompt();
        }
        return true;
    }

    // Check for cd
	if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "cd") == 0) {

		int err;
		if (_simpleCommands[i]->_arguments.size() != 1) {
            char * env1 = const_cast<char*>(_simpleCommands[i]->_arguments[1]->c_str());
			err = chdir(env1);
		}
		else {
            err = chdir(getenv("HOME"));
		}

		if (err < 0) {	//if err
            char * env1 = const_cast<char*>(_simpleCommands[i]->_arguments[1]->c_str());
			fprintf(stderr, "cd: can't cd to %s\n", env1);
		}

		clear();
        if (isatty(0)){
		    Shell::prompt();
        }
		return true;
	}


	return false;

}

bool Command::printenvCheck(int i) {
    // Check for printenv
    if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv") == 0) {
        char ** envList = environ;
        while (*envList != NULL) {
            printf("%s\n", *envList);
            envList++;
        }

        clear();
        if (isatty(0)){
		    Shell::prompt();
        }
        return true;
    }


	return false;
}

bool Command::sourceCheck(int i) {
    if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "source") == 0) {
        if (_simpleCommands[0]->_arguments.size() < 2) {
            //print error
            printf("Usage: source FILE \n");
            return false;
            //break;
        }
        //read file character by character
        char buffer[1000];
        int i = 0;
        char c;
        //do things
        char * x = new char[strlen(_simpleCommands[i]->_arguments[1]->c_str())+1];
        // Convert each argument to a string
        x = const_cast<char*>(_simpleCommands[i]->_arguments[1]->c_str());

        // End each argument string with null byte
        x[strlen(_simpleCommands[i]->_arguments[1]->c_str())] = '\0';

        int file = open(x, NULL);
        while (read(file, &c, 1)) {
            if (c == '\n') {
                //printf("NEWLINE\n");
                buffer[i] = 0;
                i = i + 1;
                //printf("Unputting with i=%d\n", i);
                int j;
                for (j = i; j > -1; j--) {
                    //printf("unput loop\n");
                    //printf("%c", buffer[j]);
                    myunputc(buffer[j]);
                }
            }
            else {
                //printf("%c", c);
                buffer[i] = c;
                i++;
            }
        }
        //myunputc(0);
        close(file);

        return true;
    }
    return false;
}


void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        if (isatty(0)){
		    Shell::prompt();
        }
        return;
    }

    // Print contents of Command data structure
    //print();

    // Add execution here
    // For every simple command fork a new process
    // Setup i/o redirection
    // and call exec
    int dupfin = dup(0);
	int dupfout = dup(1);
	int dupferr = dup(2);

    int fdin = 0;
	int	fdout = 0;
	int fderr = 0;

    // Checks for input pipe
    if (_inFile != NULL) {
		fdin = open(_inFile->c_str(), O_RDONLY);
	}
	else {
		fdin = dup(dupfin);
	}

    if (_errFile != NULL) {
        if (!_append) {
            fderr = open(_errFile->c_str(),  O_TRUNC | O_WRONLY | O_CREAT, 0666);
		}
		else {
			fderr = open(_errFile->c_str(), O_APPEND | O_CREAT | O_WRONLY, 0666);
		}
    }
    else {
		fderr = dup(dupferr);
	}
    dup2(fderr, 2);
	close(fderr);

    int ret;
    int temp = _simpleCommands.size();
    for (int i = 0; i < _simpleCommands.size(); i++) {

        if (isBuiltIn(i)) {
			return ;
		}

		dup2(fdin, 0);
		close(fdin);

		//Output
		if (i == _simpleCommands.size() - 1) {
			// Last simple command
			if (_outFile) {
				if (!_append) {
					fdout = open(_outFile->c_str(), O_TRUNC | O_WRONLY | O_CREAT, 0644);
				}
				else {
                    fdout = open(_outFile->c_str(), O_APPEND | O_CREAT | O_WRONLY, 0644);
				}
			}
			else {
				//Use default output
				fdout = dup(dupfout);
			}
		}
		else {	//Not last simple command->create pipe
			int fdpipe[2];
			pipe(fdpipe);
			fdout = fdpipe[1];
			fdin = fdpipe[0];
		}	//if/else

		//Redirect the output
		dup2(fdout, 1);
		close(fdout);

        ret = fork();
        if (ret == 0) {

            if (printenvCheck(i)) {
                return;
            }

            if (sourceCheck(i)) {
                return;
            }
            
            //Child
			size_t nArgs = _simpleCommands[i]->_arguments.size();
			char ** x = new char*[nArgs+1];
			for (size_t j = 0; j < nArgs; j++) {
                // Convert each argument to a string
				x[j] = const_cast<char*>(_simpleCommands[i]->_arguments[j]->c_str());

                // End each argument string with null byte
				x[j][strlen(_simpleCommands[i]->_arguments[j]->c_str())] = '\0';

			}
			x[nArgs] = NULL;
			execvp(x[0], x);
			perror("execvp");
			_exit(1);	//exit immeditately without messing with buffer
        }
        else if (ret < 0) {
            perror("fork");
            return;
        }
        // Parent Shell continue
    }

    dup2(dupfin, 0);
	dup2(dupfout, 1);
	dup2(dupferr, 2);
	close(dupfin);
	close(dupfout);
	close(dupferr);

    if (!_background) {
        waitpid(ret, NULL, 0);
    }


    // Clear to prepare for next command
    clear();

    // Print new prompt
    if (isatty(0)) {
        Shell::prompt();
    }
}

SimpleCommand * Command::_currentSimpleCommand;
