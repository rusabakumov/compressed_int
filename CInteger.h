#ifndef _CINTEGER_H_
#define _CINTEGER_H_

// C++ wrapper for cint library

#include "cint.h"
#include <string>

class CInteger {
private:
    std::string value;

    struct OfSize { size_t _s; explicit OfSize(size_t s) : _s(s) {}; };
    
    CInteger(const OfSize & sz) 
    {
        value.resize(sz._s);
    };
public:    
    CInteger() { };
    
    size_t size() const { return value.size(); };
    CInteger & squeeze() { value.resize(cint_get_size((const cint_t) value.data())); return *this; };
    
    CInteger(int x) 
    {
        value.resize(sizeof(x)); 
        cint_assign((cint_t) value.data(), x); 
    };

    CInteger(const CInteger & x) 
      : value(x.value)
    {
        squeeze();
    };

    CInteger & operator =(int x) 
    {
        value.resize(sizeof(x)); 
        cint_assign((cint_t) value.data(), x); 
        return *this;
    };
    
    CInteger & operator =(const CInteger & x) 
    {
        if (&x != this) {
            value = x.value;
        };
        return *this;
    };

    int compare(const CInteger & x) const
    {
        return cint_cmp((const cint_t) value.data(), (const cint_t) x.value.data());
    };
    
    bool operator ==(const CInteger & x) const { return compare(x) == 0; };
    bool operator !=(const CInteger & x) const { return compare(x) != 0; };
    bool operator < (const CInteger & x) const { return compare(x) < 0; };
    bool operator >(const CInteger & x) const { return compare(x) > 0; };
    bool operator >=(const CInteger & x) const { return compare(x) >= 0; };
    bool operator <=(const CInteger & x) const { return compare(x) <= 0; };
    
    CInteger operator +(const CInteger & x)
    {
        CInteger result(OfSize(std::max(this->size(), x.size()) + 2));
        cint_add((const cint_t) value.data(), (const cint_t) x.value.data(), (cint_t) result.value.data());
        return result;
    };

    CInteger operator -(const CInteger & x)
    {
        CInteger result(OfSize(std::max(this->size(), x.size()) + 2));
        cint_subtract((const cint_t) value.data(), (const cint_t) x.value.data(), (cint_t) result.value.data());
        return result;
    };  

    CInteger operator *(const CInteger & x)
    {
        CInteger result(OfSize(this->size() + x.size()));
        cint_mul((const cint_t) value.data(), (const cint_t) x.value.data(), (cint_t) result.value.data());
        return result.squeeze();
    };

    std::string str() const
    {
        std::string result;
        result.resize(MAX_NUMLEN);
        cint_to_str((const cint_t) value.data(), (char *) result.data());
        return result;
    };
};

#endif /* _CINTEGER_H_ */
