#include <algorithm> // std::lower_bound
#include <cassert>
#include <cstring> // memmove
#include <stdint.h>
#include <stdio.h>
#include <utility> // std::pair

template <typename T, typename T2>
inline constexpr size_t align(T2 unaligned)
{
    constexpr size_t alignMask = (alignof(T) - 1);
    static_assert((alignMask & alignof(T)) == 0); // is pow of 2
    return (((size_t)unaligned + alignMask) & ~alignMask);
}

class TrieNode;

using NumType = uint8_t;
using KeyType = uint8_t;

class TrieNode {
    uint8_t* data;

public:
    bool valid() const { return data; }

    inline void getNumKeyNodes(NumType& outNum, KeyType*& outKeys, TrieNode*& outNodes) const
    {
        outNum = data ? *(NumType*)data : 0;
        outKeys = (KeyType*)align<KeyType>((NumType*)data + 1);
        outNodes = (TrieNode*)align<TrieNode>(outKeys + outNum);
    }

    static inline TrieNode* getNodeStartForNewNum(void* dataFrom, NumType newNum)
    {
        auto keys = (KeyType*)align<KeyType>((NumType*)dataFrom + 1);
        return (TrieNode*)align<TrieNode>(keys + newNum);
    }

    struct SoAIterator {
        KeyType* p1;
        TrieNode* p2;

        std::pair<KeyType&, TrieNode&> operator*() const { return { *p1, *p2 }; }
        bool operator!=(const SoAIterator& other) const { return p1 != other.p1; }
        SoAIterator& operator++()
        {
            ++p1, ++p2;
            return *this;
        }
    };

    auto begin() const
    {
        NumType num;
        KeyType* keys;
        TrieNode* nodes;
        getNumKeyNodes(num, keys, nodes);
        return SoAIterator { keys, nodes };
    }

    auto end() const
    {
        NumType num;
        KeyType* keys;
        TrieNode* nodes;
        getNumKeyNodes(num, keys, nodes);
        return SoAIterator { keys + num, nodes + num };
    }

    TrieNode find(const KeyType& key) const
    {
        NumType num;
        KeyType* keys;
        TrieNode* nodes;
        getNumKeyNodes(num, keys, nodes);

        if (num == 0)
            return {};

        auto keyIt = std::lower_bound(keys, keys + num, key);
        if (keyIt == keys + num || *keyIt != key)
            return *(nodes + std::distance(keys, keyIt));

        return {};
    }

    TrieNode& insert(const KeyType& key)
    {
        NumType num;
        KeyType* keyStart;
        TrieNode* nodeStart;
        getNumKeyNodes(num, keyStart, nodeStart);

        auto keyIt = std::lower_bound(keyStart, keyStart + num, key);
        size_t _pos = std::distance(keyStart, keyIt);

        if (keyIt == keyStart + num || *keyIt != key) {
            TrieNode* nodeStartNew = getNodeStartForNewNum(data, num + 1);
            TrieNode* nodeEndNew = nodeStartNew + num + 1;

            uint8_t* dataOld = data;
            data = (uint8_t*)realloc(dataOld, (size_t)nodeEndNew - (size_t)data);

            if (dataOld == nullptr) {
                memset(data, 0, (size_t)nodeEndNew);
                printf("Initial allocation\n");
            }

            std::ptrdiff_t dataOffset = data - dataOld;
            (uint8_t*&)keyStart += dataOffset;
            (uint8_t*&)nodeStart += dataOffset;
            (uint8_t*&)nodeStartNew += dataOffset;
            (uint8_t*&)nodeEndNew += dataOffset;

            if (nodeStartNew != nodeStart) {
                memmove(nodeStartNew, nodeStart, num * sizeof(*nodeStart));
                // *nodes = {};
            }

            if (_pos < num) {
                memmove(&keyStart[_pos + 1], &keyStart[_pos], (num - _pos) * sizeof(*keyStart));
                memmove(&nodeStart[_pos + 1], &nodeStart[_pos], (num - _pos) * sizeof(*nodeStart));
            }

            keyStart[_pos] = key;
            nodeStart[_pos] = TrieNode {};
            *(NumType*)data += 1; // increment size
        }
        return nodeStart[_pos];
    }

    void print(int offset) const
    {
        putchar('\n');
        offset++;
        for (const auto& [c, node] : *this) {
            for (int i = 0; i < offset; i++)
                putchar(' ');
            printf("%c", c);

            node.print(offset);
        }
    }
    void clear()
    {
        if (data) {
            free(data);
            printf("Deallocation\n");
        }
    }
};

class Trie {
    TrieNode root {};
    void clear(TrieNode& n)
    {
        for (const auto& [k, v] : n)
            clear(v);
        n.clear();
    }

public:
    ~Trie() { clear(root); };

    void insert(const char* word)
    {
        TrieNode* node = &root;
        while (char c = *word) {
            auto& foundNode = node->insert(c);
            if (foundNode.valid())
                node = &foundNode;
            else {
                foundNode = TrieNode {};
                node = &foundNode;
            }

            word++;
        }
        // node->op = op;
    }

    void print() { root.print(0); }

    int match(const char* text, int start) const
    {
        TrieNode node = root;
        int len = 0;

        for (int i = start; text[i]; ++i) {
            TrieNode foundNode = node.find(text[i]);
            if (!foundNode.valid())
                break;

            node = foundNode;
            ++len;
        }

        return 0;
    }
};

int main()
{
    Trie trie;
    trie.insert("sin");
    trie.insert("cos");
    trie.insert("-");
    trie.insert("cat");
    trie.insert("car");
    trie.print();
    return 0;
}
