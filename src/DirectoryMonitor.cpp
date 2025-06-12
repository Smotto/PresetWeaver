#include "DirectoryMonitor.h"

#include "Debug.h"

#include <fstream>
#include <functional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>

std::string DirectoryMonitor::ChangeInfo::TypeToString() const {
	switch (type) {
		case ADDED:
			return "ADDED";
		case MODIFIED:
			return "MODIFIED";
		case DELETED:
			return "DELETED";
		case RENAMED:
			return "RENAMED";
		default:
			return "UNKNOWN";
	}
}

DirectoryMonitor::DirectoryMonitor(const std::filesystem::path& path, bool recurse_subdirectories)
    : root_path(path), recurse_subdirectories(recurse_subdirectories), file_cache(ScanDirectory()) {
	if (!fs::exists(root_path) || !fs::is_directory(root_path)) {
		throw std::runtime_error("Invalid directory path: " + root_path.generic_string());
	}
}

DirectoryMonitor::~DirectoryMonitor() {
}

size_t DirectoryMonitor::GetFileCount() const {
	return file_cache.size();
}

std::vector<DirectoryMonitor::ChangeInfo> DirectoryMonitor::CheckForDirectoryChanges() {
	std::vector<ChangeInfo>                changes;
	auto                                   currentSnapshot = ScanDirectory();

	// Maps for tracking renames via file_id
	std::unordered_map<uint64_t, fs::path> old_id_to_path;
	std::unordered_map<uint64_t, fs::path> new_id_to_path;

	for (const auto& [path, info] : file_cache) {
		if (info.file_id != 0) {
			old_id_to_path[info.file_id] = path;
		}
	}

	for (const auto& [path, info] : currentSnapshot) {
		if (info.file_id != 0) {
			new_id_to_path[info.file_id] = path;
		}
	}

	// Detect modified and renamed files
	for (const auto& [file_id, new_path] : new_id_to_path) {
		auto old_it = old_id_to_path.find(file_id);
		if (old_it != old_id_to_path.end()) {
			const auto& old_path = old_it->second;

			const auto& old_info = file_cache.at(old_path);
			const auto& new_info = currentSnapshot.at(new_path);

			if (old_path != new_path) {
				changes.push_back({ ChangeInfo::RENAMED, new_path, old_path });
			} else if (old_info != new_info) {
				changes.push_back({ ChangeInfo::MODIFIED, new_path });
			}
		} else {
			changes.push_back({ ChangeInfo::ADDED, new_path });
		}
	}

	// Detect deleted files
	for (const auto& [file_id, old_path] : old_id_to_path) {
		if (!new_id_to_path.contains(file_id)) {
			changes.push_back({ ChangeInfo::DELETED, old_path });
		}
	}

	file_cache = std::move(currentSnapshot);
	return changes;
}

void DirectoryMonitor::ResetCache() {
	file_cache = ScanDirectory();
	DEBUG_LOG("DirectoryMonitor reset. Now tracking " << file_cache.size() << " items.");
}

std::unordered_map<std::filesystem::path, FileInfo> DirectoryMonitor::ScanDirectory() const {
	std::unordered_map<std::filesystem::path, FileInfo> current_files;

	try {
		auto process_entry = [&](const fs::directory_entry& entry) {
			try {
				if (entry.is_regular_file() && entry.path().extension() == ".cus") {
					current_files[entry.path()] = FileInfo(entry.path());
				}
			} catch (const fs::filesystem_error& error) {
				DEBUG_LOG(error.what());
			}
		};

		if (recurse_subdirectories) {
			for (const auto& entry : fs::recursive_directory_iterator(root_path, fs::directory_options::skip_permission_denied)) {
				process_entry(entry);
			}
		} else {
			for (const auto& entry : fs::directory_iterator(root_path, fs::directory_options::skip_permission_denied)) {
				process_entry(entry);
			}
		}
	} catch (const fs::filesystem_error& error) {
		DEBUG_LOG(error.what());
	}

	DEBUG_LOG("Scan complete. Cached " << file_cache.size() << " items.");

	return current_files;
}

void DirectoryMonitor::PrintChanges(const std::vector<ChangeInfo>& changes) {
	if (changes.empty()) {
		DEBUG_LOG("No changes detected.");
		return;
	}

	for (const auto& change : changes) {
		if (change.type == ChangeInfo::RENAMED) {
			DEBUG_LOG("[" << change.TypeToString() << "] " << change.old_path << " -> " << change.path);
		} else {
			DEBUG_LOG("[" << change.TypeToString() << "] " << change.path);
		}
	}
}
