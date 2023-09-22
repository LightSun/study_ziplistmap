
#include <string>
#include <cmath>

namespace h7 {

// '101100101' to int
static inline int binaryStr2Dec(const char* str, int size)
{
    int ret = 0;
    for (int i = 0; i < size; ++i) {
        if (str[i] == '1') {
            ret += pow(2.0, size - i - 1);
        }
    }
    return ret;
}

}
