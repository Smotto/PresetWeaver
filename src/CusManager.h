#ifndef CUSMANAGER_H_
#define CUSMANAGER_H_

#include <app-window.h>
#include <filesystem>
#include <slint_models.h>
#include <unordered_set>

struct CusFile {
	std::filesystem::path path_relative_to_customizing_directory;
	std::vector<char>     data;
	std::string           region;

	bool                  modified = false;
	bool                  invalid  = false;
};

class CusManager {
public:
	explicit CusManager(const std::filesystem::path& customizing_directory);
	void                                              AddFile(const CusFile& file);
	void                                              UpdateFile(const CusFile& file, size_t index);
	std::shared_ptr<slint::VectorModel<SlintCusFile>> GetSlintModel();
	CusFile&                                          GetInternalFile(size_t index);
	~CusManager() = default;

	std::vector<CusFile>& GetFiles();

	size_t                Count() const;
	static bool           ModifyRegion(CusFile& file, const std::string& region_name);

	bool                  LoadFiles();
	void                  SaveModified() const;

private:
	std::vector<CusFile>                              internal_files;
	std::shared_ptr<slint::VectorModel<SlintCusFile>> slint_model;
	std::filesystem::path                             customizing_directory;
	const std::unordered_set<std::string>             available_regions = std::unordered_set<std::string> { R"(USA)", R"(KOR)", R"(RUS)" };

	bool                                              LoadRegion(CusFile& file) const;
};

#endif /* CUSMANAGER_H_ */
