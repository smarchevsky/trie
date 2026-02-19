#include <algorithm> // std::lower_bound
#include <cassert>
#include <cstring> // memmove
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

const char* fruits[] = { "Apple", "Apricot", "Avocado", "Banana", "Bilberry", "Blackberry", "Blackcurrant", "Blueberry",
    "Boysenberry", "Currant", "Cherry", "Cherimoya", "Chico fruit", "Cloudberry", "Coconut", "Cranberry", "Cucumber", "Custard apple",
    "Damson", "Date", "Dragonfruit", "Durian", "Elderberry", "Feijoa", "Fig", "Goji berry", "Gooseberry", "Grape", "Raisin",
    "Grapefruit", "Guava", "Honeyberry", "Huckleberry", "Jabuticaba", "Jackfruit", "Jambul", "Jujube", "Juniper berry", "Kiwano",
    "Kiwifruit", "Kumquat", "Lemon", "Lime", "Loquat", "Longan", "Lychee", "Mango", "Mangosteen", "Marionberry", "Melon", "Cantaloupe",
    "Honeydew", "Watermelon", "Miracle fruit", "Mulberry", "Nectarine", "Nance", "Olive", "Orange", "Blood orange", "Clementine",
    "Mandarine", "Tangerine", "Papaya", "Passionfruit", "Peach", "Pear", "Persimmon", "Physalis", "Plantain", "Plum", "Prune", "Pineapple",
    "Plumcot", "Pomegranate", "Pomelo", "Purple mangosteen", "Quince", "Raspberry", "Salmonberry", "Rambutan", "Redcurrant", "Salal berry",
    "Salak", "Satsuma", "Soursop", "Star fruit", "Solanum quitoense", "Strawberry", "Tamarillo", "Tamarind", "Ugli fruit", "Yuzu" };

template <typename Key, typename Val>
class BinarySearchMap {
public:
    Key* keys = nullptr;
    Val* vals = nullptr;
    uint32_t size = 0;
    uint32_t capacity = 0;

public:
    struct SoAIterator {
        const Key* p1;
        const Val* p2;

        std::pair<const Key&, const Val&> operator*() const { return { *p1, *p2 }; }

        SoAIterator& operator++()
        {
            ++p1, ++p2;
            return *this;
        }

        bool operator!=(const SoAIterator& other) const { return p1 != other.p1; }
    };

    auto begin() const { return SoAIterator { keys, vals }; }
    auto end() const { return SoAIterator { keys + size, vals + size }; }

    Val& insert(const Key& key)
    {
        auto keyIt = std::lower_bound(keys, keys + size, key);
        int newKeyPos = std::distance(keys, keyIt);

        if (keyIt == keys + size || *keyIt != key) {
            size_t _pos = newKeyPos;
            if (size >= capacity) {
                size_t _new_cap = (capacity == 0) ? 4 : capacity * 2;
                keys = (Key*)realloc(keys, _new_cap * sizeof(*keys));
                vals = (Val*)realloc(vals, _new_cap * sizeof(*vals));
                capacity = _new_cap;
            }

            if (_pos < size) {
                memmove(&keys[_pos + 1], &keys[_pos], (size - _pos) * sizeof(*keys));
                memmove(&vals[_pos + 1], &vals[_pos], (size - _pos) * sizeof(*vals));
            }

            keys[_pos] = key;
            vals[_pos] = Val {};
            size++;
        }
        return vals[newKeyPos];
    }

    const Val* find(const Key& key) const
    {
        auto keyIt = std::lower_bound(keys, keys + size, key);
        if (keyIt == keys + size || *keyIt != key)
            return &*(vals + std::distance(keys, keyIt));
        return nullptr;
    }
};

struct TrieNode {
    // Op op = opUnknown;
    bool bStop = false;
    BinarySearchMap<char, TrieNode*> children;

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

    int match(const char* text, int start) const
    {
        const TrieNode* node = &root;
        int len = 0;

        for (int i = start; text[i]; ++i) {
            TrieNode* const* foundValue = node->children.find(text[i]);
            if (!foundValue)
                break;

            node = *foundValue;
            ++len;

            if (node->bStop == true) {
                // op = node->op;
                return len;
            }
        }

        return 0;
    }
};

// for (const auto& [k, v] : node.children)
using IndexType = uint32_t;
using NumType = uint8_t;
using KeyType = uint8_t;

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

    size_t pack(const TrieNode* node, size_t thisNodeStart)
    {
        const size_t numStart = thisNodeStart;
        const size_t keyOffsetStart = numStart + sizeof(NumType);
        const size_t childNodeOffsetStart = align<IndexType>(keyOffsetStart + node->children.size * sizeof(KeyType));
        size_t thisNodeEnd = childNodeOffsetStart + node->children.size * sizeof(IndexType);

        m_data.resize(thisNodeEnd);

        *((NumType*)(m_data.data() + numStart)) = node->children.size;

        auto* keys = node->children.keys;
        auto* nodes = node->children.vals;

        for (int i = 0; i < node->children.size; ++i) {
            ((KeyType*)(m_data.data() + keyOffsetStart))[i] = keys[i];
            IndexType* dataAsIndex = (IndexType*)(m_data.data() + childNodeOffsetStart);
            if (nodes[i]->children.size != 0) {
                dataAsIndex[i] = thisNodeEnd;
                thisNodeEnd = pack(nodes[i], thisNodeEnd);
            } else
                dataAsIndex[i] = 0;
        }

        return thisNodeEnd;
    }
};

int main()
{

    Trie trie;
    for (auto& f : fruits) {
        // trie.insert(f);
    }

    trie.insert("sin");
    trie.insert("cos");
    trie.insert("car");

    trie.print();

    DenseTrie dTrie;
    dTrie.pack(&trie.root, 0);

    NumType childNum = 0;
    int len = dTrie.match("car");
    printf("%d\n", len);

#if 1
    FILE* f = fopen("tree.bin", "wb");
    fwrite(dTrie.m_data.data(), 1, dTrie.m_data.size(), f);
    fclose(f);
#endif
    return 0;
}
