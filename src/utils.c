#include "utils.h"

int ctoi(char c) {
    int ret = c - '0';
    if (ret < 0 || ret > 9) return 0;
    return ret;
}

char itoc(int i) {
    return i + '0';
}
