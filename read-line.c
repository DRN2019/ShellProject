/*
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char ** history;
int history_max = 10;
int history_length = 0;
int pos;

void add_history( const char *input) {
  char * line = (char *) malloc(strlen(input) * sizeof(char));
  strcpy(line, input);
	// realloc if necessary
	if ( history_length == history_max + 1) {
		history_max *= 2;
		history = (char**) realloc (history, history_max * sizeof(char*));
	}

	// add the new line
	line[ strlen(line) ] = '\0'; // remove newline at the end
	//printf("\tadding line: \"%s\"", line);
	history[ history_length ] = line;
	history_length++;
	history_index++; // index tracks most recently added line

  free(line);
  line = NULL;

  if (history_index >= history_max) {
    history_index = 0;
  }
}

void init_hist() {
	history = (char**) malloc( history_max * sizeof(char*) );
	const char * line = "";
	add_history( line );
}

void backspace(int n) {
	int i;
	char ch = 8;
	for (i = 0; i < n; i++) {
		write( 1, &ch, 1 );
	}
}

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  pos = 0;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch == 1) { // Ctrl A
      backspace(pos);
      pos = 0;
    }
    else if (ch == 4) { // Ctrl D
      // Go forward 1 char
      if (pos < line_length) {
        int i;

        for (i = pos; i < line_length - 1; i++) {
          line_buffer[i] = line_buffer[i + 1];
          
        }
        line_buffer[line_length - 1] = '\0';
        
        // Go back one character
        //clear screen, ready to reprint
        backspace(pos);

        // delete character
        for (i = 0; i < line_length; i++) {
          ch = ' ';
          write(1, &ch, 1);

        }
        // cursor goes to the very beginning
        backspace(line_length);
        line_length--;

        //print the line that already modified
        for (i = 0; i < line_length; i++) {
          ch = line_buffer[i];
          write(1, &ch, 1);

        }

        backspace(line_length - pos);
      }
    } 
    else if (ch == 5) { // Ctrl E
      if (line_length == 0) {
        continue;
      }
      if (pos == line_length) {
        continue;
      }

      pos = line_length;

    }

    else if (ch >= 32 && ch < 127) {
      // It is a printable character. 

      if (line_length != pos) {
        // In the middle of a line
				char * tempStr = (char*) malloc(sizeof(char) * MAX_BUFFER_LINE);
				int i;
				// copy all the characters after where we're at
				for ( i = 0; i <= MAX_BUFFER_LINE - 1; i++) {

					if (line_buffer[pos + i] == '\0' ) {
						// If null char, we finish copying
						break;
					}
					tempStr[i] = line_buffer[pos + i];
				}

				// write the character we want to add
				write (1, &ch, 1);

				// If max number of character reached return.
				if (line_length==MAX_BUFFER_LINE-2) break; 
				
				// add char to buffer.
				line_buffer[pos]=ch;
				line_length++;
				pos++;

				// print all the rest of the characters afterwards
				int added_Chars = 0;
				for (i = 0; i < MAX_BUFFER_LINE; i++) {
					added_Chars+=1;
					write(1, &tempStr[i], 1);
					if (line_buffer[pos + i] == '\0' ) {
						// if we see the null char, we've gotten it all
						break;
					}
				}
				// go back to where we were
				backspace(added_Chars);
      }
      else {
        // Do echo
        write(1,&ch,1);

        // If max number of character reached return.
        if (line_length==MAX_BUFFER_LINE-2) break; 

        // add char to buffer.
        line_buffer[line_length]=ch;
        line_length++;
        pos++;
      }
    }
    else if (ch==10) {
      // <Enter> was typed. Return line
      // TODO: Add to history
      // if ( strlen(line_buffer) != 0 ) {
			// 	add_history(line_buffer);
			// }
      
      // Print newline
      write(1,&ch,1);

      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == 8) {
      // <backspace> was typed. Remove previous character read.

      if (pos <= 0) {
				continue;
			}

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      // Write a space to erase the last character read
      ch = ' ';
      write(1,&ch,1);

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      // Remove one character from buffer
      line_length--;
      pos--;
    }
    else if (ch == 127) {
      // <backspace> was typed. Remove previous character read.

      // Check if at start of command
      if (pos <= 0) {
				continue;
			}

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      // Write a space to erase the last character read
      ch = ' ';
      write(1,&ch,1);

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      // Remove one character from buffer
      line_length--;
      pos--;
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
        // Up arrow. Print next line in history.

        // Erase old line
        // Print backspaces
        int i = 0;
        for (i =0; i < line_length; i++) {
          ch = 8;
          write(1,&ch,1);
        }

        // Print spaces on top
        for (i =0; i < line_length; i++) {
          ch = ' ';
          write(1,&ch,1);
        }

        // Print backspaces
        for (i =0; i < line_length; i++) {
          ch = 8;
          write(1,&ch,1);
        }	

        // Copy line from history
        strcpy(line_buffer, history[history_index]);
        line_length = strlen(line_buffer);
        history_index=(history_index+1)%history_length;

        // echo line
        write(1, line_buffer, line_length);
      }

      if (ch1==91 && ch2==66) {
        // Down arrow
      }

      if (ch1==91 && ch2==68) {
        // Left arrow
				if (pos <= 0) {
					continue;
				}
				if (pos > 0) {
					pos--;
					ch = 8;
          write( 1, &ch, 1 );
				}
      }

      if (ch1==91 && ch2==67) {
        // Right arrow
        if (pos < line_length) {

					// grab current char
					char c = line_buffer[pos];

					// print grabbed char
					write( 1, &c, 1);

					pos++;
				}
      }
      
    }

  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  return line_buffer;
}

