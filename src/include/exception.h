// Creator: Saad AIT YAHIA - @github: Saad-programmer
#pragma once

#include <exception>
#include <string>

class PException: public std::exception
{
public:

    PException(const std::string &text):
        text(text)
    {
    }

    const char * what() const noexcept
    {
        return text.c_str();
    }

private:

    std::string text;
};

/// Helper macros to convert `__LINE__` into a string literal.
#define MAKESTRING2(x)          #x
#define MAKESTRING(x)           MAKESTRING2(x)

// Helper macro to use when throwing a `PException`.
#define MakePException(text)    PException(std::string() + text + std::string(" at " __FILE__ ":" MAKESTRING(__LINE__)))

