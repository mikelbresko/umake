// Mikel Bresko
// umake
// 11/22/18

/* The purpose of this program is to parse through a file and
*  execute commands to build a makefile (if needed) or to redirect
* inputs and outputs.
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include "arg_parse.h"
#include "target.h"

/* Process Line
 * line   The command line to execute.
 * This function interprets line as a command line.  It creates a new child
 * process to execute the line and waits for that process to complete.
 */
void processline(char *argarray);

// Checks for an equals sign in a line
// Returns true/false
// Checks for a pound sign in a line
// Returns true/false
bool check_pound(char *line);

// If there is a pound sign, truncate line up to that point
char *cut_line(char *line);

int find_sym(char *line);

char *find_left(char *line);
char *find_right(char *line);

bool check_equals(char *line);

char *detach_line(char *line);


/* Main entry point.
 * argc    A count of command-line arguments
 * argv    The command-line argument values
 *3
 * Micro-make (umake) reads from the uMakefile in the current working
 * directory.  The file is read one line at a time.  Lines with a leading tab
 * character ('\t') are interpreted as a command and passed to processline minus
 * the leading tab.
 */
int main(int argc, const char* argv[]) {
  FILE* makefile = fopen("./uMakefile", "r");
  if (makefile == 0) {
    perror("makefile");
    return EXIT_FAILURE;
  }

  size_t  bufsize = 0;
  char*   line    = NULL;
  ssize_t linelen = getline(&line, &bufsize, makefile);

  while(-1 != linelen) {
    if(line[linelen-1]=='\n') {
      linelen -= 1;
      line[linelen] = '\0';
    }
    // Check for comments
    if (line[0] != '#') {
      if (check_pound(line)) {
        line = cut_line(line);
      }
      // If tabbed line
      if (isspace(line[0]) && (line[0] != '\n') && line[0] != '\r') {
        if (find_sym(line) == 4) {
          read_rules(&line[1]);
        }
        // If there's an I/O manipulation
        else {
          int i = 0;
          while ((line[i] != '>') && (line[i] != '<')) {
            i++;
          }
          int sym_index = i;
          int symbol = find_sym(line);

          // Make copies of line 
          // in order to return left and right args of the redirection
          char *linecopy_left = strdup(line);
          char *linecopy_right = strdup(line);

          char *leftside = find_left(linecopy_left);
          char *rightside = find_right(linecopy_right);

          // Variables to check for second I/O manipulation
          char *sec_right;
          int check_sec = 0;
          int sec_sym = 4;

          // If the manipulation is a redirect
          if (symbol == 3) {
            char *linecopy = line;
            sec_sym = find_sym(detach_line(linecopy));
    
            // If there's a second I/O manipulation
            if (sec_sym != 4) {
              check_sec = 1;
              sec_right = find_right(detach_line(linecopy));
            }
          }
          // Set up for first I/O manipulation and null terminate for processline
          char *redir = apply_redirect(symbol,leftside,rightside);
          line[sym_index] = '\0';
          
          // If second I/O manip, link the result of the first one into it
          if (check_sec == 1) {
            apply_redirect(sec_sym,redir,sec_right);
          }
          processline(line);
          free(linecopy_left);
          free(linecopy_right);
        }
      }

      else if (!isspace(line[0])) {
        if (check_equals(line)) {
          read_env(line);
        }
        else {
          read_target_line(&line[0]);
        }
      }
    }

    linelen = getline(&line, &bufsize, makefile);
  }
  exec_args(argv,argc);
  free(line);
  fclose(makefile);
  return EXIT_SUCCESS;
}


void processline (char *line) {
  int argcount = 0;
  char *linecopy = strdup(line);
  char **argarray = arg_parse(&linecopy[1], &argcount);
  argarray[argcount] = NULL;

  // Ignores blank lines
  if (argcount != 0) {
    const pid_t cpid = fork();
    switch(cpid) {

    case -1: {
      perror("fork");
      break;
    }

    case 0: {
      execvp(argarray[0], argarray);
      perror("execvp");
      exit(EXIT_FAILURE);
      break;
    }

    default: {
      int   status;
      const pid_t pid = wait(&status);
      if(-1 == pid) {
        perror("wait");
      }
      else if (pid != cpid) {
        fprintf(stderr, "wait: expected process %d, but waited for process %d",
                cpid, pid);
      }
      break;
    }
  }
  }
  free(argarray);
  free(linecopy);
}

// Checks for an equals sign in a char *line
bool check_equals(char *line) {
  int i = 0;
  while ((line[i] != '\r') && (line[i] != '\n') && (line[i] != '\0')) {
    if (line[i] == '=') {
      return true;
    }
    i++;
  }
  return false;
}

// Checks for a pound sign in a char * line
bool check_pound(char *line) {
  int i = 0;
  while ((line[i] != '\r') && (line[i] != '\n') && (line[i] != '\0')) {
    if (line[i] == '#') {
      return true;
    }
    i++;
  }
  return false;
}

// Cuts commented lines accordingly
char *cut_line(char *line) {
  int i = 0;
  char *newline;

  while (line[i] != '#') {
    i++;
  }
  line[i] = '\0';
  newline = &line[0];
  return newline;
}

// Returns an int 1-4 depending on symbol or lack of in line
// 1 for >>
// 2 for >
// 3 for <
// 4 if none present
int find_sym(char *line) {
  int i;
  for (i=0; i<strlen(line); i++) {
    if ((line[i] == '>') || (line[i] == '<')) {
      break;
    }
  }

  int symbol;
  if (line[i] == '>') {
    if (line[i+1] == '>') {
      symbol = 1;
    }
    else {
      symbol = 2;
    }
  }
  else if (line[i] == '<') {
    symbol = 3;
  }
  else {
    symbol = 4;
  }
  switch(symbol) {
    case 1: {
      return symbol;
    }
    case 2: {
      return symbol;
    }
    case 3: {
      return symbol;
    }
    case 4: {
      return symbol;
    }
  }
  return symbol;
}

// Find argument to left of symbol
char *find_left(char *line) {
  int i = 0;

  while ((line[i] != '>') && (line[i] != '<')) {
    i++;
  }
  line[i] = '\0';
  char *leftside = &line[0];

  return leftside;
}

// Find argument to right of symbol
char *find_right(char *line) {
  int argcount = 0;
  int i;
  int brk = 0;
  char **argarray = arg_parse(&line[1], &argcount);

  for (i=0; i<argcount; i++) {
    char *ch = argarray[i];

    for (int j=0; j<strlen(ch); j++) {
      if ((ch[j] == '>') || (ch[j] == '<')) {
        brk = 1;
      }
    }
    if (brk != 0) {
      break;
    }
  }
  char *rightside = argarray[i+1];
  free(argarray);
  return rightside;
}

char *detach_line(char *line) {
  int i = 0;
  char *newline;

  while ((line[i] != '<') && (line[i] != '>')) {
    i++;
  }
  if (line[i+1] == '>') {
    newline = &line[i+3];
    return newline;
  }
  else {
    newline = &line[i+2];
    return newline;
  }
}
