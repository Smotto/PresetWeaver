#include "CusManager.h"

#include "Debug.h"
#include "OperatingSystemFunctions.h"

#include <fstream>
#include <ranges>
#include <utility>

CusManager::CusManager()
    : customizing_directory { OperatingSystemFunctions::FindLostArkCustomizationDirectory() },
      slint_model_unconverted_files { std::make_shared<slint::VectorModel<SlintCusFile>>() },
      internal_files { std::unordered_map<std::string, std::vector<std::unique_ptr<CusFile>>> {} } {
	LoadFilesFromDisk();
}

void CusManager::RefreshUnconvertedFiles(const std::string& selected_region) const {
	slint_model_unconverted_files->clear();

	// TODO: Maybe instead of having 1 source of unconverted files, we can have multiple regions have their own vector models. I'm lazy rn.
	for (const std::string& region : available_regions) {
		if (region == selected_region) {
			continue;
		}

		auto region_iterator = internal_files.find(region);
		if (region_iterator != internal_files.end()) {
			for (const auto& file_ptr : region_iterator->second) {
				if (file_ptr->region != selected_region) {
					slint_model_unconverted_files->push_back(SlintCusFile {
					    .path      = slint::SharedString(file_ptr->path_relative_to_customizing_directory.string()),
					    .region    = slint::SharedString(file_ptr->region),
					    .invalid   = file_ptr->invalid,
					    .data_size = static_cast<int>(file_ptr->data.size()) });
				}
			}
		}
	}
}

std::shared_ptr<slint::VectorModel<SlintCusFile>> CusManager::GetSlintModelUnconvertedFiles() {
	return slint_model_unconverted_files;
}

std::filesystem::path CusManager::GetCustomizingDirectory() const {
	return customizing_directory;
}

const std::unordered_map<std::string, std::vector<std::unique_ptr<CusFile>>>& CusManager::GetFiles() const {
	return internal_files;
}

bool CusManager::ConvertFilesToRegion(const std::string& region_name) {
	if (region_name.length() != 3) {
		DEBUG_LOG("Region name must be exactly 3 characters.");
		return false;
	}

	std::vector<CusFile*> files_to_save;

	for (const std::string& region : available_regions) {
		if (region == region_name) {
			continue;
		}

		auto region_iterator = internal_files.find(region);
		if (region_iterator != internal_files.end()) {
			auto& source_vector = region_iterator->second;
			for (auto file_iterator = source_vector.rbegin(); file_iterator != source_vector.rend();) {
				(*file_iterator)->region     = region_name;
				(*file_iterator)->data[0x08] = region_name[0];
				(*file_iterator)->data[0x09] = region_name[1];
				(*file_iterator)->data[0x0A] = region_name[2];

				files_to_save.push_back(file_iterator->get());
				internal_files[region_name].push_back(std::move(*file_iterator));
				file_iterator = std::reverse_iterator(source_vector.erase(std::next(file_iterator).base()));
			}
		}
	}

	return SaveFilesToDisk(files_to_save);
}

bool CusManager::LoadRegion(CusFile& file) const {
	file.region += static_cast<char>(static_cast<unsigned char>(file.data[0x08]));
	file.region += static_cast<char>(static_cast<unsigned char>(file.data[0x09]));
	file.region += static_cast<char>(static_cast<unsigned char>(file.data[0x0A]));

	if (!available_regions.contains(file.region)) {
		file.invalid = true;
		DEBUG_LOG("Warning: Invalid region '" << file.region << "' in " << file.path_relative_to_customizing_directory);
		return false;
	}

	return true;
}

bool CusManager::LoadFilesFromDisk() {
	try {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(customizing_directory)) {
			if (entry.is_regular_file() && entry.path().extension() == ".cus") {
				CusFile file;
				file.path_relative_to_customizing_directory = std::filesystem::relative(entry.path(), customizing_directory);

				std::ifstream f(entry.path(), std::ios::binary);
				if (!f.is_open())
					continue;

				f.seekg(0, std::ios::end);
				size_t size = f.tellg();
				f.seekg(0, std::ios::beg);

				if (size < 0x0B)
					continue;

				file.data.resize(size);
				f.read(file.data.data(), size);
				f.close();

				if (!LoadRegion(file))
					continue;

				internal_files[file.region].emplace_back(std::make_unique<CusFile>(std::move(file)));
			}
		}

		return !internal_files.empty();
	} catch (const std::exception& e) {
		DEBUG_LOG("Error loading files: " << e.what());
		return false;
	}
}

bool CusManager::SaveFilesToDisk(const std::vector<CusFile*>& modified_files) const {
	int saved = 0;

	for (const CusFile* file : modified_files) {
		std::filesystem::path file_write_out_path = customizing_directory / file->path_relative_to_customizing_directory;
		std::ofstream         f(file_write_out_path, std::ios::binary);
		f.write(file->data.data(), file->data.size());
		f.close();
		saved++;
	}

	DEBUG_LOG("Saved " << saved << " modified files to disk.");
	return true;
}
