#ifndef TRILAT_STORE_H
#define TRILAT_STORE_H

void store_init();
void store_deinit();
void store_erase_all();
int store_get_str(const char*, char*, int);
int store_set_str(const char*, const char*);

#endif /* TRILAT_STORE_H */