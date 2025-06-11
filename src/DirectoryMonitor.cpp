#include "DirectoryMonitor.h"

#include <string>

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