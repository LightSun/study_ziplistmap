
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

static inline std::string decToBinaryString(int decimalNum, bool align32){
    std::string binaryStr;

    if (decimalNum < 0) {
        // negative
        unsigned int uDecimalNum = static_cast<unsigned int>(decimalNum); //to unsigned int
        while (uDecimalNum > 0) {
            binaryStr = (uDecimalNum % 2 == 0 ? "0" : "1") + binaryStr;
            uDecimalNum /= 2;
        }
    } else {
        // positive
        if (decimalNum == 0) {
            binaryStr = "0";
        } else {
            while (decimalNum > 0) {
                binaryStr = (decimalNum % 2 == 0 ? "0" : "1") + binaryStr;
                decimalNum /= 2;
            }
        }
    }
    if(align32){
        int left = 32 - binaryStr.length();
        if(left > 0){
            binaryStr.reserve(binaryStr.length() + left);
            for(int i = 0 ;i < left; ++i){
               binaryStr = "0" + binaryStr;
            }
        }
    }
    return binaryStr;
}

}
