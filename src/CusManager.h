#ifndef CUSMANAGER_H_
#define CUSMANAGER_H_

#include <filesystem>
#include <map>
#include <unordered_set>

struct CusFile {
	std::filesystem::path path;
	std::vector<char>     data;
	std::string           region;

	bool                  modified = false;
	bool                  invalid  = false;
};

class CusManager {
public:
	explicit CusManager(const std::filesystem::path& directory);
	~CusManager() = default;

	std::map<std::string, CusFile>& GetFiles();
	std::vector<char>&              GetFile(const std::string& relative_path);
	std::vector<std::string>        GetFileList();

	size_t                          Count() const;
	static bool                     ModifyRegion(CusFile& file, const std::string& region_name);

	bool                            LoadFiles();
	void                            SaveModified();

private:
	std::map<std::string, CusFile>  files;
	std::filesystem::path           customizing_directory;
	std::unordered_set<std::string> available_regions = std::unordered_set<std::string> { R"(USA)", R"(KOR)", R"(RUS)" };

	bool                            LoadRegion(CusFile& file) const;
};

#endif /* CUSMANAGER_H_ */
