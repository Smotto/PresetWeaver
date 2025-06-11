#ifndef DIRECTORYMONITOR_H_
#define DIRECTORYMONITOR_H_

#include <string>
#include <filesystem>

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

		Type type;
		std::string path;
		std::string old_path; // For renames

		[[nodiscard]] std::string TypeToString() const;
	};

private:

};

#endif /*! DIRECTORYMONITOR_H_ */
