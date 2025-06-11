#ifndef DIRECTORYMONITOR_H_
#define DIRECTORYMONITOR_H_

#include "FileInfo.h"

#include <condition_variable>
#include <filesystem>
#include <string>
#include <thread>
#include <unordered_map>

namespace fs = std::filesystem;

class DirectoryMonitor {
public:
	struct ChangeInfo {
		enum Type {
			ADDED,
			MODIFIED,
			DELETED,
			RENAMED
		};

		Type                      type;
		std::filesystem::path     path;
		std::filesystem::path     old_path; // For renames

		[[nodiscard]] std::string TypeToString() const;
	};

	explicit DirectoryMonitor(const std::filesystem::path& path, bool recurse_subdirectories = true);
	~DirectoryMonitor(); // Properly clean up threads
	DirectoryMonitor(const DirectoryMonitor& other)                  = delete;
	DirectoryMonitor&       operator=(const DirectoryMonitor& other) = delete;

	void                    ResumeMonitoring();
	void                    StopMonitoring();
	bool                    IsMonitoringActive() const;

	std::vector<ChangeInfo> CheckForDirectoryChanges();
	static void             PrintChanges(const std::vector<ChangeInfo>& changes);
	size_t                  GetFileCount() const;
	void                    ResetCache();

private:
	std::filesystem::path                               root_path;
	bool                                                recurse_subdirectories;
	std::unordered_map<std::filesystem::path, FileInfo> file_cache;
	std::chrono::milliseconds                           interval { std::chrono::milliseconds(1000) };
	std::thread                                         monitor_thread;
	std::atomic<bool>                                   monitoring_active { false };
	std::atomic<bool>                                   monitor_thread_should_run { true };
	std::condition_variable                             monitor_thread_condition_variable;
	std::mutex                                          monitor_thread_mutex;
	std::atomic<bool>                                   thread_finished { true };

	std::unordered_map<std::filesystem::path, FileInfo> ScanDirectory() const;
	void                                                StartMonitorThread();
	void                                                StopMonitorThread();
	void                                                BackgroundTask();
};

#endif /*! DIRECTORYMONITOR_H_ */
