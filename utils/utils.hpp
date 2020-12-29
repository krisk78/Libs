#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <filesystem>
#include <string>
#include <vector>

std::string get_message(const char* fmt...);
// returns a string matching sprintf(buf, fmt...) - char * must be used for strings

std::string to_lower(const std::string& str);
// converts str to lower case

std::string to_upper(const std::string& str);
// converts str to upper case

#ifdef _WIN32
std::wstring str_to_wstr(const std::string& str, unsigned int codepage);
// converts string to utf-16 wide string
#endif // _WIN32

std::vector<std::filesystem::path> dir(const std::string& pattern);
// returns list of paths matching pattern that can contain *, ? (Windows, POSIX) and [] (POSIX)
