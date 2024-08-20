#ifndef Variant_H
#define Variant_H

#include <string>
#include <sstream>

using namespace std;

struct Variant: public string {
    template<typename T>
    Variant(const T &t) : Variant(to_string(t))
    {
    }

    Variant(const bool &t) : Variant(t ? to_string(1) : to_string(0))
    {
    }

    template<size_t N>
    Variant(const char (&s)[N]) : string(s, N) 
    {
    }

    Variant(const char *cstr) : string(cstr)
    {
    }

    Variant(const string &other = string()) : string(other)
    {
    }

    template<typename T>
    operator T() const
    {
        T t;
        stringstream ss;
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