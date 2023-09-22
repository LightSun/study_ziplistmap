#include <iostream>
#include <string>
#include <unordered_map>
#include <queue>

// Define a structure for a Huffman tree node
struct HuffmanNode {
    char character;
    int frequency;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(char c, int freq) : character(c), frequency(freq),
        left(nullptr), right(nullptr) {}
};

// Define a comparison function for Huffman nodes (used in priority queue)
struct CompareHuffmanNodes {
    bool operator()(HuffmanNode* lhs, HuffmanNode* rhs) const {
        return lhs->frequency > rhs->frequency;
    }
};

// Function to build the Huffman tree
HuffmanNode* buildHuffmanTree(const std::unordered_map<char, int>& frequencies) {
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, CompareHuffmanNodes> pq;

    // Create a leaf node for each character and add it to the priority queue
    for (const auto& pair : frequencies) {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }

    // Build the Huffman tree
    while (pq.size() > 1) {
        HuffmanNode* left = pq.top();
        pq.pop();
        HuffmanNode* right = pq.top();
        pq.pop();

        HuffmanNode* internalNode = new HuffmanNode('\0', left->frequency + right->frequency);
        internalNode->left = left;
        internalNode->right = right;
        pq.push(internalNode);
    }

    return pq.top();
}

// Function to generate Huffman codes for characters
void generateHuffmanCodes(HuffmanNode* root, const std::string& code, std::unordered_map<char, std::string>& codes) {
    if (!root) {
        return;
    }

    if (root->character != '\0') {
        codes[root->character] = code;
    }

    generateHuffmanCodes(root->left, code + "0", codes);
    generateHuffmanCodes(root->right, code + "1", codes);
}

// Function to compress a string using Huffman coding
std::string compressHuffman(const std::string& input,
                            std::unordered_map<char, int>& frequencies) {
    if (input.empty()) {
        return "";
    }

    // Count the frequency of each character in the input
    //std::unordered_map<char, int> frequencies;
    for (char c : input) {
        frequencies[c]++;
    }

    // Build the Huffman tree
    HuffmanNode* root = buildHuffmanTree(frequencies);

    // Generate Huffman codes for each character
    std::unordered_map<char, std::string> codes;
    generateHuffmanCodes(root, "", codes);

    // Encode the input using Huffman codes
    std::string compressed;
    for (char c : input) {
        compressed += codes[c];
    }

    // Clean up the Huffman tree (optional, but recommended)
    // Implement a function to delete the tree nodes
    return compressed;
}
// Function to decode a Huffman-encoded string
std::string decodeHuffman(const std::string& compressed, HuffmanNode* root) {
    if (compressed.empty() || !root) {
        return "";
    }

    std::string decoded;
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

void test_dna_compress() {
    std::unordered_map<char, int> frequencies;
    //std::string input = "ACTGACTGACTGACTGAGACTGACTGACTGACTGACTGACTGACTGGACTGACTGACTG";
    std::string input = "NTTGCAAAAAATTTCTCTCATTCTGTAGGTTGCCTGTTCACTCTGATGATAGTTTGTTTTGG";
    std::string compressed = compressHuffman(input, frequencies);

    std::cout << "------------ encode --------- " << std::endl;
    std::cout << "Original String: " << input << std::endl;
    std::cout << "Compressed String: " << compressed << std::endl;
    std::cout << "Compressed String len: " << compressed.length() << std::endl;
    //----------
    std::cout << "------------ deocde --------- " << std::endl;
    {
       HuffmanNode* root = buildHuffmanTree(frequencies); // You need a function to build the Huffman tree

       // Compressed string obtained from encoding
       //std::string compressed = "001101110";

       // Decode the compressed string
       std::string decoded = decodeHuffman(compressed, root);

       std::cout << "Compressed String: " << compressed << std::endl;
       std::cout << "Decoded String: " << decoded << std::endl;
       int len = decoded.length() % 32 != 0 ? (decoded.length() / 32 + 1)
                                             : decoded.length() / 32;
       std::cout << "Decoded String(int count): " << len << std::endl;
       std::cout << "decode ok = " << (decoded == input) << std::endl;
    }
}
