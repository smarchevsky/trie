#include <stdint.h>
#include <stdio.h>
#include <vector>

// clang-format off
enum Op : int8_t { opUnknown = 0, parOpen, parClose, opPlus, opMinus, opMul, opDiv, opNegate, opSin, opCos, opTan, opCat, opCar };
// clang-format on

template <typename Key, typename Value>
class BinarySearchMap {
    std::vector<Key> keys;
    std::vector<Value> values;

public:
    struct SoAIterator {
        const Key* p1;
        const Value* p2;

        std::pair<const Key&, const Value&> operator*() const { return { *p1, *p2 }; }

        SoAIterator& operator++()
        {
            ++p1, ++p2;
            return *this;
        }

        bool operator!=(const SoAIterator& other) const { return p1 != other.p1; }
    };

    auto begin() const { return SoAIterator { keys.data(), values.data() }; }
    auto end() const { return SoAIterator { keys.data() + keys.size(), values.data() + values.size() }; }

    Value& insert(const Key& key)
    {
        auto keyIt = std::lower_bound(keys.begin(), keys.end(), key);
        int newKeyPos = std::distance(keys.begin(), keyIt);
        auto valueIt = values.begin() + newKeyPos;
        if (keyIt == keys.end() || *keyIt != key) {
            keys.insert(keyIt, key);
            valueIt = values.emplace(valueIt, Value {});
        }
        return *valueIt;
    }

    const Value* find(const Key& key) const
    {
        auto it = std::lower_bound(keys.begin(), keys.end(), key);
        if (it != keys.end() && *it == key)
            return &*(values.begin() + std::distance(keys.begin(), it));
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
