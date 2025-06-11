#include "FileInfo.h"

#include <fstream>

FileInfo::FileInfo()
    : size(0), is_directory(false) {
}

FileInfo::FileInfo(const fs::path& filepath) {
    try {
        if (fs::exists(filepath)) {
            last_modified = fs::last_write_time(filepath);
            is_directory  = fs::is_directory(filepath);
            size          = is_directory ? 0 : fs::file_size(filepath);
            content_hash  = is_directory ? "" : CalculateHash(filepath);
        } else {
            size         = 0;
            is_directory = false;
            content_hash = "";
        }
    } catch (const fs::filesystem_error&) {
        size         = 0;
        is_directory = false;
        content_hash = "";
    }
}

bool FileInfo::HasSameContent(const FileInfo& other) const {
    return !content_hash.empty() && content_hash == other.content_hash &&
           size == other.size && is_directory == other.is_directory;
}

std::string FileInfo::CalculateHash(const fs::path& filepath) const {
    try {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return "";
        }

        // Simple hash based on file size and first/last bytes
        // For production, consider using a proper hash function like SHA-256
        std::hash<std::string> hasher;
        std::string content;

        // Read first 1KB and last 1KB for efficiency
        const size_t chunk_size = 1024;
        char buffer[chunk_size];

        file.read(buffer, chunk_size);
        content.append(buffer, file.gcount());

        // If file is larger than 2KB, also read the end
        if (size > 2 * chunk_size) {
            file.seekg(-static_cast<std::streamoff>(chunk_size), std::ios::end);
            file.read(buffer, chunk_size);
            content.append(buffer, file.gcount());
        }

        return std::to_string(hasher(content + std::to_string(size)));
    } catch (...) {
        return "";
    }
}

bool FileInfo::operator!=(const FileInfo& other) const {
    return last_modified != other.last_modified ||
           size != other.size ||
           is_directory != other.is_directory;
}
