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

	// Take initial snapshot
	file_cache = ScanDirectory();
	DEBUG_LOG("Initial scan complete. Monitoring " << file_cache.size() << " items.");

	StartMonitorThread();
}

DirectoryMonitor::~DirectoryMonitor() {
	StopMonitorThread();
}

void DirectoryMonitor::StartMonitorThread() {
	monitor_thread = std::thread {&DirectoryMonitor::BackgroundTask, this};
}

void DirectoryMonitor::StopMonitorThread() {
	monitor_thread_should_run = false;
	monitor_thread_condition_variable.notify_all(); // Wake up thread if paused
	if (monitor_thread.joinable()) {
		monitor_thread.join();
	}
}

void DirectoryMonitor::ResumeMonitoring() {
	{
		monitoring_active = true;
		DEBUG_LOG("Monitoring is now active!");
	}
	monitor_thread_condition_variable.notify_all();
}

void DirectoryMonitor::StopMonitoring() {
	{
		monitoring_active = false;
		DEBUG_LOG("Monitoring deactivated.");
	}
	monitor_thread_condition_variable.notify_all();
}

bool DirectoryMonitor::IsMonitoringActive() const {
	return monitoring_active.load();
}

void DirectoryMonitor::BackgroundTask() {
	std::unique_lock<std::mutex> lock(monitor_thread_mutex);

	DEBUG_LOG("Background Task started.");

	while (monitor_thread_should_run) {
		monitor_thread_condition_variable.wait(lock, [this] {
			return monitoring_active || !monitor_thread_should_run;
		});

		if (!monitor_thread_should_run) {
			break;
		}

		lock.unlock();

		std::vector<ChangeInfo> changes = CheckForDirectoryChanges();
		if (!changes.empty()) {
			PrintChanges(changes);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(interval));

		lock.lock();
	}

	DEBUG_LOG("Background Task stopped. Thread is exiting.");
}

std::vector<DirectoryMonitor::ChangeInfo> DirectoryMonitor::CheckForDirectoryChanges() {
	std::vector<ChangeInfo> changes;
	auto                    currentSnapshot = ScanDirectory();

	// Collect potential additions and deletions first
	std::vector<ChangeInfo> potential_adds;
	std::vector<ChangeInfo> potential_deletes;

	// Check for new and modified files
	for (const auto& [path, currentInfo] : currentSnapshot) {
		auto it = file_cache.find(path);
		if (it == file_cache.end()) {
			// Potentially new file/directory
			potential_adds.push_back({ ChangeInfo::ADDED, path });
		} else if (it->second != currentInfo) {
			// Modified file/directory
			changes.push_back({ ChangeInfo::MODIFIED, path });
		}
	}

	// Check for deleted files
	for (const auto& path : file_cache | std::views::keys) {
		if (!currentSnapshot.contains(path)) {
			potential_deletes.push_back({ ChangeInfo::DELETED, path });
		}
	}

	// Detect renames by matching content between deletes and adds
	auto remaining_adds    = potential_adds;
	auto remaining_deletes = potential_deletes;

	for (auto del_it = remaining_deletes.begin(); del_it != remaining_deletes.end();) {
		bool found_rename = false;

		for (auto add_it = remaining_adds.begin(); add_it != remaining_adds.end(); ++add_it) {
			// Get the old file info for the deleted file
			auto old_file_it = file_cache.find(del_it->path);
			if (old_file_it != file_cache.end()) {
				auto new_file_it = currentSnapshot.find(add_it->path);
				if (new_file_it != currentSnapshot.end()) {
					// Get the new file info for the added file
					if (old_file_it->second.HasSameContent(new_file_it->second)) {
						changes.push_back({ ChangeInfo::RENAMED, add_it->path, del_it->path });
						remaining_adds.erase(add_it);
						del_it       = remaining_deletes.erase(del_it);
						found_rename = true;
						break;
					}
				}
			}
		}

		if (!found_rename) {
			++del_it;
		}
	}

	// Add remaining deletions and additions
	changes.insert(changes.end(), remaining_deletes.begin(), remaining_deletes.end());
	changes.insert(changes.end(), remaining_adds.begin(), remaining_adds.end());

	// Update cache with current state
	file_cache = std::move(currentSnapshot);
	return changes;
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

size_t DirectoryMonitor::GetFileCount() const {
	return file_cache.size();
}

void DirectoryMonitor::ResetCache() {
	file_cache = ScanDirectory();
	DEBUG_LOG("Monitor reset. Now tracking " << file_cache.size() << " items.");
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

	return current_files;
}
