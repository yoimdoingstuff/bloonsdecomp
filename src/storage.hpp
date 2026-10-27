#pragma once
#include <stddef.h>
#include <stdint.h>
#ifndef BLOONS_EMBEDDED
#include <vector>
#include <deque>
#include <string>
#endif

namespace bloons {
// Aggregate layout preserves generated level/geometry initializers without a
// dependency on libstdc++ in firmware plugins.
template<class T,size_t N> struct Array {
    T values[N];
    constexpr T& operator[](size_t i) { return values[i]; }
    constexpr const T& operator[](size_t i) const { return values[i]; }
    constexpr size_t size() const { return N; }
    constexpr T* begin() { return values; }
    constexpr T* end() { return values+N; }
    constexpr const T* begin() const { return values; }
    constexpr const T* end() const { return values+N; }
};
template<class T> constexpr const T& minimum(const T& a,const T& b) { return b<a?b:a; }
template<class T> constexpr const T& maximum(const T& a,const T& b) { return a<b?b:a; }

#ifdef BLOONS_EMBEDDED
// Provided by the host. Capacity exhaustion must fail visibly, never silently
// drop projectiles or change a level's outcome.
[[noreturn]] void platformFailure(const char* message);
template<class T,size_t N> class BoundedList {
    T values[N]{};
    size_t count=0;
public:
    void clear() { count=0; }
    void push_back(const T& value) {
        if(count==N) platformFailure("Bloons entity capacity exceeded");
        values[count++]=value;
    }
    size_t size() const { return count; }
    bool empty() const { return count==0; }
    T& operator[](size_t i) { return values[i]; }
    const T& operator[](size_t i) const { return values[i]; }
    T& back() { return values[count-1]; }
    const T& back() const { return values[count-1]; }
    T* begin() { return values; }
    T* end() { return values+count; }
    const T* begin() const { return values; }
    const T* end() const { return values+count; }
};
template<class T,size_t N> using List=BoundedList<T,N>;
template<class T,size_t N> using StableList=BoundedList<T,N>;
using LevelTitle=const char*;
#else
template<class T,size_t N> using List=std::vector<T>;
template<class T,size_t N> using StableList=std::deque<T>;
using LevelTitle=std::string;
#endif
}
