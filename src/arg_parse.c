/* Mikel Bresko
*  arg_parse
* 11/22/2018
*
* The purpose of this program is to separate a string of characters
* into a char** array, delimited with null characters
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include "arg_parse.h"


// Parses through a line of chars, counts the arguments,
// and deliminates the separate arguments with null characters
char **arg_parse(char *line, int *argcp) {

  int index = 0;
  int argcount = 0;

 // Until we reach the null character at the end of the string
  // If, on a char, the next index holds a space/null, counts argument
  // If, on a space, the previous index holds a char, replace current index with null
  while (line[index] != '\0') {
    if ((!isspace(line[index]) && (line[index] != '\0')) && (isspace(line[index+1]) || (line[index+1]) == '\0')) {
      argcount++;
    }
    if (isspace(line[index]) && (!isspace(line[index-1]) && (line[index-1] != '\0'))) {
      line[index] = '\0';
    }
    index++;
  }

  // Update argcp with our argcount
  // Malloc for pointer
  *argcp = argcount;
  char **argarray = malloc((sizeof(char*) * (argcount+1)));

 // Set pointers to args
  return argsetter(argarray, line, argcount);
}

// Tracks number of null chars in the string while
// assigning pointers to first letter of each arg
char** argsetter(char **argarray, char *line, int argcount) {
  int i = 0;
  int j = findfirst(line);
  int nullcount = 0;

  while (nullcount != argcount) {
    if (line[i+1] == '\0') {
      argarray[nullcount] = &line[j];
      nullcount++;
      if (nullcount == argcount) {
        break;
      }
      j = setnextindex(line, i);
    }
    i++;
  }

  return argarray;
}
// Sets index to first letter of next available command
int setnextindex(char *line, int index) {
  // Check for extra spaces between null char and next arg
  while (isspace(line[index+1]) || (line[index+1] == '\0')) {
    index++;
  }
  index++;
  return index;
}

// Helper function to argsetter
int findfirst(char *line) {
  int index = 0;
  while (isspace(line[index])) {
    index++;
  }
  return index;
}

