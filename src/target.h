// Declaring target node
typedef struct target_st {
    char *name;
    struct dep_str_st *dep;
    struct rule_str_st *rul;
    struct target_st *next;
}target;

typedef struct dep_str_st {
    char *deps;
    struct dep_str_st *next;
}dep;

typedef struct rule_str_st {
    char *rules;
    struct rule_str_st *next;
    int argcount;
}rule;

target *create(char* name);
void add_target(char *name);
void add_dep(char *name, char *dependency);
void add_rule(char *name, char *rule);
target *find_target(char *name);
char *copystrarray(int size, char *arr);
void read_target_line(char *line);
void read_rules(char *line);
void exec_args(const char *argv[], int argc);
void processline(char *argarray);
void read_env(char *line);
int expand(char *orig, char *new, int newsize);
char *replace_var(char *var);
char *checkrm_colon(char *line);
void append(char *first, char *second);
void redirect_input(char *file);
void trunc_file(char *file, int check_sec);
char *apply_redirect(int symbol, char *leftside, char *rightside, int check_sec);
