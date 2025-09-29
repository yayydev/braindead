#ifndef BRAINDHEAD_H
#define BRAINDHEAD_H

#include <stddef.h>

#define VERSION "0.0.1-beta"

extern int dev_mode;

void print_help(void);
void print_version(void);

char *read_file(const char *path, size_t *len);
int write_file(const char *path, const char *data);

int is_bf_char(char c);

int check_errors(const char *code, size_t len, const char *filename);
void emit_error_variation(const char *filename, int line, int col, const char *token);

char *sanitize_filename(const char *in);
void mkdir_p(const char *path);

char *make_c_from_bf(const char *code, size_t len, const char *srcname);

int run_command(const char *cmd);
char *find_compiler(void);

int rand_range(int a, int b);

#endif
