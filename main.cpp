#include <algorithm> // std::lower_bound
#include <cassert>
#include <cstring> // memmove
#include <flat_set>
#include <stdint.h>
#include <stdio.h>
#include <utility> // std::pair
#include <vector>

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

    size_t find(size_t thisNodeStart, const KeyType& key, NumType& outNumChildren) const
    {
        const size_t numStart = thisNodeStart;
        const NumType num = *((NumType*)(m_data.data() + numStart));
        outNumChildren = num;

        const size_t keyOffsetStart = numStart + sizeof(NumType);
        KeyType* keys = ((KeyType*)(m_data.data() + keyOffsetStart));

        const size_t childNodeOffsetStart = align<IndexType>(keyOffsetStart + num * sizeof(KeyType));
        IndexType* nodes = (IndexType*)(m_data.data() + childNodeOffsetStart);

        if (*nodes == (IndexType)0) // is leaf
            return (size_t)-2;

        auto keyIt = std::lower_bound(keys, keys + num, key);
        if (keyIt == keys + num || *keyIt != key)
            return (size_t)-1;

        return *(nodes + std::distance(keys, keyIt));
    }

    int match(const char* text) const
    {
        if (m_data.empty())
            return 0;

        size_t currentNode = 0;
        int len = 0;

        while (*text) {
            NumType numChildren;
            size_t foundValue = find(currentNode, *text, numChildren);
            if (foundValue == (size_t)-1) // not found
                break;

            if (foundValue == (size_t)-2) // is leaf
                return len + 1;

            currentNode = foundValue;

            ++len;
            ++text;
        }

        return 0;
    }

    size_t pack(const TrieNode& node, const size_t thisNodeStart)
    {
        // make additional shift for duplicates
        int duplicateShift = 0;
        for (int i = 0; i < node.getSize(); ++i) {
            const TrieNode* childNode = node.getNode(i);
            if (childNode->getSize() != 0 && childNode->bStop)
                duplicateShift++;
        }

        const size_t nodeSize = node.getSize();
        const size_t layoutSize = node.getSize() + duplicateShift;

        const size_t numStart = thisNodeStart;
        const size_t keyStart = numStart + sizeof(NumType);
        size_t childNodeStart = align<IndexType>(keyStart + layoutSize * sizeof(KeyType));
        const size_t nodeEndInitial = childNodeStart + layoutSize * sizeof(IndexType);
        size_t nodeEnd = nodeEndInitial;

        m_data.resize(nodeEnd), assert((size_t)m_data.data() % 8 == 0);

        NumType* numPacked = (NumType*)&m_data.at(numStart);
        KeyType* packedKeys = (KeyType*)&m_data.at(keyStart);
        IndexType* packedNodes = (IndexType*)&m_data.at(childNodeStart);

        *numPacked = nodeSize;

        for (int I = 0, packedI = 0; I < nodeSize; ++I, ++packedI) {
            const auto& childKey = node.getKey(I);
            const auto& childNode = node.getNode(I);

            packedKeys[packedI] = childKey;
            packedNodes[packedI] = nodeEnd;

            if (childNode->getSize() != 0) {
                packedNodes[packedI] = nodeEnd;
                nodeEnd = pack(*childNode, nodeEnd);

                if (childNode->bStop) {
                    packedKeys[++packedI] = childKey;
                    packedNodes[packedI] = 0;
                    (*numPacked)++;
                }

            } else {
                packedNodes[packedI] = 0;
            }
        }

        return nodeEnd;
    }
};

const char* fruits[] = { "apple", "apricot", "avocado", "banana", "bilberry", "blackberry", "blackcurrant", "blueberry",
    "boysenberry", "currant", "cherry", "cherimoya", "chico fruit", "cloudberry", "coconut", "cranberry", "cucumber", "custard apple",
    "damson", "date", "dragonfruit", "durian", "elderberry", "feijoa", "fig", "goji berry", "gooseberry", "grape", "raisin",
    "grapefruit", "guava", "honeyberry", "huckleberry", "jabuticaba", "jackfruit", "jambul", "jujube", "juniper berry", "kiwano",
    "kiwifruit", "kumquat", "lemon", "lime", "loquat", "longan", "lychee", "mango", "mangosteen", "marionberry", "melon", "cantaloupe",
    "honeydew", "watermelon", "miracle fruit", "mulberry", "nectarine", "nance", "olive", "orange", "blood orange", "clementine",
    "mandarine", "tangerine", "papaya", "passionfruit", "peach", "pear", "persimmon", "physalis", "plantain", "plum", "prune", "pineapple",
    "plumcot", "pomegranate", "pomelo", "purple mangosteen", "quince", "raspberry", "salmonberry", "rambutan", "redcurrant", "salal berry",
    "salak", "satsuma", "soursop", "star fruit", "solanum quitoense", "strawberry", "tamarillo", "tamarind", "ugli fruit", "yuzu" };

int main()
{

    Trie trie;
    DenseTrie dtrie;
#if 0
    int index = 0;
    for (auto& f : fruits) {
        printf("Added: %s\n", f);
        trie.insert(f);
        if (index++ > 30)
            break;
    }
#endif

    trie.insert("ca");
    trie.insert("car");
    trie.insert("cb");
    trie.insert("cbz");

    // trie.insert("cd");
    // trie.insert("d");

    dtrie.pack(trie.root, 0);

    // printf("New:  %d\n", dtrie.match("ca"));
    // printf("New:  %d\n", dtrie.match("car"));

#if 0
    int numErrors = 0;
    for (auto& f : fruits) {
        auto oldLen = trie.match(f);
        auto newLen = dtrie.match(f);
        if (oldLen != 0 && newLen == 0) {
            printf("Old:  %d %s\n", oldLen, f);
            printf("New:  %d %s\n", newLen, f);
            numErrors++;
        }
    }
    printf("numErrors: %d\n", numErrors);
#endif

#if 1
    FILE* f = fopen("tree.bin", "wb");
    fwrite(dtrie.m_data.data(), 1, dtrie.m_data.size(), f);
    fclose(f);
#endif
    return 0;
}
