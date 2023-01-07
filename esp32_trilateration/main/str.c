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

int str_are_equal_to(const char* a, const char* b, int max) {
    int i = 0;
    while (*(a+i) == *(b+i) && i!=max) i++;
    if (i==max) return max;
    return -1;
}

void str_copy(const char* from, char* to, int num) {
    for (int i = 0; i<num;i++) {
        to[i] = from[i];
    }
}

static int is_non_displayable_char(char chr) {
    return chr<(' '+1);
}

static int is_numeric(char chr) {
    if (chr >= '0' && chr <= '9') return 1;
    return 0;
}

static int is_hexadec(char chr) {
    if (is_numeric(chr)) return 1;
    if (chr >= 'A' && chr <= 'F') return 1;
    if (chr >= 'a' && chr <= 'f') return 1;
    return 0;
}

static int get_next_hex_nibble_char(const char* str) {
    int i = 0;
    while (!is_hexadec(str[i]) && str[i] != 0) i++;
    return i;
}

static int get_hex_nibble(char chr) {
    if (is_numeric(chr)) return chr-'0';
    if (chr >= 'A' && chr <= 'F') return chr-'A'+10;
    if (chr >= 'a' && chr <= 'f') return chr-'a'+10;
    return -1;
}

int str_parse_hex(const char* str, int num_nibbles, char* buffer) {
    int pos = 0, nibble = 0;
    for (int i = 0; i < num_nibbles; i++) {
        pos += get_next_hex_nibble_char(str+pos);
        if ((nibble = get_hex_nibble(str[pos])) == -1) return i;
        if (i%2==1) buffer[i/2] = buffer[i/2] << 4;
        else buffer[i/2]=0;
        buffer[i/2] += nibble;
        pos++;
    }
    return num_nibbles;
}

int str_parse_int(const char* str) {
    int out = 0, i = 0, is_num = 0, negative = 0; 
    while ((is_num = is_numeric(str[i])) || (str[i]!=0 && is_non_displayable_char(str[i])) || str[i]=='-') {
        if (is_num) out = (str[i]-'0') + (out*10);
        else if (str[i]=='-') negative = 1;
        i++;
    }
    return negative? -out:out;
}