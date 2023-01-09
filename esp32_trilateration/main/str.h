#ifndef TRILAT_STR_H
#define TRILAT_STR_H

int str_length(const char*);

int str_next_non_space_char(const char*);

int str_starts_with(const char*, const char*);

int str_are_equal(const char*, const char*);

int str_are_equal_to(const char*, const char*, int);

void str_copy(const char* from, char* to, int num);

int str_parse_hex(const char*, int, char* );

int str_parse_int(const char*);

#endif /* TRILAT_STR_H */