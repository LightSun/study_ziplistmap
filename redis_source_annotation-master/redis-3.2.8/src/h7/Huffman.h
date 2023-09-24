#pragma once

#include <vector>
#include <unordered_map>
#include <queue>
#include <memory.h>
#include <string.h>

#include "ByteBufferIO.h"

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
    std::string rawCompress(const std::string& input, std::unordered_map<char, int>& out){
         auto str = _compress(input);
         out = m_frequencies;
         return str;
    }

    std::string rawDecompress(const std::string& input, const std::unordered_map<char, int>& out){
         m_frequencies = out;
         return _decompress(input);
    }

    std::string compress(const std::string& input){
        auto str = _compress(input);
        auto fstr = _encodeFrequencies();
        LOGE("compress >> str=%s\n", str.data());
        LOGE("compress >> fstr=%s\n", fstr.data());
        //
        ByteBufferOut bout;
        bout.putString16(fstr);
        bout.putBinaryStringAsInts(str);

        return bout.bufferToString();
    }
    std::string decompress(const std::string& input){
        ByteBufferIO bio((String*)&input);
        auto fstr = bio.getString16();
        _decodeFrequencies(fstr);
        auto str = bio.getBinaryStringFromInts();
        LOGE("decompress >> str=%s\n", str.data());
        LOGE("decompress >> fstr=%s\n", fstr.data());
        return _decompress(str);
    }

private:
    void _decodeFrequencies(CString input){
        m_frequencies.clear();
        ByteBufferIO bio((String*)&input);
        auto size = bio.getInt();
        for(int i = 0 ; i < size ; ++i){
            char key = bio.getByte();
            int val = bio.getInt();
            m_frequencies[key] = val;
        }
    }
    std::string _encodeFrequencies(){
        const int size = m_frequencies.size();
        //ASSERT(size <= 4, "have special char. only permit ATCG.");
        //size, data
        ByteBufferOut bout;
        bout.prepareBuffer(64);
        bout.putInt(size);
        //
        auto it = m_frequencies.begin();
        for(; it != m_frequencies.end(); ++it){
            bout.putByte(it->first);
            bout.putInt(it->second);
        }
        return bout.bufferToString();
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
