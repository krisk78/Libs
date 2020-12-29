// consoleapp.cpp : Defines the functions for the static library.
//

#include "pch.h"

#include "consoleapp.hpp"
#include "../utils/utils.hpp"

std::string ConsoleApp::Arguments(int argc, char* argv[])
{
	assert(!m_argschecked && "Arguments checks were already performed.");	// Arguments function should be called once
	SetUsage();
	if (us.program_name == "undefined")
	{
		us.program_name = argv[0];
		auto itr = us.program_name.find_last_of("/\\");
		if (itr != std::string::npos)
			us.program_name = us.program_name.substr(itr + 1, us.program_name.size() - itr);
	}
	auto msg = us.set_parameters(argc, argv);
	if (msg == "?")
	{
#ifdef _WIN32
		if (m_windowsmode)
		{
			std::ostringstream os;
			os << us;
			MessageBox(NULL, str_to_wstr(os.str(), CP_ACP).c_str(), str_to_wstr(us.program_name, CP_ACP).c_str(), MB_ICONINFORMATION | MB_OK);
		}
		else
#endif // _WIN32
			std::cout << us;
		return msg;				// m_argschecked is not set to true because in this case there is nothing to run
	}
	if (msg.empty())
	{
		m_values = us.get_values();
		msg = CheckArguments();
	}
	if (!msg.empty())
	{
#ifdef _WIN32
		if (m_windowsmode)
			MessageBox(NULL, str_to_wstr(msg, CP_ACP).c_str(), str_to_wstr(us.program_name, CP_ACP).c_str(), MB_ICONERROR | MB_OK);
		else
#endif // _WIN32
			std::cout << msg;
	}
	m_argschecked = true;
	return msg;
}

std::unordered_map<std::string, std::vector<std::string>> ConsoleApp::values() const noexcept
{
	assert(!m_values.empty() && "Attempt to get values before processing command line arguments.");
	return m_values;
}

std::vector<std::string> ConsoleApp::values(const std::string& name)
{
	assert(!m_values.empty() && "Attempt to get values before processing command line arguments.");
	return us.get_values(name);
}

int ConsoleApp::Run()
{
	assert(m_argschecked && "Arguments must be parsed and checked first.");
	int nbfiles{ 0 };
	PreProcess();
	try {
		nbfiles = ByFile();
	}
	catch (const std::exception&) {
		throw;
	}
	PostProcess();
	return nbfiles;
}

int ConsoleApp::ByFile()
{
	int nbfiles{ 0 };
	auto itr = m_values.find("file");
	if (itr == m_values.end())
		return nbfiles;
	for (auto value : (*itr).second)
	{
		auto files = dir(value);
		for (auto file : files)
		{
			MainProcess(file);
			nbfiles++;
		}
	}
	if (nbfiles == 0)
		throw std::filesystem::filesystem_error("No matching file.", std::make_error_code(std::errc::no_such_file_or_directory));
	return nbfiles;
}