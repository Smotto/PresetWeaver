#ifndef CUSMANAGER_H_
#define CUSMANAGER_H_

#include <app-window.h>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

class DirectoryMonitor;
class SlintCusFile;

struct CusFile {
	std::filesystem::path path_relative_to_customizing_directory;
	std::vector<char>     data;
	std::string           region;
	bool                  invalid = false;
};

class CusManager {
public:
	explicit CusManager(slint::ComponentHandle<AppWindow> ui);
	~CusManager();
	void                                                                                        LoadFile(const std::filesystem::path& full_path);
	void                                                                                        RemoveFile(const std::filesystem::path& full_path);

	void                                                                                        RefreshUnconvertedFiles(const std::string& excluded_region) const;
	std::shared_ptr<slint::VectorModel<SlintCusFile>>                                           GetSlintModelFiles(const std::string& excluded_region);

	std::filesystem::path                                                                       GetCustomizingDirectory() const;

	[[nodiscard]] const std::unordered_map<std::string, std::vector<std::unique_ptr<CusFile>>>& GetFiles() const;
	[[nodiscard]] bool                                                                          ConvertFilesToRegion(const std::string& region_name);

	bool                                                                                        SaveFilesToDisk(const std::vector<CusFile*>& modified_files);
	void                                                                                        SetSelectedRegionSafe(const std::string& region);
	std::string                                                                                 GetSelectedRegionSafe();
	bool                                                                                        GetAutomaticConversionEnabled() const;

	void                                                                                        SetAutomaticConversionEnabled(bool enabled);

	std::mutex                                                                                  conversion_mutex;

private:
	slint::ComponentHandle<AppWindow>                                                  ui_handle;

	std::thread                                                                        monitor_thread;
	std::atomic<bool>                                                                  file_handling_active         = true;
	std::atomic<bool>                                                                  automatic_conversion_enabled = false;
	std::condition_variable                                                            monitor_condition_variable;
	std::atomic<bool>                                                                  active            = true;

	const std::unordered_set<std::string>                                              available_regions = { "USA", "KOR", "RUS" };
	std::string                                                                        selected_region;
	std::mutex                                                                         selected_region_mutex;
	std::mutex                                                                         file_mutex;
	std::mutex                                                                         monitor_mutex;

	std::mutex                                                                         recently_modified_mutex;
	std::unordered_set<std::filesystem::path>                                          recently_modified_paths;

	std::filesystem::path                                                              customizing_directory;
	std::unique_ptr<DirectoryMonitor>                                                  directory_monitor;

	std::unordered_map<std::string, std::shared_ptr<slint::VectorModel<SlintCusFile>>> slint_models_by_excluded_region;
	std::unordered_map<std::string, std::vector<std::unique_ptr<CusFile>>>             region_files_map;

	void                                                                               StartMonitorThread();
	void                                                                               StopMonitorThread();
	bool                                                                               LoadFilesFromDisk();
	bool                                                                               LoadRegion(CusFile& file) const;
};

#endif /* CUSMANAGER_H_ */