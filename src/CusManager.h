#ifndef CUSMANAGER_H_
#define CUSMANAGER_H_

#include <app-window.h>
#include <filesystem>
#include <unordered_set>

// TODO: Cache files by region
// unordered_map<region, vector>
// internal files are 1 single source of truth, the UI has to all of it minus the huge amount of data each file carries
//

// UI. I should be able to see un_converted files based on selected region and current file[index] region
// UI. I should be able to select a region and have a change in the selected files.

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
	void RefreshUnconvertedFiles(const std::string& selected_region) const;
	~CusManager() = default;

	void                                              AddFileToUI(const CusFile& file) const;
	std::shared_ptr<slint::VectorModel<SlintCusFile>> GetSlintModelUnconvertedFiles();

	std::vector<CusFile>&                             GetFiles();

	[[nodiscard]] size_t                              Count() const;
	static bool                                       ModifyRegion(CusFile& file, const std::string& region_name);

	bool                                              LoadFiles();
	void                                              SaveModified() const;

private:
	const std::unordered_set<std::string>             available_regions = std::unordered_set<std::string> { R"(USA)", R"(KOR)", R"(RUS)" };
	std::shared_ptr<AppWindow>                        ui_instance;
	std::filesystem::path                             customizing_directory;
	std::vector<CusFile>                              internal_files;
	std::shared_ptr<slint::VectorModel<SlintCusFile>> slint_model_unconverted_files;

	bool                                              LoadRegion(CusFile& file) const;
};

#endif /* CUSMANAGER_H_ */
