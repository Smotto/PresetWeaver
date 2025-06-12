#ifndef DIRECTORYMONITOR_H_
#define DIRECTORYMONITOR_H_

#include "FileInfo.h"

#include <filesystem>
#include <string>
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
	~DirectoryMonitor();
	DirectoryMonitor(const DirectoryMonitor& other)                  = delete;
	DirectoryMonitor&       operator=(const DirectoryMonitor& other) = delete;


	std::vector<ChangeInfo> CheckForDirectoryChanges();
	static void             PrintChanges(const std::vector<ChangeInfo>& changes);
	size_t                  GetFileCount() const;
	void                    ResetCache();

private:
	std::filesystem::path                               root_path;
	bool                                                recurse_subdirectories;
	std::unordered_map<std::filesystem::path, FileInfo> file_cache;

	std::unordered_map<std::filesystem::path, FileInfo> ScanDirectory() const;
};

#endif /*! DIRECTORYMONITOR_H_ */
