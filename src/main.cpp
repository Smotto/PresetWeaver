#include "CusManager.h"
#include "DirectoryMonitor.h"
#include "OperatingSystemFunctions.h"

#include <app-window.h>
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	auto                                    ui                = AppWindow::create();

	const std::unique_ptr<CusManager>       cus_file_manager  = std::make_unique<CusManager>(ui);
	const std::unique_ptr<DirectoryMonitor> directory_monitor = std::make_unique<DirectoryMonitor>(cus_file_manager->GetCustomizingDirectory()); // TODO: Lives in cus file manager now, remove

	auto                                    initial_region    = slint::SharedString(OperatingSystemFunctions::GetLocalizationRegion());
	ui->global<GlobalVariables>().set_local_region(initial_region);
	ui->global<GlobalVariables>().set_selected_region(initial_region);

	cus_file_manager->SetSelectedRegionSafe(initial_region.data());
	cus_file_manager->RefreshUnconvertedFiles(initial_region.data());
;
	ui->global<GlobalVariables>().on_selected_region_changed([&cus_file_manager](const slint::SharedString& selected_region) {
		std::lock_guard<std::mutex> lock(cus_file_manager->conversion_mutex);

		cus_file_manager->SetSelectedRegionSafe(selected_region.data());

		if (cus_file_manager->GetAutomaticConversionEnabled()) {
			if (cus_file_manager->ConvertFilesToRegion(cus_file_manager->GetSelectedRegionSafe())) {
				cus_file_manager->RefreshUnconvertedFiles(cus_file_manager->GetSelectedRegionSafe());
			}
		}
	});

	ui->global<GlobalVariables>().on_request_refresh_files([&cus_file_manager, &ui]() -> void {
		const auto region = ui->global<GlobalVariables>().get_selected_region();
		cus_file_manager->RefreshUnconvertedFiles(region.data());
	});

	ui->global<GlobalVariables>().on_convert_files([&cus_file_manager, &ui]() -> void {
		const auto region = ui->global<GlobalVariables>().get_selected_region();
		if (!cus_file_manager->ConvertFilesToRegion(region.data())) {
			return;
		}

		cus_file_manager->RefreshUnconvertedFiles(region.data());

		ui->global<GlobalVariables>().set_convert_button_flashing(true);

		slint::Timer::single_shot(std::chrono::milliseconds(500), [&ui]() -> void {
			ui->global<GlobalVariables>().set_convert_button_flashing(false);
		});
	});

	ui->global<GlobalVariables>().on_toggle_automatic_conversion([&ui, &cus_file_manager]() -> void {
		bool enabled = !ui->global<GlobalVariables>().get_automatically_converting();
		ui->global<GlobalVariables>().set_automatically_converting(enabled);
		cus_file_manager->SetAutomaticConversionEnabled(enabled);
		const auto region = ui->global<GlobalVariables>().get_selected_region();
		if (cus_file_manager->ConvertFilesToRegion(region.data())) {
			cus_file_manager->RefreshUnconvertedFiles(region.data());
		}
	});

	ui->run();

	return 0;
}