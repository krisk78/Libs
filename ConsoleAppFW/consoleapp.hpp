#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <filesystem>
#include <string>

#include "../usage/usage.hpp"

class ConsoleApp
{
public:
	ConsoleApp() = default;
#ifdef _WIN32
	ConsoleApp(bool windowsmode)
		: m_windowsmode(windowsmode) {};								// if windowsmode is true then messages are displayed in a modal window else to console
#endif // _WIN32
	bool windows_mode() { return m_windowsmode; }

	std::string Arguments(int argc, char* argv[]);						// Launches SetUsage and parses the command line performing standard usage controls, then launches Check_Arguments
	bool Arguments_Checked() { return m_argschecked; }					// In case of you need it, indicates if function Arguments was already launched
	std::unordered_map<std::string, std::vector<std::string>> values() const noexcept;
																		// Returns the values read for each argument
	std::vector<std::string> values(const std::string& name);			// Returns the values read for a single argument
	int Run();															// Runs the sequence PreProcess, ByFile and PostProcess and returns the number of files processed

protected:
	Usage	us{ "undefined" };											// Contains args and values

	virtual void SetUsage() = 0;										// Defines expected arguments and help.
	virtual std::string CheckArguments() { return ""; }					// Performs more accurate checks and initializations if necessary
	virtual void PreProcess() {};										// Launched before ByFile, do nothing by default
	virtual void MainProcess(const std::filesystem::path& file) {};		// Launched by ByFile for each file matching argument 'file' values
	virtual void PostProcess() {};										// Launched after ByFile, do nothing by default

private:
	bool m_argschecked{ false };
	bool m_windowsmode{ false };

	std::unordered_map<std::string, std::vector<std::string>> m_values{};
																		// stores values for each argument

	int ByFile();														// Calls MainProcess for each file matching argument 'file' values and returns the number of files processed
};