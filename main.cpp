#include <algorithm> // std::lower_bound
#include <cassert>
#include <cctype>
#include <cstring> // memmove
#include <stdint.h>
#include <stdio.h>
#include <utility> // std::pair
#include <vector>

#include "words.h"

template <typename T>
inline constexpr size_t align(size_t unaligned)
{
    constexpr size_t alignMask = (alignof(T) - 1);
    static_assert((alignMask & alignof(T)) == 0); // is pow of 2
    return (((size_t)unaligned + alignMask) & ~alignMask);
}
#define ARR_SIZE(x) sizeof(x) / sizeof(*x)

template <typename Key, typename Val>
class BinarySearchMap {
public:
    std::vector<Key> keys;
    std::vector<Val> vals;

public:
    struct SoAIterator {
        const Key* p1;
        const Val* p2;

        std::pair<const Key&, const Val&> operator*() const { return { *p1, *p2 }; }
        bool operator!=(const SoAIterator& other) const { return p1 != other.p1; }

        SoAIterator& operator++()
        {
            ++p1, ++p2;
            return *this;
        }
    };

    auto begin() const { return SoAIterator { keys.data(), vals.data() }; }
    auto end() const { return SoAIterator { keys.data() + keys.size(), vals.data() + vals.size() }; }

    Val& insert(const Key& key)
    {
        auto keyIt = std::lower_bound(keys.begin(), keys.end(), key);
        int newKeyPos = std::distance(keys.begin(), keyIt);
        auto valueIt = vals.begin() + newKeyPos;
        if (keyIt == keys.end() || *keyIt != key) {
            keys.insert(keyIt, key);
            valueIt = vals.emplace(valueIt, Val {});
        }
        return *valueIt;
    }

    const Val* find(const Key& key) const
    {
        auto it = std::lower_bound(keys.begin(), keys.end(), key);
        if (it != keys.end() && *it == key)
            return &*(vals.begin() + std::distance(keys.begin(), it));
        return nullptr;
    }
};

struct TrieNode {
    // Op op = opUnknown;
    bool bStop = false;
    BinarySearchMap<char, TrieNode*> children;

    const auto& getKey(size_t index) const { return children.keys.at(index); }
    const auto& getNode(size_t index) const { return children.vals.at(index); }
    size_t getSize() const
    {
        assert(children.keys.size() == children.vals.size());
        return children.keys.size();
    }

    void print(int offset) const
    {
        putchar('\n');
        offset++;
        for (const auto& [c, node] : children) {
            for (int i = 0; i < offset; i++)
                putchar(' ');
            printf("%c", c);

            node->print(offset);
        }
    }
};

class Trie {
public:
    TrieNode root;

public:
    void clear(TrieNode& n)
    {
        for (const auto& [k, v] : n.children) {
            clear(*v);
            delete v;
        }
    }

    ~Trie() { clear(root); };

    void insert(const char* word)
    {

        TrieNode* node = &root;
        while (char c = *word) {
            auto& foundNode = node->children.insert(c);
            if (!foundNode)
                foundNode = new TrieNode {};

            node = foundNode;

            word++;
        }
        // node->op = op;
        node->bStop = true;
    }

    void print() { root.print(0); }

    int match(const char* text) const
    {
        const TrieNode* node = &root;
        int len = 0;

        while (*text) {
            TrieNode* const* foundValue = node->children.find(*text);
            if (!foundValue)
                break;

            node = *foundValue;
            ++len;

            if (node->bStop == true) {
                // op = node->op;
                return len;
            }

            text++;
        }

        return 0;
    }
};

// for (const auto& [k, v] : node.children)
using IndexType = uint32_t;
using NumType = uint8_t;
using KeyType = char;

class DenseTrie {
public:
    std::vector<uint8_t> m_data;

public:
    DenseTrie()
    {
        m_data.reserve(50);
        assert(((size_t)m_data.data()) % 8 == 0);
    }

    static bool isIdent(char c) { return isalnum(c) || c == '_'; }

    int match(const char* text) const
    {
        if (m_data.empty())
            return 0;

        size_t currentNode = 0;
        int len = 0, lenEnd = 0;

        while (const char& c = *text) {
            // retrieve child odes
            ++len;
            const size_t numStart = currentNode;
            const NumType num = *(NumType*)(&m_data.at(numStart));
            const size_t keyOffsetStart = numStart + sizeof(NumType);
            const KeyType* keys = (const KeyType*)(&m_data.at(keyOffsetStart));
            const size_t childNodeOffsetStart = align<IndexType>(keyOffsetStart + num * sizeof(KeyType));
            const IndexType* nodes = (const IndexType*)(&m_data.at(childNodeOffsetStart));

            auto keysEnd = keys + num;
            auto keyIt = std::lower_bound(keys, keysEnd, c);
            if (keyIt == keysEnd || *keyIt != c) {
                break; // not found
            }

            currentNode = *(nodes + size_t(keyIt - keys));

            if (currentNode == 0) {
                lenEnd = len;
            }

            auto nextKeyIt = keyIt + 1;
            if (nextKeyIt != keysEnd && *keyIt == *nextKeyIt) {
                lenEnd = len;
            }

            ++text;
        }

        return lenEnd;
    }

    void pack(const TrieNode& node)
    {
        // make additional shift for duplicates
        int duplicateShift = 0;
        for (int i = 0; i < node.getSize(); ++i) {
            const TrieNode* childNode = node.getNode(i);
            if (childNode->getSize() != 0 && childNode->bStop)
                duplicateShift++;
        }

        const size_t nodeSize = node.getSize();
        const size_t layoutSize = nodeSize + duplicateShift;

        const size_t numStart = m_data.size();
        const size_t keyStart = numStart + sizeof(NumType);
        const size_t childNodeStart = align<IndexType>(keyStart + layoutSize * sizeof(KeyType));

        m_data.resize(childNodeStart + layoutSize * sizeof(IndexType));
        assert((size_t)m_data.data() % 8 == 0);

        NumType* numPacked = (NumType*)&m_data.at(numStart);
        KeyType* packedKeys = (KeyType*)&m_data.at(keyStart);
        IndexType* packedNodes = (IndexType*)&m_data.at(childNodeStart);

        *numPacked = nodeSize;

        for (int I = 0, packedI = 0; I < nodeSize; ++I, ++packedI) {
            const auto& childKey = node.getKey(I);
            const auto& childNode = node.getNode(I);

            packedKeys[packedI] = childKey;
            packedNodes[packedI] = m_data.size();

            if (childNode->getSize() != 0) {
                packedNodes[packedI] = m_data.size();
                pack(*childNode);

                // as data can be reallocated we should update pointers
                numPacked = (NumType*)&m_data.at(numStart);
                packedKeys = (KeyType*)&m_data.at(keyStart);
                packedNodes = (IndexType*)&m_data.at(childNodeStart);

                if (childNode->bStop) {
                    (*numPacked)++;
                    packedKeys[++packedI] = childKey;
                    packedNodes[packedI] = 0;
                }

            } else {
                packedNodes[packedI] = 0;
            }
        }
    }
};

#define WORDS 0
int main()
{
    Trie trie;
    DenseTrie dtrie;
    const auto& words = WordsFruits();

#if WORDS
    for (auto& f : words) {
        printf("Added: %s\n", f);
        trie.insert(f);
    }
#endif

#if !WORDS

    trie.insert("car");

    dtrie.pack(trie.root);

    printf("%d %s\n", dtrie.match("caraganda"), "caraganda");

#else
    dtrie.pack(trie.root);
#endif

#if WORDS
    int numMismatches = 0;
    for (auto& f : words) {
        auto oldLen = trie.match(f);
        auto newLen = dtrie.match(f);
        if (oldLen != newLen) {
            printf("Old: %d,  New: %d  %s\n", oldLen, newLen, f);
            numMismatches++;
        }
    }
    printf("num Mmsmatches: %d\n", numMismatches);
#endif

#if 1
    FILE* f = fopen("tree.bin", "wb");
    fwrite(dtrie.m_data.data(), 1, dtrie.m_data.size(), f);
    fclose(f);
#endif
    return 0;
}
