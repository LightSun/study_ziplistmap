
#include "ByteBufferIO.h"

using namespace h7;

static std::string decimalToBinary(int decimalNum, bool align32 = true) {
    std::string binaryStr;

    if (decimalNum < 0) {
        // 处理负数情况
        unsigned int uDecimalNum = static_cast<unsigned int>(decimalNum); // 将负数转换为无符号整数
        while (uDecimalNum > 0) {
            binaryStr = (uDecimalNum % 2 == 0 ? "0" : "1") + binaryStr;
            uDecimalNum /= 2;
        }
    } else {
        // 处理正数情况
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

void test_buffer_io(){
//    {
//        auto str1 = decimalToBinary(-1);
//        auto str2 = decimalToBinary(1);
//        LOGE("str1 = %s\n", str1.data());
//        LOGE("str2 = %s\n", str2.data());
//    }
//    {
//        auto str = h7::decToBinaryString(-1, true);
//        LOGE("str = %s\n", str.data());
//        int val = h7::binaryStr2Dec(str.data(), str.length());
//        LOGE("val = %d\n", val);
//    }
    {
        int number = -1141374976;
        int neg_number = ~number + 1; // Took 2's compliment of number
    }
    String str = "100111101111110101011010100000000111001100110011011101111001100110111011111110";
    ByteBufferOut bos;
    bos.putBinaryStringAsInts(str);

    auto str_coded = bos.bufferToString();

    ByteBufferIO bio(&str_coded);
    auto str2 = bio.getBinaryStringFromInts();
    LOGE("str  = %s\n", str.data());
    LOGE("str2 = %s\n", str2.data());
    ASSERT(str2 == str, "test_buffer_io >> must str2 == str.");
}
