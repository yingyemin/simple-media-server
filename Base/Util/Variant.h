#ifndef Variant_H
#define Variant_H

#include <string>
#include <sstream>

// using namespace std;

struct Variant: public std::string {
    template<typename T>
    Variant(const T &t) : Variant(std::to_string(t))
    {
    }

    Variant(const bool &t) : Variant(t ? std::to_string(1) : std::to_string(0))
    {
    }

    template<size_t N>
    Variant(const char (&s)[N]) : std::string(s, N) 
    {
    }

    Variant(const char *cstr) : std::string(cstr)
    {
    }

    Variant(const std::string &other = std::string()) : std::string(other)
    {
    }

    template<typename T>
    operator T() const
    {
        T t;
        std::stringstream ss;
        return ss << *this && ss >> t ? t : T();
    }

    template<typename T> bool operator ==(const T &t) const
    {
        return 0 == this->compare(variant(t));
    }

    bool operator ==(const char *t) const
    {
        return this->compare(t) == 0;
    }

    template<typename T>
    T as() const
    {
        return (T) (*this);
    }
};

#endif