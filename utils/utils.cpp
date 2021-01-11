// utils.cpp : Defines the functions for the static library.
//

#include "pch.h"

#include "utils.hpp"

std::string get_message(const char* fmt...)
// returns a string matching sprintf(buf, fmt...) - char * must be used for strings
{
    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);
    std::vector<char> msg(static_cast<size_t>(std::vsnprintf(nullptr, 0, fmt, args1)) + 1);
    va_end(args1);
    std::vsnprintf(msg.data(), msg.size(), fmt, args2);
    va_end(args2);
    std::string mesg(msg.begin(), msg.end());
    return mesg;
}

std::string to_lower(const std::string& str)
// converts str to lower case
{
    std::string s{ str };
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::string to_upper(const std::string& str)
// converts str to upper case
{
    std::string s{ str };
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    return s;
}

#ifdef _WIN32
std::wstring str_to_wstr(const std::string& str, unsigned int codepage)
{
    if (str.empty())
        return std::wstring();
    int len = MultiByteToWideChar(codepage, 0, &str[0], (int)str.size(), 0, 0);
    std::wstring result(len, 0);
    MultiByteToWideChar(codepage, 0, &str[0], (int)str.size(), &result[0], len);
    return result;
}
#endif // _WIN32

void replace_all(std::string& source, char replace, const std::string& replacement)
{
    size_t pos{ 0 };
    while (pos < source.length() && (pos = source.find(replace, pos)) != std::string::npos)
    {
        source.replace(pos, 1, replacement);
        pos += replacement.length();
    }
}

std::string to_regex_expression(const std::string& path)
{
    std::string expression{ "^" + path };
    replace_all(expression, '.', "\\.");
    replace_all(expression, '*', ".*");
    replace_all(expression, '?', "(.{1,1})");
    // TODO replacement of [..]
    return expression;
}

namespace fs = std::filesystem;

std::vector<fs::path> dir(const std::string& pattern)
{
#ifdef _WIN32
    const std::string wildcards{ "*?" };
#else
    const std::string wildcards{ "*?[" };
#endif // _WIN32

    std::vector<fs::path> result{};
    if (pattern.find_first_of(wildcards) == std::string::npos)
    {
        if (fs::exists(fs::path(pattern)))
            result.push_back(pattern);
        return result;
    }
    std::string directory{ "." };
    std::string filename{ pattern };
    while (!filename.empty() && (filename.back() == '\\' || filename.back() == '/'))
        filename.pop_back();        // ending path separators are ignore
    auto itr = filename.find_last_of("/\\");
    if (itr != std::string::npos)
    {
        directory = filename.substr(0, itr);
        filename = filename.substr(itr + 1, filename.size());
    }
    if (directory.find_first_of(wildcards) != std::string::npos || !fs::exists(fs::path(directory)))
        return result;          // wildcards are not allowed in parent path
    std::regex expr(to_regex_expression(filename), std::regex_constants::icase);
    for (auto it = fs::directory_iterator(directory); it != fs::directory_iterator(); ++it)
    {
        if (std::regex_match(it->path().filename().string(), expr))
            result.push_back(it->path());
    }
    return result;
}

EOL find_EOL(const std::vector<char>& buf)
{
    auto itr = std::find(buf.rbegin(), buf.rend(), '\r');
    if (itr != buf.rend())
    {
        if (itr != buf.rbegin())
        {
            itr--;
            if (*itr == '\n')
                return EOL::Windows;
        }
        return EOL::Mac;
    }
    itr = std::find(buf.rbegin(), buf.rend(), '\n');
    if (itr != buf.rend())
        return EOL::Unix;
    return EOL::Unknown;
}

EOL file_EOL(const std::filesystem::path& filepath)
{
    const std::ios::off_type BUF_LENGTH{ 4096 };

    if (!std::filesystem::exists(filepath))
        return EOL::Unknown;
    std::ifstream file(filepath, std::ios_base::binary | std::ios_base::in);
    if (auto fop = file.is_open(); !fop)
        return EOL::Unknown;
    file.seekg(0, std::ios::end);
    std::vector<char> buf(BUF_LENGTH);
    auto pos = file.tellg();
    if (pos > BUF_LENGTH)
    {
        pos = BUF_LENGTH;
        file.seekg(-pos, std::ios::end);         // first search at end of file
        file.read(buf.data(), pos);
        auto ret = find_EOL(buf);
        if (ret != EOL::Unknown)
        {
            file.close();
            return ret;
        }
    }
    file.seekg(std::ios::beg);                      // next search at begin of file
    file.read(buf.data(), pos);
    auto ret = find_EOL(buf);
    file.close();
    return ret;
}

size_t EOL_length(const EOL eol_type)
{
    switch (eol_type)
    {
        case EOL::Unknown:
            return 0;
        case EOL::Windows:
            return 2;
        default:
            return 1;
    }
}

std::string EOL_str(const EOL eol_type)
{
    switch (eol_type)
    {
        case EOL::Windows:
            return "\n\r";
        case EOL::Unix:
            return "\n";
        case EOL::Mac:
            return "\r";
        default:
            return "";
    }
}

std::vector<std::string> split(const std::string& s, const char delim)
{
    std::vector<std::string> result;
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim))
        result.push_back(item);
    return result;
}