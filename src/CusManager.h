#ifndef CUSMANAGER_H_
#define CUSMANAGER_H_

#include <app-window.h>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

class SlintCusFile;

struct CusFile {
	std::filesystem::path path_relative_to_customizing_directory;
	std::vector<char>     data;
	std::string           region;
	bool                  invalid = false;
};

class CusManager {
public:
	CusManager();
	~CusManager() = default;

	void                                                                       RefreshUnconvertedFiles(const std::string& selected_region) const;
	std::shared_ptr<slint::VectorModel<SlintCusFile>>                          GetSlintModelUnconvertedFiles();

	[[nodiscard]] const std::unordered_map<std::string, std::vector<CusFile>>& GetFiles() const;
	[[nodiscard]] bool                                                         ConvertFilesToRegion(const std::string& region_name);

	bool                                                                       SaveFilesToDisk(std::vector<CusFile*> modified_files) const;

private:
	const std::unordered_set<std::string>                 available_regions = { "USA", "KOR", "RUS" };
	std::filesystem::path                                 customizing_directory;

	std::shared_ptr<AppWindow>                            ui_instance;
	std::shared_ptr<slint::VectorModel<SlintCusFile>>     slint_model_unconverted_files;
	std::unordered_map<std::string, std::vector<CusFile>> internal_files; // TODO: This will be edited by a thread in the background that listens to operating system file data-modifications, adds, removes, renames.

	bool                                                  LoadFilesFromDisk();
	bool                                                  LoadRegion(CusFile& file) const;
};

#endif /* CUSMANAGER_H_ */