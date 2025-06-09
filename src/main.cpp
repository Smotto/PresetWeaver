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

	ui->global<GlobalVariables>().on_request_refresh_files([&cus_file_manager, &ui]() -> bool {
		const auto region = ui->global<GlobalVariables>().get_selected_region();
		cus_file_manager->RefreshUnconvertedFiles(region.data());
		return true;
	});

	ui->global<GlobalVariables>().on_convert_files([&cus_file_manager, &ui]() -> bool {
		const auto region = ui->global<GlobalVariables>().get_selected_region();
		if (!cus_file_manager->ConvertFilesToRegion(region.data())) {
			return false;
		}

		cus_file_manager->RefreshUnconvertedFiles(region.data());

		ui->global<GlobalVariables>().set_convert_button_flashing(true);

		slint::Timer::single_shot(std::chrono::milliseconds(500), [&ui]() {
			ui->global<GlobalVariables>().set_convert_button_flashing(false);
		});

		return true;
	});

	ui->run();

	return 0;
}