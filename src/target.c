/* Mikel Bresko
* target
* 11/22/18
*
* The purpose of this program is to build a linked list containing lists,
* in order to process makefile targets, rules, and dependencies,
* as well as applying I/O manipulations (append, truncate, redirect)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "target.h"
#include "arg_parse.h"

// CONSTANTS
target *head = NULL;
char *tg_name;

/* Expand
 * orig    The input string that may contain variables to be expanded
 * new     An output buffer that will contain a copy of orig with all
 *         variables expanded
 * newsize The size of the buffer pointed to by new.
 * returns 1 upon success or 0 upon failure.
 *
 * Example: "Hello, ${PLACE}" will expand to "Hello, World" when the environment
 * variable PLACE="World".
 */
int expand(char* orig, char* new, int newsize);

/*Replace Var
* var   The rest of the string char after (and including) the $ to parse environment variable
*           Returns the updated variable without braces or a 'fail' char* if no variable found
*/
char *replace_var(char *var);

/* Check & remove colon
* line the command line to change
* This function checks if there is a comma and replaces it with a space if there is
*/
char *checkrm_colon(char *line);

/* Assign Dependencies
* tg    The dependency we are checking for as a target
*       Once we have a linked list of rules and dependencies, this function recurses through the
*       dependencies to execute the rules in the correct order.
*           Calls processline at each recurrence to execute rules
*/
void assign_deps(target *tg);

/* Read Environment Variable
* line  string where an environment variable is being set to a value
*
*/
void read_env(char *line);

time_t getFileTime(char *path);

char *apply_redirect(int symbol, char *leftside, char *rightside);


char *checkrm_colon(char *line) {
  int index = 0;
  while (line[index] != '\0') {
    if (line[index] == ':') {
      line[index] = ' ';
    }
    index++;
  }
  return line;
}

// Creates and mallocs a new target node
target *create(char *name) {
    target *newtarget = malloc(sizeof(target));
    newtarget -> next = NULL;
    newtarget -> name = strdup(name);
    newtarget -> dep = NULL;
    newtarget -> rul = NULL;

    free(name);
    return newtarget;
}

// Returns a target node by name
target *find_target(char *name) {
    target *current = head;
    while (current != NULL) {
        if (strcmp((current->name),name) == 0) {
            return current;
        }
        if (current->next == NULL) {
            break;
        }
        current = current->next;
    }
    return NULL; // If we didn't find what we wanted
}

// Returns last target node in list
target *last_target() {
    target *tg = head;
    while (tg->next != NULL) {
        tg = tg->next;
    }
    return tg;
}

// Adds a node to the current list, and sets it to the last node's next
void add_target(char *name) {
    target *p;
    target *node = create(strdup(name));
    if (head == NULL) {
        head = node;

    }
    else {
        p = head;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = node;
    }
}

// Adds a dependency to a new node
// If dependency struct is null, set new dependency to be the first in the list
// Set dependency when we encounter null in the dependency list
void add_dep(char *name, char *dependency) {
    target *tg = find_target(name);
    if (tg->dep == NULL) {
        tg->dep = malloc(sizeof(dep));
        tg->dep->deps = strdup(dependency);
        tg->dep->next = NULL;
    }
    else {
        dep *current = tg -> dep;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = malloc(sizeof(dep));
        current->next->deps = strdup(dependency);
        current->next->next = NULL;
    }
}

// Adds a rule to a new node
// If rule struct is null, set new rule to be the first in the list
// Set rule when we encounter null in the rule list
// Calls expand
void add_rule(char *name, char *rule) {
    target *tg = find_target(name);

    struct rule_str_st *rul = tg->rul;

    int newsize = strlen(rule) + 20;
    char *new = malloc(sizeof(char)*(newsize+1));

    for (int i=0; i<newsize; i++) {
        new[i] = '\0';
    }
    int exp = expand(rule,new,newsize);

    if (exp == 0) {
        perror("Did not expand");
    }

    if (tg->rul == NULL) {
        tg->rul = malloc(sizeof(struct rule_str_st)+1);
        tg->rul->rules = strdup(new);
        tg->rul->next = NULL;
    }

    else {
        while (rul->next != NULL) {
            rul = rul->next;
        }
        rul->next = malloc(sizeof(struct rule_str_st)+1);
        rul->next->rules = strdup(new);
        rul->next->next = NULL;
    }
    free(new);
}

// Read target line
// Assigns variables from a target line (target and dependencies)
void read_target_line(char *line) {
    int argcount = 0;

    char *newline = checkrm_colon(line);
    char **argarray = arg_parse(newline,&argcount);

    tg_name = argarray[0];
    add_target(tg_name);

    for (int i=1; i<argcount; i++) {
        add_dep(tg_name,argarray[i]);
    }
    free(argarray);
}

// Read rules line
// Assigns the rule to the last target in the list
// (a rule in the file will always be for the most recent target)
void read_rules(char *line) {
    add_rule((last_target()->name), line);
}

// Execute arguments
// Finds targets sharing name to the command line args
// Executes the rules that belong to them
void exec_args(const char *argv[], int argc) {
    target *tg;

    for (int i=1; i<argc; i++) {
        tg = find_target((char *) argv[i]);
        if (tg != NULL) {
            assign_deps(tg);
        }
        else {
            printf("Target not found\n");
            exit(0);
        }
    }
    free(tg->name);

}

// Recursively assigns dependencies
void assign_deps(target *tg) {
    target *curr_tg = tg;
    int create = 0;
    time_t tg_time = getFileTime(curr_tg->name);
    time_t dep_time = 0;


    while (curr_tg->dep != NULL) {
        char *depname = (char *) curr_tg->dep->deps;

        if (find_target(depname) != NULL) {
            assign_deps(find_target(depname));
        }
            dep_time = getFileTime(depname);

            if ((dep_time >= tg_time) || (tg_time == 0)){
                create = 1;
            }
        curr_tg->dep = curr_tg->dep->next;
    }

    if ((create == 1) || ((dep_time == 0) && (tg_time == 0))) {
        while (curr_tg->rul != NULL) {
            processline(curr_tg->rul->rules);
            curr_tg->rul = curr_tg->rul->next;
        }
    }
}

int expand(char *orig, char *new, int newsize) {
    int i = 0;
    int check_i = 0;
    int new_i = 0;
    char *variable;
    while (orig[i] != '\0') {

        if (orig[i] == '$') { // If we find a $ sign
            if (orig[i+1] == '{') {
                variable = replace_var(&orig[i]); // Expanded variable

                for (int j=0; j<strlen(variable); j++) {
                    new[new_i] = variable[j];
                    new_i++;
                }
            }
            while (orig[check_i] != '}') {
                check_i++;
            }
            check_i++;

        }

        else {
            new[new_i] = orig[check_i];
            new_i++;
            check_i++;
        }
        i = check_i;
    }
  return 1;
}

// Replaces environment variable with its defined value
// Returns environment variable if found, NULL if not
char *replace_var(char *rest) {
  int i = 0;
  char *fail = "";
  char *newvar = strdup(rest);
  char *variable;
  if (newvar[1] != '{') {
    return fail;
  }
  // If symbol after $ is {, set index to first letter of variable
  else if (newvar[1] == '{') {
    i=2;

    // If encounter a null or space inside brackets, perror and return NULL
    while (newvar[i] != '}') {
      if (isspace(newvar[i]) || (newvar[i] == '\0')) {
        perror("Error in environment variable syntax");
        return fail;
      }
      i++;
    }
    newvar = &newvar[2];
    newvar[i-2] = '\0';
    variable = getenv(newvar);
  }
  return variable;
}

// Reads a line with environmental variable and assigns it to value
void read_env(char *line) {
    int i = 0;
    char *linecopy = strdup(line); // Need to isolate variable from '=' without destroying line
    while (linecopy[i] != '=') {
        i++;
    }
    linecopy[i] = '\0'; // Get pointer to char* name
    int j = i+1; // j = index after '\0'
    char *name = &linecopy[0];
    char *value = &linecopy[j];


    int set_env = setenv(name,value,1);
    if (set_env == -1) {
        perror("setenv");
    }
    free(linecopy);
}

time_t getFileTime(char *path) {
    struct stat stats;

    if (stat(path,&stats) == 0) {
        return stats.st_mtim.tv_sec;
    }

    return 0;
}

void append(char *filename) {
    int file = open(filename, O_APPEND | O_WRONLY | O_CREAT);

    if (file < 0) {
        perror("File");
    }
    dup2(file,1);
}

void trunc_file(char *filename) {
    int file = open(filename, O_TRUNC | O_CREAT | O_WRONLY);
    if (file < 0) {
        perror("File");
    }
    dup2(file,1);
}

void redirect_input(char *filename) {
    int file = open(filename, O_RDONLY | O_CREAT | O_WRONLY);
    if (file < 0) {
        perror("File");
    }    
    dup2(file,0);
}

char *apply_redirect(int symbol, char *leftside, char *rightside) {
    //Check_second = 0 if no second redirection
    switch(symbol) {
        case 1: {
            append(rightside); // return output to potentially be used in <
            return rightside;
        }
        case 2: {
            trunc_file(rightside);
            return rightside;
        }
        case 3: {
            redirect_input(rightside);
            return rightside;
        }
    }
    return NULL;
}
