#include "CusManager.h"
#include "Debug.h"

#include <app-window.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <vector>
#include <windows.h>

// Automatic detection region of file
// Default region based on computer's network region.
// A button to change to convert file in-place (temporary full in RAM).

// Cases: RUS, USA, KOR

/* Windows only */
namespace OperatingSystemFunctions {
	static std::string GetSteamInstallPath() {
		std::cout << "Finding Windows Steam Install Path....." << "\n";

		HKEY        hKey;
		const char* subkey = "SOFTWARE\\WOW6432Node\\Valve\\Steam";
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
			return "";

		char  path[MAX_PATH];
		DWORD size = sizeof(path);
		if (RegQueryValueExA(hKey, "InstallPath", nullptr, nullptr, reinterpret_cast<LPBYTE>(path), &size) !=
		    ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return "";
		}

		RegCloseKey(hKey);
		return std::string(path);
	}

	static void ReplaceDoubleBackslashWithSlash(std::string& input_string) {
		std::string::size_type position = 0;
		while ((position = input_string.find("\\\\", position)) != std::string::npos) {
			input_string.replace(position, 2, "\\");
			++position;
		}
	}

	static std::vector<std::filesystem::path> ParseSteamLibraryVDF(const std::filesystem::path& path_to_vdf) {
		std::vector<std::filesystem::path> library_paths;
		std::ifstream                      file(path_to_vdf);

		if (!file.is_open()) {
			std::cerr << "Failed to open VDF file: " << path_to_vdf << "\n";
			return library_paths;
		}

		std::string line;
		int         depth = 0;

		while (std::getline(file, line)) {
			// Remove leading and trailing whitespace
			std::string not_of_pattern = " \t\r\n";
			line.erase(0, line.find_first_not_of(not_of_pattern));
			line.erase(line.find_last_not_of(not_of_pattern) + 1);
			if (line.empty())
				continue;

			// Count depth with braces
			for (const char c : line) {
				if (c == '{') {
					depth++;
				}
				if (c == '}') {
					depth--;
				}
			}

			// Only look for path inside entries like "0", "1", etc. which are depth level 2
			if (depth == 2 && line.find("\"path\"") == 0) {
				size_t first_quote  = line.find('"', 6);
				size_t second_quote = line.find('"', first_quote + 1);
				if (first_quote != std::string::npos && second_quote != std::string::npos) {
					std::string raw_path = line.substr(first_quote + 1, second_quote - first_quote - 1);
					ReplaceDoubleBackslashWithSlash(raw_path);
					std::filesystem::path corrected_path = std::filesystem::path(raw_path) / "steamapps" / "common";
					std::cout << corrected_path.generic_string() << "\n";
					library_paths.emplace_back(corrected_path);
				}
			}
		}

		file.close();
		return library_paths;
	}

	static std::vector<std::filesystem::path> RetrieveSteamLibraryPaths() {
		std::filesystem::path steam_path               = GetSteamInstallPath();
		std::filesystem::path library_folders_vdf_path = steam_path / "steamapps" / "libraryfolders.vdf";

		std::cout << "Reading: " << library_folders_vdf_path.string() << "\n";

		std::vector<std::filesystem::path> paths = ParseSteamLibraryVDF(library_folders_vdf_path);

		return paths;
	}

	static std::filesystem::path FindLostArkCustomizationDirectory(const std::vector<std::filesystem::path>& paths) {
		std::filesystem::path found_path;
		for (const auto& path : paths) {
			std::filesystem::path desired_path = path / "Lost Ark" / "EFGame" / "Customizing";
			if (std::filesystem::is_directory(desired_path)) {
				std::cout << "LOA Customizing directory found in " << desired_path.generic_string() << "\n";
				found_path = desired_path;
				break;
			}
		}

		return found_path;
	}

	static std::string GetLocalizationRegion() {
		wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
		if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH)) {
			std::wstring wlocale(localeName);
			std::string  locale(wlocale.begin(), wlocale.end());

			if (locale.find("en-US") != std::string::npos)
				return "USA";
			if (locale.find("ko-KR") != std::string::npos)
				return "KOR";
			if (locale.find("ru-RU") != std::string::npos)
				return "RUS";
		}

		return "USA"; // default fallback
	}
} // namespace OperatingSystemFunctions

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	_putenv("SLINT_BACKEND=winit-skia");
	// Allocate a console for this GUI application
	AllocConsole();

	// Redirect stdout, stdin, stderr to console
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
	freopen_s((FILE**)stdin, "CONIN$", "r", stdin);

	// Make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
	// point to console as well
	std::ios::sync_with_stdio(true);

	// Now you can use printf, cout, etc.
	std::cout << "Debug console is now available!" << std::endl;
	printf("Command line: %s\n", lpCmdLine);

	// TODO: move operating system functions somewhere else
	std::vector<std::filesystem::path> steam_library_paths = OperatingSystemFunctions::RetrieveSteamLibraryPaths();
	std::filesystem::path              customizing_directory_path    = OperatingSystemFunctions::FindLostArkCustomizationDirectory(steam_library_paths);

	DEBUG_LOG("Customizing path" << customizing_directory_path);

	if (customizing_directory_path.empty()) {
		DEBUG_LOG("Lost Ark customizing directory not found!");
		return EXIT_FAILURE;
	}

	std::unique_ptr<CusManager> cus_file_manager = std::make_unique<CusManager>(customizing_directory_path);

	if (!cus_file_manager->LoadFiles()) {
		DEBUG_LOG("Failed to load .cus files!");
		return EXIT_FAILURE;
	}

	std::vector<CusFile> files = cus_file_manager->GetFiles();
	auto ui = AppWindow::create();

	ui->global<GlobalVariables>().set_files(cus_file_manager->GetSlintModel());
	ui->global<GlobalVariables>().set_local_region(slint::SharedString(OperatingSystemFunctions::GetLocalizationRegion()));
	ui->run();

	return 0;
}