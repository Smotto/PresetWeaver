#ifndef CUSMANAGER_H_
#define CUSMANAGER_H_

#include <app-window.h>
#include <filesystem>
#include <unordered_set>

class SlintCusFile;

struct CusFile {
	std::filesystem::path path_relative_to_customizing_directory;
	std::vector<char>     data;
	std::string           region;

	bool                  modified = false;
	bool                  invalid  = false;
};

class CusManager {
public:
	CusManager();
	~CusManager() = default;

	void                                              AddFile(const CusFile& file);
	void                                              UpdateFile(const CusFile& file, size_t index);
	std::shared_ptr<slint::VectorModel<SlintCusFile>> GetSlintModel();
	CusFile&                                          GetInternalFile(size_t index);

	std::vector<CusFile>&                             GetFiles();

	[[nodiscard]] size_t                              Count() const;
	static bool                                       ModifyRegion(CusFile& file, const std::string& region_name);

	bool                                              LoadFiles();
	void                                              SaveModified() const;

private:
	const std::unordered_set<std::string>             available_regions = std::unordered_set<std::string> { R"(USA)", R"(KOR)", R"(RUS)" };
	std::filesystem::path                             customizing_directory;
	std::vector<CusFile>                              internal_files;
	std::shared_ptr<slint::VectorModel<SlintCusFile>> slint_model;

	bool                                              LoadRegion(CusFile& file) const;
};

#endif /* CUSMANAGER_H_ */
