#pragma once

#include <vector>
#include <unordered_map>
#include <queue>
#include <memory.h>
#include <string.h>

#include "common.h"
#include "c_common.h"
#include "_utils.h"

namespace h7 {

class Huffman
{
public:
    struct HuffmanNode {
        char character;
        int frequency;
        HuffmanNode* left;
        HuffmanNode* right;

        HuffmanNode(char c, int freq) : character(c), frequency(freq),
            left(nullptr), right(nullptr) {}
    };
    struct CompareHuffmanNodes {
        bool operator()(HuffmanNode* lhs, HuffmanNode* rhs) const {
            return lhs->frequency > rhs->frequency;
        }
    };

public:
    std::string compress(const std::string& input){
        auto str = _compress(input);
        const uint32 str_len = str.length();

        auto fstr = _encodeFrequencies();
        //encode binary to oct
        std::vector<int> vec_str;
        {
            //align 32
            int left = 32 - str_len % 32;
            str.reserve(str_len + left);
            for(int i = 0 ; i < left ; ++i){
                str += "0";
            }
            int size = str_len >> 5; // x/32
            auto _data = str.data();
            vec_str.resize(size);
            for(int i = 0 ; i < size ; ++i){
                vec_str[i] = h7::binaryStr2Dec(_data + (i << 5), 32);
            }
        }
        //str_len + vec_str.length + int_datas
        std::vector<char> str_data;
        str_data.resize(sizeof(uint32) + sizeof(uint32)
                        + vec_str.size() *sizeof(uint32));
        {
            //set data
            uint32 offset = 0;
            memcpy(str_data.data(), &str_len, sizeof (uint32));
            offset += sizeof (uint32);
            uint32 vec_len = vec_str.size();
            memcpy(str_data.data() + offset, &vec_len, sizeof (uint32));
            offset += sizeof (uint32);
            //
            memcpy(str_data.data() + offset, vec_str.data(),
                   sizeof(uint32) * vec_len);
        }
        return fstr + String(str_data.data(), str_data.size());
    }
    std::string decompress(const std::string& input){
        //TODO
    }

private:
    std::string _encodeFrequencies(){
        const int size = m_frequencies.size();
        ASSERT(size <= 4, "have special char. only permit ATCG.");
        //size, data
        std::string str;
        str.resize(sizeof (int) + size * (sizeof (char) + sizeof (int)));
        char* ptr_data = str.data();
        memcpy(ptr_data, &size, sizeof (int));
        //
        size_t offset = sizeof (int);
        auto it = m_frequencies.begin();
        for(; it != m_frequencies.end(); ++it){
            char key = it->first;
            int val = it->second;
            memcpy(ptr_data + offset, &key, sizeof (char));
            offset += sizeof (char);

            memcpy(ptr_data + offset, &val, sizeof (int));
            offset += sizeof (int);
        }
        return str;
    }
    std::string _compress(const std::string& input) {
        if (input.empty()) {
            return "";
        }
        m_frequencies.clear();

        for (char c : input) {
            m_frequencies[c]++;
        }

        HuffmanNode* root = buildHuffmanTree(m_frequencies);

        // Generate Huffman codes for each character
        std::unordered_map<char, std::string> codes;
        generateHuffmanCodes(root, "", codes);

        // Encode the input using Huffman codes
        std::string compressed;
        compressed.reserve(input.length() * 2);
        for (char c : input) {
            compressed += codes[c];
        }
        clear();
        return compressed;
    }
    std::string _decompress(const std::string& input){
         HuffmanNode* root = buildHuffmanTree(m_frequencies);
         auto ret = decodeHuffman(input, root);
         clear();
         return ret;
    }
    void clear(){
        for(auto& n: m_nodes){
            delete n;
        }
        m_nodes.clear();
    }
    std::string decodeHuffman(const std::string& compressed, HuffmanNode* root) {
        if (compressed.empty() || !root) {
            return "";
        }
        std::string decoded;
        decoded.reserve(compressed.length()/2);
        HuffmanNode* current = root;
        for (char c : compressed) {
            if (c == '0') {
                current = current->left;
            } else if (c == '1') {
                current = current->right;
            }
            if (current->character != '\0') {
                decoded += current->character;
                current = root; // Reset to the root for the next character
            }
        }
        return decoded;
    }
    HuffmanNode* buildHuffmanTree(const std::unordered_map<char, int>& frequencies) {
        std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, CompareHuffmanNodes> pq;

        // Create a leaf node for each character and add it to the priority queue
        for (const auto& pair : frequencies) {
            pq.push(newNode(pair.first, pair.second));
        }
        // Build the Huffman tree
        while (pq.size() > 1) {
            HuffmanNode* left = pq.top();
            pq.pop();
            HuffmanNode* right = pq.top();
            pq.pop();

            HuffmanNode* internalNode = newNode('\0', left->frequency + right->frequency);
            internalNode->left = left;
            internalNode->right = right;
            pq.push(internalNode);
        }
        return pq.top();
    }
    void generateHuffmanCodes(HuffmanNode* root, const std::string& code,
                              std::unordered_map<char, std::string>& codes) {
        if (!root) {
            return;
        }
        if (root->character != '\0') {
            codes[root->character] = code;
        }
        generateHuffmanCodes(root->left, code + "0", codes);
        generateHuffmanCodes(root->right, code + "1", codes);
    }
    HuffmanNode* newNode(char c, int freq){
        HuffmanNode* ret = new HuffmanNode(c, freq);
        m_nodes.push_back(ret);
        return ret;
    }

private:
    std::vector<HuffmanNode*> m_nodes;
    std::unordered_map<char, int> m_frequencies;
};

}
