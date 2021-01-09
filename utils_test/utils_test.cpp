#include "pch.h"
#include "../utils/utils.hpp"

TEST(Testing_get_message, Format_Is_Empty)
{
	const char* form = "";
	int i{ 5 };
	std::string astr{ "A test string" };
	auto m = get_message(form, i, astr.c_str());
	EXPECT_STREQ(m.c_str(), "");
}

TEST(Testing_get_message, Integers)
{
	const char* form = "Format test %i == %i";
	int i{ 5 };
	int j{ -5 };
	auto m = get_message(form, i, j);
	EXPECT_STREQ(m.c_str(), "Format test 5 == -5");
}

TEST(Testing_get_message, Strings)
{
	const char* form = "Format test %s follows %s.";
	std::string astr1{ "a test string" };
	std::string astr2{ "another test string" };
	auto m = get_message(form, astr1.c_str(), astr2.c_str());
	EXPECT_STREQ(m.c_str(), "Format test a test string follows another test string.");
}

TEST(Testing_get_message, Mix)
{
	const char* form = "Format test %i + %i = %s.";
	int i{ 5 };
	int j{ 6 };
	std::string astr{ "eleven" };
	auto m = get_message(form, i, j, astr.c_str());
	EXPECT_STREQ(m.c_str(), "Format test 5 + 6 = eleven.");
}

TEST(Testing_to_lower, String_Is_Empty)
{
	std::string str{ "" };
	auto m = to_lower(str);
	EXPECT_STREQ(m.c_str(), "");
}

TEST(Testing_to_lower, Mixed_String)
{
	std::string str{ "a StRiNg TeSt; It TaKeS 30 mInUtEs To RuN." };
	auto m = to_lower(str);
	EXPECT_STREQ(m.c_str(), "a string test; it takes 30 minutes to run.");
}

TEST(Testing_to_upper, String_Is_Empty)
{
	std::string str{ "" };
	auto m = to_upper(str);
	EXPECT_STREQ(m.c_str(), "");
}

TEST(Testing_to_upper, Mixed_String)
{
	std::string str{ "a StRiNg TeSt; It TaKeS 30 mInUtEs To RuN." };
	auto m = to_upper(str);
	EXPECT_STREQ(m.c_str(), "A STRING TEST; IT TAKES 30 MINUTES TO RUN.");
}

namespace fs = std::filesystem;

TEST(Dir_Test, Current_Path_Dir)
{
	fs::current_path(fs::path("H:/Windows/System32"));
	std::string testp{ "msxml?.*" };
	auto result{ dir(testp) };
	EXPECT_EQ(result.size(), 2);
	std::sort(result.begin(), result.end());
	if (result.size() == 2)
	{
		EXPECT_STREQ(result[0].filename().string().c_str(), "msxml3.dll");
		EXPECT_STREQ(result[1].filename().string().c_str(), "msxml6.dll");
	}
}

TEST(Dir_Test, Absolute_Path_Dir)
{
	std::string testp{ "H:\\Windows\\System32\\msxml?.*" };
	auto result{ dir(testp) };
	EXPECT_EQ(result.size(), 2);
	std::sort(result.begin(), result.end());
	if (result.size() == 2)
	{
		EXPECT_STREQ(result[0].filename().string().c_str(), "msxml3.dll");
		EXPECT_STREQ(result[1].filename().string().c_str(), "msxml6.dll");
	}
}

// TODO dir with patterns containing []

// TODO str_to_wstr tests

TEST(EOL_Test, Windows_File)
{
	std::filesystem::path p("C:/temp/windows.txt");
	EXPECT_EQ(file_EOL(p), EOL::Windows);
}

TEST(EOL_Test, Unix_File)
{
	std::filesystem::path p("C:/temp/unix.txt");
	EXPECT_EQ(file_EOL(p), EOL::Unix);
}
