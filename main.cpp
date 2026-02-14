#include <algorithm> // std::lower_bound
#include <cstring> // memmove
#include <stdint.h>
#include <stdio.h>
#include <utility> // std::pair

// clang-format off
enum Op : int8_t { opUnknown = 0, parOpen, parClose, opPlus, opMinus, opMul, opDiv, opNegate, opSin, opCos, opTan, opCat, opCar };
// clang-format on

template <typename Key, typename Val>
class BinarySearchMap {
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
    Op op = opUnknown;
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
    TrieNode root;
    void clear(TrieNode& n)
    {
        for (const auto& [k, v] : n.children) {
            clear(*v);
            delete v;
        }
    }

public:
    ~Trie() { clear(root); };

    void insert(const char* word, Op op)
    {
        if (op == opUnknown)
            return;

        TrieNode* node = &root;
        while (char c = *word) {
            auto& foundNode = node->children.insert(c);
            if (foundNode)
                node = foundNode;
            else {
                foundNode = new TrieNode {};
                node = foundNode;
            }

            word++;
        }
        node->op = op;
    }

    void print() { root.print(0); }

    int match(const char* text, int start, Op& op) const
    {
        const TrieNode* node = &root;
        int len = 0;

        for (int i = start; text[i]; ++i) {
            TrieNode* const* foundValue = node->children.find(text[i]);
            if (!foundValue)
                break;

            node = *foundValue;
            ++len;

            if (node->op == opUnknown) {
                op = node->op;
                return len;
            }
        }

        return 0;
    }
};

int main()
{
    Trie trie;
    trie.insert("sin", opSin);
    trie.insert("cos", opCos);
    trie.insert("-", opMinus);
    trie.insert("cat", opCat);
    trie.insert("car", opCat);
    trie.print();
    return 0;
}
