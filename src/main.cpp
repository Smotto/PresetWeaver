#include "CusManager.h"
#include "Debug.h"
#include "OperatingSystemFunctions.h"

#include <app-window.h>
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	_putenv("SLINT_BACKEND=winit-skia");
	auto                              ui               = AppWindow::create();

	const std::unique_ptr<CusManager> cus_file_manager = std::make_unique<CusManager>();

	auto                              initial_region   = slint::SharedString(OperatingSystemFunctions::GetLocalizationRegion());
	ui->global<GlobalVariables>().set_local_region(initial_region);
	ui->global<GlobalVariables>().set_selected_region(initial_region);

	cus_file_manager->RefreshUnconvertedFiles(initial_region.data());
	ui->global<GlobalVariables>().set_unconverted_files(cus_file_manager->GetSlintModelUnconvertedFiles());

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

	ui->global<GlobalVariables>().on_toggle_automatic_conversion([&ui]() -> void {
		// - Create the thread based on a request, only 1 thread can be created, a singleton thread in this case.
		ui->global<GlobalVariables>().set_automatically_converting(!ui->global<GlobalVariables>().get_automatically_converting());

	});


	ui->run();

	// - If any thread exists, destroy it.

	return 0;
}