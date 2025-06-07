#include "CusManager.h"

#include "Debug.h"
#include <fstream>
#include <ranges>

CusManager::CusManager(const std::filesystem::path& directory)
    : customizing_directory(directory) {
}

std::map<std::string, CusFile>& CusManager::GetFiles() {
	return files;
}

std::vector<char>& CusManager::GetFile(const std::string& relative_path) {
	files[relative_path].modified = true;
	return files[relative_path].data;
}

std::vector<std::string> CusManager::GetFileList() {
	std::vector<std::string> list;
	for (const auto& key : files | std::views::keys) {
		list.push_back(key);
	}
	return list;
}

size_t CusManager::Count() const {
	return files.size();
}

bool CusManager::ModifyRegion(CusFile& file, const std::string& region_name) {
	if (region_name.length() > 3) {
		DEBUG_LOG("Region name too long, shouldn't be longer than 3 characters.");
		return false;
	}

	file.region = region_name;

	return true;
}

bool CusManager::LoadRegion(CusFile& file) const {
	// Read region from bytes 0x08, 0x09, 0x0A
	file.region += static_cast<char>(static_cast<unsigned char>(file.data[0x08]));
	file.region += static_cast<char>(static_cast<unsigned char>(file.data[0x09]));
	file.region += static_cast<char>(static_cast<unsigned char>(file.data[0x0A]));

	// Validate region
	if (!available_regions.contains(file.region)) {
		file.invalid = true; // TODO: This is useless, file object goes into the void. IDC
		DEBUG_LOG("Warning: Invalid region '" << file.region << "' in " << file.path);
		return false;
	}

	return true;
}

bool CusManager::LoadFiles() {
	try {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(customizing_directory)) {
			if (entry.is_regular_file() && entry.path().extension() == ".cus") {
				CusFile file;
				file.path = entry.path();

				std::ifstream f(entry.path(), std::ios::binary);
				if (!f.is_open())
					continue;

				f.seekg(0, std::ios::end);
				size_t size = f.tellg();
				f.seekg(0, std::ios::beg);

				// Skipping super-small files.
				if (size < 0x0B) {
					continue;
				}

				file.data.resize(size);
				f.read(file.data.data(), size);
				f.close();

				// Skipping invalid files.
				if (!LoadRegion(file)) {
					continue;
				}

				std::string key = std::filesystem::relative(entry.path(), customizing_directory).string();
				files[key]      = std::move(file);
			}
		}

		DEBUG_LOG("Loaded " << files.size() << " .cus files");
		return !files.empty();
	} catch (const std::exception& e) {
		DEBUG_LOG("Error loading files: " << e.what());
		return false;
	}
}

void CusManager::SaveModified() {
	int saved = 0;
	for (auto& [path, data, region, modified, invalid] : files | std::views::values) {
		if (modified) {
			std::ofstream f(path, std::ios::binary);
			f.write(data.data(), data.size());
			f.close();
			saved++;
		}
	}
	DEBUG_LOG("Saved " << saved << " modified files");
}