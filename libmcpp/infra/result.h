#ifndef _M_RESULT_H_
#define _M_RESULT_H_

#include <string.h>
#include <exception.h>

namespace upan {

template <typename T>
class error
{
  private:
    T _value;
    string _reason;
  private:
    error() {}
  public:
    error(const T& value, const string& reason) : _value(value), _reason(reason) {}
    const T& value() const { return _value; }
    const string& reason() const { return _reason; }

  template<typename G, typename B> friend class result;
};

template <typename Good, typename Bad>
class result
{
  private:
    bool _isBad;
    Good _value;
    error<Bad> _error;
  public:
    result(const Good& value) : _isBad(false), _value(value) {}
    result(const error<Bad>& error) : _isBad(true), _error(error) {}
    
    bool isBad() const { return _isBad; }
    bool isGood() const { return !isBad(); }

    const Good& goodValue() const
    {
      if(_isBad)
        throw exception(XLOC, "result is Bad - can't get Good value");
      return _value;
    }

    const error<Bad>& badValue() const
    {
      if(!_isBad)
        throw exception(XLOC, "result is Good - can't get Error");
      return _error;
    }
};

}

#endif
