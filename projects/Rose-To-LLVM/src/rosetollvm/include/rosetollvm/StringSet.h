#ifndef STRING_SET_INCLUDED
#define STRING_SET_INCLUDED

#include<assert.h>
#include <string.h>

#include <stack>
#include <vector>
#include "rosetollvm/Control.h"

class StringElement
{
public:
    StringElement *next;

    unsigned HashAddress() { return hash_address; }

    char *Name() { return name; }

    int Length() { return length; } // The actual strlen of name 

    int Size() { return size; }     // The length of name when encoded Hexadecimal characters (such as \0F) are counted as one character

    int Index() { return pool_index; }

    StringElement(const char *name_, int size_, int pool_index_, unsigned hash_address_) : size(size_),
                                                                                           pool_index(pool_index_),
                                                                                           hash_address(hash_address_)
    {
        length = strlen(name_);
        name = new char[length + 1];
        memmove(name, name_, length * sizeof(char));
        name[length] = '\0';
    }

    ~StringElement() { delete [] name; }

private:

    int  pool_index,
         length,
         size;
    unsigned hash_address;
    char *name;
};


class HashPrimes
{
public:
    enum
    {
        DEFAULT_HASH_SIZE = 4093,
        MAX_HASH_SIZE = 32771
    };

    static int primes[];
    int prime_index;

    HashPrimes() : prime_index(0)
    {}
};


class Hash
{
public:
    //
    // Same as above function for a regular "char" string.
    //
    inline static unsigned Function(const char *str, int len)
    {
        unsigned hash_value = str[len >> 1]; // start with center (or unique) letter
        const char *tail = &str[len - 1];

        for (int i = 0; i < 5 && str < tail; i++)
        {
            unsigned k = *tail--;
            hash_value += ((k << 7) + *str++);
        }

        return hash_value;
    }

    inline static unsigned Function(const char *str)  { return Function(str, strlen(str)); }
};


class StringSet : public HashPrimes
{
    public:
        StringSet();
        ~StringSet();

        int insert(const char *, int size = 0);
 
        bool contains(const char *);

        int getIndex(const char *);

        int size() { return element_pool.size(); }

        StringElement *operator[](const int i) { return element_pool[i]; }

        void Push() { container_stack.push(element_pool.size()); }

        void Pop()
        {
           int previous_size = container_stack.top();
           container_stack.pop(); // remove top element.

           //
           // First, remove all the elements from the hash table;
           //
           for (int i = element_pool.size() - 1; i >= previous_size; i--)
           {
               StringElement *element = element_pool[i];
               int k = element -> HashAddress() % hash_size;
               ROSE2LLVM_ASSERT(base[k] == element);
               base[k] = (StringElement *) element -> next;
               delete element;
           }
           //
           // Then, remove the elements from the pool.
           //
           element_pool.resize(previous_size);
        }

        void clear()
        {
            while(! container_stack.empty()) {
                this -> Pop();
            }
            if (element_pool.size() > 0)
            {
                container_stack.push(0);
                this -> Pop();
            }
        }

    private:

        std::vector<StringElement *> element_pool;
        std::vector<StringElement *> base;
        int hash_size;

        std::stack<int> container_stack;

        inline static unsigned Hash(const char *str) { return Hash::Function(str); }

        void Rehash();
    };
#endif
