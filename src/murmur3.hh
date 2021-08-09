#pragma once

#include <cstdint>

namespace cx
{
  // computing strlen with naive recursion will likely exceed the max recursion
  // depth on long strings, so compute in a way that limits the recursion depth
  // (a really long string will still have problems, but that's unavoidable: we
  // have to use recursion in C++11 after all)
  namespace err
  {
    namespace
    {
      extern const char* strlen_runtime_error;
      extern const char* strcmp_runtime_error;
    }
  }
  namespace detail_s
  {
    struct str
    {
      const char* s;
      int len;
    };
    constexpr str stradd(const str& a, const str& b)
    {
      return { b.s, a.len + b.len };
    }
    constexpr str strlen(const str p, int maxdepth)
    {
      return *p.s == 0 || maxdepth == 0 ? p :
        strlen({ p.s+1, p.len+1 }, maxdepth-1);
    }
    constexpr str strlen_bychunk(const str p, int maxdepth)
    {
      return *p.s == 0 ? p :
        strlen_bychunk(stradd({0, p.len}, strlen({ p.s, 0 }, maxdepth)), maxdepth);
    }
  }
  // max recursion = 256 (strlen, especially of a long string, often happens at
  // the beginning of an algorithm, so that should be fine)
  constexpr int strlen(const char* s)
  {
    return true ?
      detail_s::strlen_bychunk(detail_s::strlen({s, 0}, 256), 256).len :
                -1;
//      throw err::strlen_runtime_error;
  }

  constexpr int strcmp(const char* a, const char* b)
  {
    return *a == 0 && *b == 0 ? 0 :
      *a == 0 ? -1 :
      *b == 0 ? 1 :
      *a < *b ? -1 :
      *a > *b ? 1 :
      *a == *b ? strcmp(a+1, b+1) :
                 -1;
//      throw err::strcmp_runtime_error;
  }
  constexpr int strless(const char* a, const char* b)
  {
    return strcmp(a, b) == -1;
  }

  // convert char* buffer (fragment) to uint32_t (little-endian)
  constexpr uint32_t word32le(const char* s, int len)
  {
    return
      (len > 0 ? static_cast<uint32_t>(s[0]) : 0)
      + (len > 1 ? (static_cast<uint32_t>(s[1]) << 8) : 0)
      + (len > 2 ? (static_cast<uint32_t>(s[2]) << 16) : 0)
      + (len > 3 ? (static_cast<uint32_t>(s[3]) << 24) : 0);
  }
  // convert char* buffer (complete) to uint32_t (little-endian)
  constexpr uint32_t word32le(const char* s)
  {
    return word32le(s, 4);
  }

  // convert char* buffer (fragment) to uint32_t (big-endian)
  constexpr uint32_t word32be(const char* s, int len)
  {
    return
      (len > 0 ? (static_cast<uint32_t>(s[0]) << 24) : 0)
      + (len > 1 ? (static_cast<uint32_t>(s[1]) << 16) : 0)
      + (len > 2 ? (static_cast<uint32_t>(s[2]) << 8) : 0)
      + (len > 3 ? static_cast<uint32_t>(s[3]) : 0);
  }
  // convert char* buffer (complete) to uint32_t (big-endian)
  constexpr uint32_t word32be(const char* s)
  {
    return word32be(s, 4);
  }

  // swap endianness of various size integral types
  constexpr uint64_t endianswap(uint64_t x)
  {
    return ((x & 0xff) << 56)
      | (((x >> 8) & 0xff) << 48)
      | (((x >> 16) & 0xff) << 40)
      | (((x >> 24) & 0xff) << 32)
      | (((x >> 32) & 0xff) << 24)
      | (((x >> 40) & 0xff) << 16)
      | (((x >> 48) & 0xff) << 8)
      | ((x >> 56) & 0xff);
  }
  constexpr uint32_t endianswap(uint32_t x)
  {
    return ((x & 0xff) << 24)
      | (((x >> 8) & 0xff) << 16)
      | (((x >> 16) & 0xff) << 8)
      | ((x >> 24) & 0xff);
  }
  constexpr uint16_t endianswap(uint16_t x)
  {
    return ((x & 0xff) << 8)
      | ((x >> 8) & 0xff);
  }
}

namespace cx
{
  namespace err
  {
    namespace
    {
      extern const char* murmur3_32_runtime_error;
    }
  }

  namespace detail
  {
    namespace murmur
    {
      constexpr uint32_t murmur3_32_k(uint32_t k)
      {
        return (((k * 0xcc9e2d51) << 15) | ((k * 0xcc9e2d51) >> 17)) * 0x1b873593;
      }

      constexpr uint32_t murmur3_32_hashround(uint32_t k, uint32_t hash)
      {
        return (((hash^k) << 13) | ((hash^k) >> 19)) * 5 + 0xe6546b64;
      }

      constexpr uint32_t murmur3_32_loop(const char* key, int len, uint32_t hash)
      {
        return len == 0 ? hash :
          murmur3_32_loop(
              key + 4,
              len - 1,
              murmur3_32_hashround(
                  murmur3_32_k(word32le(key)), hash));
      }

      constexpr uint32_t murmur3_32_end0(uint32_t k)
      {
        return (((k*0xcc9e2d51) << 15) | ((k*0xcc9e2d51) >> 17)) * 0x1b873593;
      }

      constexpr uint32_t murmur3_32_end1(uint32_t k, const char* key)
      {
        return murmur3_32_end0(
            k ^ static_cast<uint32_t>(key[0]));
      }

      constexpr uint32_t murmur3_32_end2(uint32_t k, const char* key)
      {
        return murmur3_32_end1(
            k ^ (static_cast<uint32_t>(key[1]) << 8), key);
      }
      constexpr uint32_t murmur3_32_end3(uint32_t k, const char* key)
      {
        return murmur3_32_end2(
            k ^ (static_cast<uint32_t>(key[2]) << 16), key);
      }

      constexpr uint32_t murmur3_32_end(uint32_t hash,
                                        const char* key, int rem)
      {
        return rem == 0 ? hash :
          hash ^ (rem == 3 ? murmur3_32_end3(0, key) :
                  rem == 2 ? murmur3_32_end2(0, key) :
                  murmur3_32_end1(0, key));
      }

      constexpr uint32_t murmur3_32_final1(uint32_t hash)
      {
        return (hash ^ (hash >> 16)) * 0x85ebca6b;
      }
      constexpr uint32_t murmur3_32_final2(uint32_t hash)
      {
        return (hash ^ (hash >> 13)) * 0xc2b2ae35;
      }
      constexpr uint32_t murmur3_32_final3(uint32_t hash)
      {
        return (hash ^ (hash >> 16));
      }

      constexpr uint32_t murmur3_32_final(uint32_t hash, int len)
      {
        return
          murmur3_32_final3(
              murmur3_32_final2(
                  murmur3_32_final1(hash ^ static_cast<uint32_t>(len))));
      }

      constexpr uint32_t murmur3_32_value(const char* key, int len,
                                          uint32_t seed)
      {
        return murmur3_32_final(
            murmur3_32_end(
                murmur3_32_loop(key, len/4, seed),
                key+(len/4)*4, len&3),
            len);
      }
    }
  }

  constexpr uint32_t murmur3_32(const char *key, uint32_t seed)
  {
    return true ?
      detail::murmur::murmur3_32_value(key, strlen(key), seed) :
                -1;
//      throw err::murmur3_32_runtime_error;
  }
}
