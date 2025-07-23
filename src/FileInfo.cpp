#include "FileInfo.h"

#include "xxhash.h"

#include <fstream>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

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
			file_id       = is_directory ? 0 : GetFileID(filepath);
		} else {
			size         = 0;
			is_directory = false;
			content_hash = "";
			file_id      = 0;
		}
	} catch (const fs::filesystem_error&) {
		size         = 0;
		is_directory = false;
		content_hash = "";
		file_id      = 0;
	}
}

bool FileInfo::operator!=(const FileInfo& other) const {
	return last_modified != other.last_modified ||
	       is_directory != other.is_directory ||
	       size != other.size ||
	       content_hash != other.content_hash;
}

bool FileInfo::HasSameContent(const FileInfo& other) const {
	return !content_hash.empty() &&
	       content_hash == other.content_hash &&
	       size == other.size &&
	       is_directory == other.is_directory;
}

std::string FileInfo::CalculateHash(const fs::path& filepath) {
	try {
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
			return "";

		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), {});
		uint64_t          hash = XXH3_64bits(buffer.data(), buffer.size());
		return std::to_string(hash);
	} catch (...) {
		return "";
	}
}

uint64_t FileInfo::GetFileID(const fs::path& filepath) {
#ifdef _WIN32
	HANDLE hFile = CreateFileW(
	    filepath.wstring().c_str(),
	    0,
	    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	    NULL,
	    OPEN_EXISTING,
	    FILE_FLAG_BACKUP_SEMANTICS, // needed for directories
	    NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}

	BY_HANDLE_FILE_INFORMATION info;
	if (!GetFileInformationByHandle(hFile, &info)) {
		CloseHandle(hFile);
		return 0;
	}

	CloseHandle(hFile);
	return (static_cast<uint64_t>(info.nFileIndexHigh) << 32) | info.nFileIndexLow;

#else
	struct stat stat_buf;
	if (stat(filepath.c_str(), &stat_buf) != 0) {
		return 0;
	}

	return (static_cast<uint64_t>(stat_buf.st_dev) << 32) | stat_buf.st_ino;
#endif
}

// Platform-specific file ID
uint64_t GetFileID(const fs::path& filepath) {
#ifdef _WIN32
	HANDLE hFile = CreateFileW(
	    filepath.wstring().c_str(),
	    0,
	    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	    NULL,
	    OPEN_EXISTING,
	    FILE_FLAG_BACKUP_SEMANTICS,
	    NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}

	BY_HANDLE_FILE_INFORMATION info;
	if (!GetFileInformationByHandle(hFile, &info)) {
		CloseHandle(hFile);
		return 0;
	}

	CloseHandle(hFile);
	return (static_cast<uint64_t>(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
#else
	struct stat stat_buf;
	if (stat(filepath.c_str(), &stat_buf) != 0) {
		return 0;
	}

	return (static_cast<uint64_t>(stat_buf.st_dev) << 32) | stat_buf.st_ino;
#endif
}
