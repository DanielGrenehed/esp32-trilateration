#include "str.h"


int str_length(const char* str) {
    int len = 0;
    while (str[len]!=0)len++;
    return len;
}

int str_next_non_space_char(const char* str) {
    int i = 0;
    while (str[i] != 0 && str[i] < 33) i++;
    return i;
}

int str_starts_with(const char* str, const char* start) {
    int i = str_next_non_space_char(str), j = 0;
    while (start[j] != 0) {
        if (str[i] == 0) return -1;
        if (str[i++] == start[j++]);
        else return -1;
    }
    return i;
}

int str_are_equal(const char* a, const char* b) {
    int i = 0;
    while (a[i] == b[i]) {
        if (a[i]==0) return i;
        else i++;
    }
    return -1;
}
#include "esp_log.h"
int str_are_equal_to(const char* a, const char* b, int max) {
    int i = 0;
    while (a[i] == b[i]) {
        if (i == max || a[i]==0) return i;
        else i++;
    }
    return -1;
}

void str_copy(const char* from, char* to, int num) {
    for (int i = 0; i<num;i++) {
        to[i] = from[i];
    }
}