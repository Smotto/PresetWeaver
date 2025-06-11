#ifndef FILEINFO_H_
#define FILEINFO_H_

#include <filesystem>

namespace fs = std::filesystem;

struct FileInfo {
    std::chrono::time_point<std::chrono::file_clock> last_modified;
    uintmax_t                                        size;
    bool                                             is_directory;
    std::string                                      content_hash; // For detecting renames

    FileInfo();
    explicit FileInfo(const fs::path& filepath);
    bool operator!=(const FileInfo& other) const;
    bool HasSameContent(const FileInfo& other) const;

private:
    std::string CalculateHash(const fs::path& filepath) const;
};

#endif /*! FILEINFO_H_ */
