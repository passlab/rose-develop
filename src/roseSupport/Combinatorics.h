#ifndef ROSE_Combinatorics_H
#define ROSE_Combinatorics_H

#include <rosePublicConfig.h>
#include <LinearCongruentialGenerator.h>

#include <algorithm>
#include <cassert>
#include <list>
#include <ostream>
#include <rose_override.h>
#include <Sawyer/Assert.h>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

#ifdef ROSE_HAVE_LIBGCRYPT
#include <gcrypt.h>
#endif

namespace Rose {

/** Combinatoric functions. */
namespace Combinatorics {

/** Returns the factorial of @p n. */
template<typename T>
static T
factorial(T n)
{
    T retval = 1;
    while (n>1) {
        T next = retval * n--;
        assert(next>retval); // overflow
        retval = next;
    }
    return retval;
}

/** Simulate flipping a coin. Randomly returns true or false with equal probability. See also,
 *  LinearCongruentialGenerator::flip_coin(). */
ROSE_DLL_API bool flip_coin();

/** Permute a vector according to the specified permutation number. The permutation number should be between zero (inclusive)
 *  and the factorial of the values size (exclusive).  A permutation number of zero is a no-op; higher permutation numbers
 *  shuffle the values in repeatable ways.  Using swap rather that erase/insert is much faster than the standard Lehmer codes,
 *  but doesn't return permutations in lexicographic order.  This function can perform approx 9.6 million permutations per
 *  second on a vector of 12 64-bit integers on Robb's machine (computing all 12! permutations in about 50 seconds). */
template<typename T>
static void
permute(std::vector<T> &values/*in,out*/, uint64_t pn, size_t sz=(size_t)(-1))
{
    if ((size_t)(-1)==sz)
        sz = values.size();
    assert(sz<=values.size());
    assert(pn<factorial(sz));
    for (size_t i=0; i<sz; ++i) {
        uint64_t radix = sz - i;
        uint64_t idx = pn % radix;
        std::swap(values[i+idx], values[i]);
        pn /= radix;
    }
}

/** Shuffle the values of a vector.  If @p nitems is supplied then only the first @p nitems of the vector are shuffled. If
 *  @p limit is specified then the algorithm returns after at least the first @p limit elements are sufficiently shuffled. If
 *  an @p lcg is specified, then it will be used to generate the random numbers, otherwise a built-in random number generator
 *  is used. */
template<typename T>
void
shuffle(std::vector<T> &vector, size_t nitems=(size_t)(-1), size_t limit=(size_t)(-1), LinearCongruentialGenerator *lcg=NULL)
{
    static LinearCongruentialGenerator my_lcg;
    if (!lcg)
        lcg = &my_lcg;
    nitems = std::min(nitems, vector.size());
    limit = std::min(limit, nitems);

    for (size_t i=0; i<limit; ++i)
        std::swap(vector[i], vector[lcg->next()%nitems]);
}

// [Robb Matzke 2018-05-09]: deprecated. Use HasherSha1 instead.
//  Compute a SHA1 digest.  The returned vector will contain 20 bytes and can be converted to a string of 40 hexadecimal
// characters via digest_to_string().  If called when a SHA1 algorithm is not available (due to ROSE configuration) an
// empty vector is returned.
ROSE_DLL_API std::vector<uint8_t> sha1_digest(const uint8_t *data, size_t size) ROSE_DEPRECATED("use HasherSha1");
ROSE_DLL_API std::vector<uint8_t> sha1_digest(const std::vector<uint8_t> &data) ROSE_DEPRECATED("use HasherSha1");
ROSE_DLL_API std::vector<uint8_t> sha1_digest(const std::string &data) ROSE_DEPRECATED("use HasherSha1");

// [Robb Matzke 2018-05-09]: deprecated. Use HasherFnv instead.
// Compute the Fowler–Noll–Vo fast string hash.  This is not a cryptographic hash. Speed is marginally slower than Murmur
// hash, but collision rate is slightly less.
ROSE_DLL_API uint64_t fnv1a64_digest(const uint8_t *data, size_t size) ROSE_DEPRECATED("use HasherFnv");
ROSE_DLL_API uint64_t fnv1a64_digest(const std::vector<uint8_t> &data) ROSE_DEPRECATED("use HasherFnv");
ROSE_DLL_API uint64_t fnv1a64_digest(const std::string &data) ROSE_DEPRECATED("use HasherFnv");

// [Robb Matzke 2018-05-09]: deprecated. Use the Hasher API instead.
// Converts a binary digest to a string of hexadecimal characters.  The input can actually be any type of data and any
// length. The output will be twice as long as the input.  If you're using this to convert binary data to a printable format
// you're doing it wrong--use StringUtility::encode_base64() instead.
ROSE_DLL_API std::string digest_to_string(const uint8_t *data, size_t size) ROSE_DEPRECATED("use Hasher");
ROSE_DLL_API std::string digest_to_string(const std::vector<uint8_t> &digest) ROSE_DEPRECATED("use Hasher");
ROSE_DLL_API std::string digest_to_string(const std::string &data) ROSE_DEPRECATED("use Hasher");

/** Hash interface.
 *
 *  This class defines the API for hash functions. A hash function takes an arbitrary size message as input and returns a
 *  fixed size digest. The subclasses implement specific hash functions, the digests of which are different sizes. For
 *  instance, a SHA1 digest is always 20 bytes, and a SHA512 digest is always 64 bytes.
 *
 *  The digest can be computed incrementally by inserting the message in parts using various @ref insert functions. Once the
 *  message has been fully inserted, the digest can be obtained with the @ref digest function. The @ref digest can be retrieved
 *  repeatedly, but no additional message parts can be added once a digest is retrieved (doing so will result in an @ref
 *  Hasher::Exception "Exception" being thrown).  However, the hasher can be reset to an initial state by calling @ref clear,
 *  after which a new message can be inserted.  Subclasses that return hashes that are 8 bytes or narrower sometimes have an
 *  additional method that returns the digest as an unsigned integer (but the @ref digest method must always be present).
 *
 *  Hasher objects are copyable. The new hasher starts out in the same state as the original hasher, but then they can diverge
 *  from one another. For instance, if you have a multi-part message and want an intermediate hash, the easiest way to do that
 *  is to insert some message parts into a hasher, then make a copy to obtain its digest, then continue inserting message parts
 *  into the original hasher (the copy cannot accept more parts since we called its @ref digest method). Obtaining intermediate
 *  hashes this way is usually faster than re-hashing.
 *
 *  The API contains a few functions for printing hashes. There's a @ref toString function that can return the digest as a
 *  hexadecimal string and a static version that that converts a given digest to a string. Also, using a hasher in an @c
 *  std::ostream output operator is the same as obtaining the hasher's digest, formatting it, and sending the string to the
 *  stream.  All these functions (except the static @c toString) call @ref digest under the covers, thus finalizing the
 *  hasher.
 *
 *  New hash functions can be created very easily by subclassing Hasher and defining an @ref append method. Often, this is all
 *  that's necessary. Other commonly overridden functions are the constructor, @ref clear, and @ref digest.  The subclasses
 *  generally have names beginning with the string "Hasher" so they can be found easily in alphabetical class listings. For
 *  instance, see the classes and typedefs in the @ref Rose::Combinatorics namespace.
 *
 *  If a hasher uses an external library (like libgcrypt) then it should be designed in such a way that the code that uses the
 *  hash compiles, but throws an exception.  The @ref Hasher::Exception "Exception" class is intended for this purpose, as well
 *  as all other situations where hashing fails. */
class ROSE_DLL_API Hasher {
public:
    /** The digest of the input message.
     *
     *  Since different hash functions have different sized digests, the digest is represented most generically as a vector of
     *  bytes. */
    typedef std::vector<uint8_t> Digest;

    /** Exceptions for hashing. */
    class Exception: public std::runtime_error {
    public:
        /** Constructor. */
        Exception(const std::string &mesg): std::runtime_error(mesg) {}
        ~Exception() throw () {}
    };
    
protected:
    Digest digest_;

public:
    virtual ~Hasher() {}

    /** Reset the hasher to its initial state. */
    virtual void clear() { digest_ = Digest(); }

    /** Return the digest.
     *
     *  Finalizes the hash function and returns the digest for all the input.  Additional input should not be inserted after
     *  this function is called since some hash functions don't support this. */
    virtual const Digest& digest() { return digest_; }

    /** Insert data into the digest.
     *
     *  This method inserts data into a digest. Data can only be inserted if the user has not called @ref digest yet.
     *
     * @{ */
    void insert(const std::string &x) { append((const uint8_t*)x.c_str(), x.size()); }
    void insert(uint64_t x) { append((uint8_t*)&x, sizeof x); }
    void insert(const uint8_t *x, size_t size) { append(x, size); }
    /** @} */
    
    /** Insert data into the digest.
     *
     *  This is the lowest level method of inserting new message content into the digest. This can be called as often as
     *  desired, building a digest incrementally. */
    virtual void append(const uint8_t *message, size_t messageSize) = 0;

    /** Convert a digest to a hexadecimal string. */
    static std::string toString(const Digest&);

    /** String representation of the digest.
     *
     *  This works by first calling @ref digest, which finalizes the hasher and thus later calls to @ref insert will not be
     *  permitted unless the hasher is first reset. */
    std::string toString();

    /** Print a hash to a stream.
     *
     *  This is a wrapper that calls the @ref digest function to finalize the hash, converts the digest to a hexadecimal
     *  string, and sends it to the stream. */
    void print(std::ostream&);
};

/** Hasher for any libgcrypt hash algorithm.
 *
 *  The @c hashAlgorithmId template parameter is one of the libgcrypt algorithm constants (type int) with a name starting with
 *  "GCRY_MD_". You can find a list
 *  [here](https://gnupg.org/documentation/manuals/gcrypt/Available-hash-algorithms.html#Available-hash-algorithms). */
template<int hashAlgorithmId>
class HasherGcrypt: public Hasher {
#ifdef ROSE_HAVE_LIBGCRYPT
    gcry_md_hd_t md_;
#endif

public:
    HasherGcrypt() {
      #ifdef ROSE_HAVE_LIBGCRYPT
        if (gcry_md_open(&md_, hashAlgorithmId, 0) != GPG_ERR_NO_ERROR)
            throw Exception("cannot initialize libgcrypt hash " + std::string(gcry_md_algo_name(hashAlgorithmId)));
      #else
        throw Exception("ROSE was not configured with libgcrypt");
      #endif
    }

    HasherGcrypt(const HasherGcrypt &other) {
      #ifdef ROSE_HAVE_LIBGCRYPT
        if (gcry_md_copy(&md_, other.md_) != GPG_ERR_NO_ERROR)
            throw Exception("cannot copy libgcrypt hash " + std::string(gcry_md_algo_name(hashAlgorithmId)));
      #else
        throw Exception("ROSE was not configured with libgcrypt");
      #endif
    }
    
    ~HasherGcrypt() {
      #ifdef ROSE_HAVE_LIBGCRYPT
        gcry_md_close(md_);
      #endif
    }

    HasherGcrypt& operator=(const HasherGcrypt &other) {
      #ifdef ROSE_HAVE_LIBGCRYPT
        gcry_md_close(md_);
        if (gcry_md_copy(&md_, other.md_) != GPG_ERR_NO_ERROR)
            throw Exception("cannot copy libgcrypt hash " + std::string(gcry_md_algo_name(hashAlgorithmId)));
      #else
        throw Exception("ROSE was not configured with libgcrypt");
      #endif
    }
    
    void clear() {
      #ifdef ROSE_HAVE_LIBGCRYPT
        gcry_md_reset(md_);
      #endif
        Hasher::clear();
    }

    const Digest& digest() {
        if (digest_.empty()) {
          #ifdef ROSE_HAVE_LIBGCRYPT
            gcry_md_final(md_);
            digest_.resize(gcry_md_get_algo_dlen(hashAlgorithmId), 0);
            uint8_t *d = gcry_md_read(md_, hashAlgorithmId);
            ASSERT_not_null(d);
            memcpy(&digest_[0], d, digest_.size());
          #else
            ASSERT_not_reachable("ROSE was not configured with libgcrypt");
          #endif
        }
        return Hasher::digest();
    }

    void append(const uint8_t *message, size_t messageSize) {
        ASSERT_require(message || 0==messageSize);
      #ifdef ROSE_HAVE_LIBGCRYPT
        if (!digest_.empty())
            throw Exception("cannot append after returning digest");
        if (messageSize > 0)
            gcry_md_write(md_, message, messageSize);
      #else
        ASSERT_not_reachable("ROSE was not configured with libgcrypt");
      #endif
    }
};

#ifdef ROSE_HAVE_LIBGCRYPT
typedef HasherGcrypt<GCRY_MD_MD5> HasherMd5;            /**< MD5 hasher. Throws exception if libgcrypt is not configured. */
typedef HasherGcrypt<GCRY_MD_SHA1> HasherSha1;          /**< SHA1 hasher. Throws exception if libgcrypt is not configured. */
typedef HasherGcrypt<GCRY_MD_SHA256> HasherSha256;      /**< SHA-256 hasher. Throws exception if libgcrypt is not configured. */
typedef HasherGcrypt<GCRY_MD_SHA384> HasherSha384;      /**< SHA-384 hasher. Throws exception if libgcrypt is not configured. */
typedef HasherGcrypt<GCRY_MD_SHA512> HasherSha512;      /**< SHA-512 hasher. Throws exception if libgcrypt is not configured. */
typedef HasherGcrypt<GCRY_MD_CRC32> HasherCrc32;        /**< ISO 3309 hasher. Throws exception if libgcrypt is not configured. */
#else
typedef HasherGcrypt<0> HasherMd5;                      // the template argument is arbitrary and they can all be the same
typedef HasherGcrypt<0> HasherSha1;
typedef HasherGcrypt<0> HasherSha256;
typedef HasherGcrypt<0> HasherSha384;
typedef HasherGcrypt<0> HasherSha512;
typedef HasherGcrypt<0> HasherCrc32;
#endif

/** Fowler-Noll-Vo hashing using the Hasher interface. */
class ROSE_DLL_API HasherFnv: public Hasher {
    uint64_t partial_;
public:
    HasherFnv(): partial_(0xcbf29ce484222325ull) {}
    const Digest& digest() ROSE_OVERRIDE;
    void append(const uint8_t *message, size_t messageSize);
    uint64_t partial() const { return partial_; }
};

/** Convert two vectors to a vector of pairs.
 *
 *  If the two input vectors are not the same length, then the length of the result is the length of the shorter input vector. */
template<class T, class U>
std::vector<std::pair<T, U> >
zip(const std::vector<T> &first, const std::vector<U> &second) {
    size_t retvalSize = std::min(first.size(), second.size());
    std::vector<std::pair<T, U> > retval;
    retval.reserve(retvalSize);
    for (size_t i = 0; i < retvalSize; ++i)
        retval.push_back(std::pair<T, U>(first[i], second[i]));
    return retval;
}

/** Convert a vector of pairs to a pair of vectors. */
template<class T, class U>
std::pair<std::vector<T>, std::vector<U> >
unzip(const std::vector<std::pair<T, U> > &pairs) {
    std::pair<std::vector<T>, std::vector<U> > retval;
    retval.first.reserve(pairs.size());
    retval.second.reserve(pairs.size());
    for (size_t i = 0; i < pairs.size(); ++i) {
        retval.first.push_back(pairs[i].first);
        retval.second.push_back(pairs[i].second);
    }
    return retval;
}

} // namespace
} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Prints a hash to a stream. See @ref Hasher::print. */
ROSE_DLL_API std::ostream& operator<<(std::ostream&, Rose::Combinatorics::Hasher&);

#endif
