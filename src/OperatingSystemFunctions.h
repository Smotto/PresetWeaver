#ifndef OPERATINGSYSTEMFUNCTIONS_H_
#define OPERATINGSYSTEMFUNCTIONS_H_

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <vector>
#include <windows.h>

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

	static std::filesystem::path FindLostArkCustomizationDirectory(const std::vector<std::filesystem::path>& paths = RetrieveSteamLibraryPaths()) {
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
		if (!GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH)) {
			return "USA";
		}

		const int size_needed = WideCharToMultiByte(CP_UTF8, 0, localeName, -1, nullptr, 0, nullptr, nullptr);
		if (size_needed <= 1) { // <= 1 because we need at least 1 char + null terminator
			return "USA";
		}

		std::string locale(size_needed - 1, '\0'); // -1 to exclude null terminator
		WideCharToMultiByte(CP_UTF8, 0, localeName, -1, locale.data(), size_needed, nullptr, nullptr);

		const std::string_view locale_view { locale };

		if (locale_view.find("en-US") != std::string_view::npos)
			return "USA";
		if (locale_view.find("ko-KR") != std::string_view::npos)
			return "KOR";
		if (locale_view.find("ru-RU") != std::string_view::npos)
			return "RUS";

		return "USA"; // default fallback
	}
} // namespace OperatingSystemFunctions

#endif /* OPERATINGSYSTEMFUNCTIONS_H_ */
