#include "CusManager.h"

#include "Debug.h"
#include "DirectoryMonitor.h"
#include "OperatingSystemFunctions.h"

#include <fstream>
#include <ranges>
#include <utility>

CusManager::CusManager(slint::ComponentHandle<AppWindow> ui)
    : ui_handle(std::move(ui)),
      selected_region(OperatingSystemFunctions::GetLocalizationRegion()),
      customizing_directory(OperatingSystemFunctions::FindLostArkCustomizationDirectory()),
      directory_monitor(std::make_unique<DirectoryMonitor>(customizing_directory, true)),
      slint_models_by_excluded_region {
	      { "USA", std::make_shared<slint::VectorModel<SlintCusFile>>() },
	      { "KOR", std::make_shared<slint::VectorModel<SlintCusFile>>() },
	      { "RUS", std::make_shared<slint::VectorModel<SlintCusFile>>() }
      },
      region_files_map(std::unordered_map<std::string, std::vector<std::unique_ptr<CusFile>>> {}) {

	LoadFilesFromDisk();

	StartMonitorThread();
}

CusManager::~CusManager() {
	StopMonitorThread();
}

void CusManager::LoadFile(const std::filesystem::path& full_path) {
	if (full_path.extension() != ".cus")
		return;

	CusFile file;
	file.path_relative_to_customizing_directory = std::filesystem::relative(full_path, customizing_directory);

	for (auto& [region, vec] : region_files_map) {
		auto it = std::remove_if(vec.begin(), vec.end(), [&](const std::unique_ptr<CusFile>& f) {
			return f->path_relative_to_customizing_directory == file.path_relative_to_customizing_directory;
		});
		if (it != vec.end()) {
			vec.erase(it, vec.end());
		}
	}

	std::ifstream f(full_path, std::ios::binary);
	if (!f.is_open())
		return;

	f.seekg(0, std::ios::end);
	size_t size = f.tellg();
	if (size < 0x0B)
		return;

	f.seekg(0, std::ios::beg);
	file.data.resize(size);
	f.read(file.data.data(), size);
	f.close();

	if (!LoadRegion(file))
		return;

	auto& vec = region_files_map[file.region];
	for (const auto& existing : vec) {
		if (existing->path_relative_to_customizing_directory == file.path_relative_to_customizing_directory)
			return; // Already present
	}

	vec.emplace_back(std::make_unique<CusFile>(std::move(file)));
}

void CusManager::RemoveFile(const std::filesystem::path& full_path) {
	auto rel_path = std::filesystem::relative(full_path, customizing_directory);

	for (auto& [region, vec] : region_files_map) {
		auto it = std::remove_if(vec.begin(), vec.end(), [&](const std::unique_ptr<CusFile>& f) {
			return f->path_relative_to_customizing_directory == rel_path;
		});
		if (it != vec.end()) {
			vec.erase(it, vec.end());
			break;
		}
	}
}

void CusManager::RefreshUnconvertedFiles(const std::string& excluded_region) const {
	auto it = slint_models_by_excluded_region.find(excluded_region);
	if (it != slint_models_by_excluded_region.end()) {
		it->second->clear();

		for (const std::string& region : available_regions) {
			if (region == excluded_region) {
				continue;
			}

			auto region_iterator = region_files_map.find(region);
			if (region_iterator != region_files_map.end()) {
				for (const auto& file_ptr : region_iterator->second) {
					it->second->push_back(SlintCusFile { .path = slint::SharedString(file_ptr->path_relative_to_customizing_directory.string()), .region = slint::SharedString(file_ptr->region), .data_size = static_cast<int>(file_ptr->data.size()), .invalid = file_ptr->invalid });
				}
			}
		}

		if (excluded_region == "USA") {
			ui_handle->global<GlobalVariables>().set_files_excluding_USA(it->second);
		} else if (excluded_region == "KOR") {
			ui_handle->global<GlobalVariables>().set_files_excluding_KOR(it->second);
		} else if (excluded_region == "RUS") {
			ui_handle->global<GlobalVariables>().set_files_excluding_RUS(it->second);
		}
	}
}

std::shared_ptr<slint::VectorModel<SlintCusFile>> CusManager::GetSlintModelFiles(const std::string& excluded_region) {
	return slint_models_by_excluded_region.at(excluded_region);
}

std::filesystem::path CusManager::GetCustomizingDirectory() const {
	return customizing_directory;
}

const std::unordered_map<std::string, std::vector<std::unique_ptr<CusFile>>>& CusManager::GetFiles() const {
	return region_files_map;
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

		auto region_iterator = region_files_map.find(region);
		if (region_iterator != region_files_map.end()) {
			auto& source_vector = region_iterator->second;
			for (auto file_iterator = source_vector.rbegin(); file_iterator != source_vector.rend();) {
				CusFile* file = file_iterator->get();

				if (file->data.size() < 0x0B) {
					DEBUG_LOG("Skipping incomplete file during conversion: " << file->path_relative_to_customizing_directory);
					++file_iterator;
					continue;
				}

				auto file_ptr = std::move(*file_iterator);
				file_iterator = std::reverse_iterator(source_vector.erase(std::next(file_iterator).base()));

				file_ptr->region     = region_name;
				file_ptr->data[0x08] = region_name[0];
				file_ptr->data[0x09] = region_name[1];
				file_ptr->data[0x0A] = region_name[2];

				files_to_save.push_back(file_ptr.get());
				region_files_map[region_name].push_back(std::move(file_ptr));
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

void CusManager::StartMonitorThread() {
	monitor_thread = std::thread([this]() {
		DEBUG_LOG("CusManager monitoring thread started.");
		std::unique_lock<std::mutex> lock(monitor_mutex);

		while (file_handling_active) {
			monitor_condition_variable.wait_for(lock, std::chrono::milliseconds(100), [this]() {
				return !file_handling_active.load();
			});

			if (!file_handling_active)
				break;

			lock.unlock();

			auto changes = directory_monitor->CheckForDirectoryChanges();
			if (!changes.empty()) {
				std::lock_guard<std::mutex> file_lock(file_mutex);
				for (auto iterator = changes.rbegin(); iterator != changes.rend(); ++iterator) {
					{
						std::lock_guard<std::mutex> lock(recently_modified_mutex);
						if (recently_modified_paths.contains(std::filesystem::weakly_canonical(iterator->path))) {
							recently_modified_paths.erase(iterator->path); // Avoid skipping forever
							continue;
						}
					}

					switch (iterator->type) {
						case DirectoryMonitor::ChangeInfo::ADDED:
						case DirectoryMonitor::ChangeInfo::MODIFIED:
							LoadFile(iterator->path);
							break;
						case DirectoryMonitor::ChangeInfo::DELETED:
							RemoveFile(iterator->path);
							break;
						case DirectoryMonitor::ChangeInfo::RENAMED:
							RemoveFile(iterator->old_path);
							LoadFile(iterator->path);
							break;
					}
				}

				std::string region_copy = GetSelectedRegionSafe();
				slint::invoke_from_event_loop([this, region_copy]() {
					std::lock_guard<std::mutex> lock(conversion_mutex);
					RefreshUnconvertedFiles(region_copy);
					if (automatic_conversion_enabled.load()) {
						if (ConvertFilesToRegion(region_copy)) {
							DEBUG_LOG("Converted files to region: " << region_copy);
						}
					}
				});
			}

			lock.lock();
		}

		DEBUG_LOG("CusManager monitoring thread exiting.");
	});
}

void CusManager::StopMonitorThread() {
	file_handling_active = false;
	monitor_condition_variable.notify_all(); // Wake up thread if paused
	if (monitor_thread.joinable()) {
		monitor_thread.join();
	}
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

				region_files_map[file.region].emplace_back(std::make_unique<CusFile>(std::move(file)));
			}
		}

		return !region_files_map.empty();
	} catch (const std::exception& e) {
		DEBUG_LOG("Error loading files: " << e.what());
		return false;
	}
}

bool CusManager::SaveFilesToDisk(const std::vector<CusFile*>& modified_files) {
	int saved = 0;

	for (const CusFile* file : modified_files) {
		std::filesystem::path file_write_out_path = customizing_directory / file->path_relative_to_customizing_directory;

		if (!std::filesystem::exists(file_write_out_path)) {
			DEBUG_LOG("Skipping write: file was deleted -> " << file_write_out_path);
			continue;
		}

		std::ofstream f(file_write_out_path, std::ios::binary);
		f.write(file->data.data(), file->data.size());
		f.close();

		{
			std::lock_guard<std::mutex> lock(recently_modified_mutex);
			recently_modified_paths.insert(std::filesystem::weakly_canonical(file_write_out_path));
		}

		saved++;
	}

	DEBUG_LOG("Saved " << saved << " modified files to disk.");
	return true;
}

void CusManager::SetSelectedRegionSafe(const std::string& region) {
	std::lock_guard<std::mutex> lock(selected_region_mutex);
	selected_region = region;
}

std::string CusManager::GetSelectedRegionSafe() {
	std::lock_guard<std::mutex> lock(selected_region_mutex);
	return selected_region;
}

bool CusManager::GetAutomaticConversionEnabled() const {
	return automatic_conversion_enabled.load();
}

void CusManager::SetAutomaticConversionEnabled(bool enabled) {
	automatic_conversion_enabled.store(enabled);
}
