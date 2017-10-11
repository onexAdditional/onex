#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdexcept>
#include <string>

namespace onex {

class OnexException : public std::runtime_error
{
public:
  OnexException(const char* msg) : std::runtime_error(msg) {}
  OnexException(std::string msg) : std::runtime_error(msg.c_str()) {}
};

} // namespace onex

#endif // EXCEPTION_H
