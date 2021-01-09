#pragma once

/*! \file utils.hpp
*	\brief The Utils library provides few functions for basic purposes. See functions documentation for details.
*	\author Christophe COUAILLET
*/

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <filesystem>
#include <string>
#include <vector>

/*! \brief Defines the types of end of line EOL */
enum class EOL
{
	Unknown,
	Windows,		// CR + LF
	Unix,			// LF
	Mac				// CR
};

/*! \brief Returns a string matching sprintf(buf, fmt...) - c-string type (char *) must be used for strings.
	\param fmt...	list of parameters in the sprintf style: the 1st is the format string and nexts are the variables to print as specified by the format string.
	\sa Refer to <a href="https://en.cppreference.com/w/cpp/io/c/fprintf" target="_blank">cppreference.com</a> for details.
*/
std::string get_message(const char* fmt...);

/*!	\brief Converts str to lower case
*/
std::string to_lower(const std::string& str);

/*!	\brief Converts str to upper case
*/
std::string to_upper(const std::string& str);

#ifdef _WIN32
/*!	\brief Converts a string to utf-16 wide string using the specified codepage.
	\warning This function is only implemented for Windows platforms.
	\param codepage This function uses the function MultiByteToWideChar.
	\sa Refer to <a href="https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar" target="_blank">MultiByteToWideChar function (stringapiset.h)</a> (Microsoft)
	for details on codepage identifiers.

	Note that the result string must be converted to a c-string with std::string::c_str() to get a LPCWSTR string.
*/
std::wstring str_to_wstr(const std::string& str, unsigned int codepage);
#endif // _WIN32

/*!	\brief Returns a list of paths matching the specified pattern that can contain *, ? (Windows, POSIX) and [..] (POSIX).

	Ending path separators are ignored. Wildcards are only allowed in the filename part of the path.
	\warning Wildcard [..] support is not yet implemented.
*/
std::vector<std::filesystem::path> dir(const std::string& pattern);

/*! \brief Detects the EOL char(s) used in a file.
	Returns EOL::Unknown if the file does not exist or if EOL chars are not found in the 4096 bytes at end or begin of the file.
*/
EOL file_EOL(const std::filesystem::path& filepath);

/*! \brief Returns the length of the EOL type. */
size_t EOL_length(const EOL eol_type);

/*! \brief Returns the chars sequence of the EOL type. */
std::string EOL_str(const EOL eol_type);
