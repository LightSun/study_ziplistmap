
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

static inline std::string decToBinaryString(int n, bool align32){
    int binaryNum[32];

    int i = 0;
    while (n > 0) {
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }
    std::string str;
    // printing binary array in reverse order
    for (int j = i - 1; j >= 0; j--){
        str += std::to_string(binaryNum[j]);
    }
    if(align32){
        int left = 32 - str.length();
        if(left > 0){
            str.reserve(str.length() + left);
            for(int i = 0 ;i < left; ++i){
               str = "0" + str;
            }
        }
    }
    return str;
}

}
