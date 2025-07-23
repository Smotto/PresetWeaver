#ifndef FILEINFO_H_
#define FILEINFO_H_

#include <filesystem>

namespace fs = std::filesystem;

struct FileInfo {
	std::chrono::time_point<std::chrono::file_clock> last_modified;
	uintmax_t                                        size;
	bool                                             is_directory;
	std::string                                      content_hash;
	uint64_t                                         file_id = 0;

	FileInfo();
	explicit FileInfo(const fs::path& filepath);
	bool               operator!=(const FileInfo& other) const;

	uint64_t           GetFileID(const fs::path& filepath);
	[[nodiscard]] bool HasSameContent(const FileInfo& other) const;

private:
	static std::string CalculateHash(const fs::path& filepath);
};

#endif /*! FILEINFO_H_ */
