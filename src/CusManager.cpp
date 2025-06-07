#include "CusManager.h"

#include "Debug.h"
#include <fstream>
#include <ranges>

CusManager::CusManager(const std::filesystem::path& customizing_directory)
    : customizing_directory(customizing_directory), slint_model(std::make_shared<slint::VectorModel<SlintCusFile>>()) {
}

void CusManager::AddFile(const CusFile& file) {
	internal_files.push_back(file);

	slint_model->push_back(SlintCusFile {
	    .path      = slint::SharedString(file.path_relative_to_customizing_directory.string()),
	    .region    = slint::SharedString(file.region),
	    .modified  = file.modified,
	    .invalid   = file.invalid,
	    .data_size = static_cast<int>(file.data.size())
	});
}

void CusManager::UpdateFile(const CusFile& file, size_t index) {
	if (index < internal_files.size()) {
		internal_files[index] = file;

		// Update Slint Model
		slint_model->set_row_data(index, SlintCusFile{
			.path = slint::SharedString(file.path_relative_to_customizing_directory.string()),
				.region = slint::SharedString(file.region),
				.modified = file.modified,
				.invalid = file.invalid,
				.data_size = static_cast<int>(file.data.size())
		});
	}
}

std::shared_ptr<slint::VectorModel<SlintCusFile>> CusManager::GetSlintModel() {
	return slint_model;
}

CusFile& CusManager::GetInternalFile(size_t index) {
	return internal_files[index];
}

std::vector<CusFile>& CusManager::GetFiles() {
	return internal_files;
}

size_t CusManager::Count() const {
	return internal_files.size();
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
		DEBUG_LOG("Warning: Invalid region '" << file.region << "' in " << file.path_relative_to_customizing_directory);
		return false;
	}

	return true;
}

bool CusManager::LoadFiles() {
	try {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(customizing_directory)) {
			if (entry.is_regular_file() && entry.path().extension() == ".cus") {
				CusFile file;
				file.path_relative_to_customizing_directory = std::filesystem::relative(entry.path(), customizing_directory).string();

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

				AddFile(file);
			}
		}

		DEBUG_LOG("Loaded " << internal_files.size() << " .cus files");
		return !internal_files.empty();
	} catch (const std::exception& e) {
		DEBUG_LOG("Error loading files: " << e.what());
		return false;
	}
}

void CusManager::SaveModified() const {
	int saved = 0;
	for (auto& file : internal_files) {
		if (file.modified) {
			// TODO: Needs customizing directory + relative path to customizing directory
			std::string file_write_out_path;
			std::ofstream f(file.path_relative_to_customizing_directory, std::ios::binary);
			f.write(file.data.data(), file.data.size());
			f.close();
			saved++;
		}
	}
	DEBUG_LOG("Saved " << saved << " modified files");
}