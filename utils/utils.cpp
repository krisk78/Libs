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

bool checkDate(int day, int month, int year)
{
    if (day < 1 || day > 31)
        return false;
    if (month < 1 || month > 12)
        return false;
    if ((month == 4 || month == 6 || month == 9 || month == 11) && day == 31)
        return false;
    if (month == 2)
    {
        if (day == 30)
            return false;
        bool leap = (year % 4 == 0) && (year % 100 != 0) || (year % 400 == 0);
        if (!leap && day == 29)
            return false;
    }
    return true;
}

strDateConverter::strDateConverter()
{
    auto t = std::time(0);
    std::tm now;
    localtime_s(&now, &t);
    century = now.tm_year / 100 + 19;
}

bool strDateConverter::setToFmt(const std::string& fmt)
{
    bool m_valid = m_fromValid && setFmt(fmt, m_toFmt, m_toDelim, m_toPos, m_toLen, m_toSep, m_toValid);
    if (m_valid)
    {
        // defines the order of converted components based on their position in the format
        m_toOrder = { YEAR_COMP, MONTH_COMP, DAY_COMP };
        for (unsigned char i = 0; i < 2; i++)
            for (unsigned char j = i + 1; j < 3; j++)
                if (m_toPos[m_toOrder[j]] < m_toPos[m_toOrder[i]])
                    std::swap(m_toOrder[i], m_toOrder[j]);
    }
    return m_valid;
}

unsigned char strDateConverter::isValid(const unsigned char mode)
{
    unsigned char result = NONE;
    if (m_fromValid && (mode & FROM) == FROM)
        result = result | FROM;
    if (m_toValid && (mode & TO) == TO)
        result = result | TO;
    return result;
}

bool strDateConverter::checkStrDate(const std::string& str)
{
    if (!m_fromValid)
        return false;
    auto date = trimc(str);
    if (date.length() > m_fromFmt.length() + 5)
        return false;
    std::array<unsigned long, 3> compvals;
    compvals.fill(0);
    if (m_fromDelim)
    {
        std::vector<std::string> comps = split(date, m_fromSep);
        if (comps.size() != 3)
            return false;
        for (size_t i = 0; i < 3; i++)
        {
            auto comp = m_fromPos[i];
            size_t proc;
            try
            {
                compvals[i] = stoul(comps[comp], &proc);
            }
            catch (...)
            {
                return false;
            }
            if (compvals[i] < 0)
                return false;
            if ((m_fromLen[i] == 2 || m_fromLen[i] == 4) && proc != m_fromLen[i])
                return false;
        }
    }
    else
    {
        for (size_t i = 0; i < 3; i++)
        {
            auto comp = date.substr(m_fromPos[i], m_fromLen[i]);
            size_t proc;
            try
            {
                compvals[i] = stoul(comp, &proc);
            }
            catch (...)
            {
                return false;
            }
            if (compvals[i] < 0)
                return false;
            if ((m_fromLen[i] == 2 || m_fromLen[i] == 4) && proc != m_fromLen[i])
                return false;
        }
    }
    if (m_fromLen[YEAR_COMP] == 2)
        compvals[YEAR_COMP] += century;
    return checkDate(compvals[DAY_COMP], compvals[MONTH_COMP], compvals[YEAR_COMP]);
}

std::string strDateConverter::convStrDate(const std::string& date)
{
    std::string result{};
    std::vector<std::string> comps{};
    if (m_fromDelim)
    {
        auto comps2 = split(date, m_fromSep);
        if (comps2.size() != 3)
            return result;
        for (unsigned char i = 0; i < 3; i++)
        {
            auto comp = comps2[m_fromPos[i]];
            comps.push_back(comp);
        }
    }
    else
    {
        for (unsigned char i = 0; i < 3; i++)
        {
            if (date.length() > m_fromPos[i])
            {
                auto comp = date.substr(m_fromPos[i], m_fromLen[i]);
                comps.push_back(comp);
            }
        }
        if (comps.size() != 3)
            return result;
    }
    if (comps[YEAR_COMP].length() == 2)
        comps[YEAR_COMP].insert(0, std::to_string(century));
    for (unsigned char i = 0; i < 3; i++)
        if (comps[i].length() < m_toLen[i])
            comps[i].insert(0, 1, '0');         // no need to do it more than once
    for (unsigned char i = 0; i < 3; i++)
    {
        result += comps[m_toOrder[i]];
        if (i < 2 && m_toDelim)
            result += m_toSep;
    }
    return result;
}

bool strDateConverter::setFmt(const std::string& fmt, std::string& m_fmt, bool& m_delim, std::array<unsigned char, 3>& m_pos, std::array<unsigned char, 3>& m_len, char& m_sep, bool& m_val)
{
    m_val = true;
    m_delim = false;
    m_pos.fill(0);
    m_len.fill(0);
    auto fmtc = to_lower(trimc(fmt));
    m_fmt = fmtc;
    auto itr = fmtc.find_first_not_of("dmy");
    if (itr != std::string::npos)
    {
        m_delim = true;
        m_sep = fmtc[itr];
    }
    if (m_delim)
    {
        std::vector<std::string> comps = split(fmtc, m_sep);
        if (comps.size() != 3)
            return m_val = false;
        for (unsigned char i = 0; i < comps.size(); i++)
        {
            if (comps[i].length() == 0)
                return m_val = false;
            switch (comps[i][0])
            {
                case 'd':
                    itr = comps[i].find_first_not_of('d');
                    if (itr != std::string::npos || comps[i].length() > 2)
                        return m_val = false;
                    m_pos[DAY_COMP] = i;
                    m_len[DAY_COMP] = (unsigned char)comps[i].length();
                    break;
                case 'm':
                    itr = comps[i].find_first_not_of('m');
                    if (itr != std::string::npos || comps[i].length() > 2)
                        return m_val = false;
                    m_pos[MONTH_COMP] = i;
                    m_len[MONTH_COMP] = (unsigned char)comps[i].length();
                    break;
                case 'y':
                    itr = comps[i].find_first_not_of('y');
                    if (itr != std::string::npos || comps[i].length() == 3 || comps[i].length() > 4)
                        return m_val = false;
                    m_pos[YEAR_COMP] = i;
                    m_len[YEAR_COMP] = (unsigned char)comps[i].length();
            }
        }
    }
    else
    {
        auto curcomp = UNDEFINED_COMP;
        for (unsigned char i = 0; i < fmtc.length(); i++)
        {
            switch (fmtc[i])
            {
                case 'd':
                    if (curcomp != DAY_COMP)
                    {
                        m_pos[DAY_COMP] = i;
                        curcomp = DAY_COMP;
                    }
                    m_len[DAY_COMP]++;
                    break;
                case 'm':
                    if (curcomp != MONTH_COMP)
                    {
                        m_pos[MONTH_COMP] = i;
                        curcomp = MONTH_COMP;
                    }
                    m_len[MONTH_COMP]++;
                    break;
                case 'y':
                    if (curcomp != YEAR_COMP)
                    {
                        m_pos[YEAR_COMP] = i;
                        curcomp = YEAR_COMP;
                    }
                    m_len[YEAR_COMP]++;
            }
        }
        if (m_len[DAY_COMP] != 2 || m_len[MONTH_COMP] != 2 || m_len[YEAR_COMP] != 2 && m_len[YEAR_COMP] != 4)
            return m_val = false;
    }
    return m_val;
}
