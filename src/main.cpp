#include "CusManager.h"
#include "Debug.h"
#include "OperatingSystemFunctions.h"

#include <app-window.h>
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	_putenv("SLINT_BACKEND=winit-skia");
	auto                              ui               = AppWindow::create();
	const std::unique_ptr<CusManager> cus_file_manager = std::make_unique<CusManager>();

	if (!cus_file_manager->LoadFiles()) {
		DEBUG_LOG("Failed to load .cus files!");
		return EXIT_FAILURE;
	}

	ui->global<GlobalVariables>().set_local_region(slint::SharedString(OperatingSystemFunctions::GetLocalizationRegion()));
	ui->global<GlobalVariables>().set_selected_region(slint::SharedString(OperatingSystemFunctions::GetLocalizationRegion()));
	auto selected_region = ui->global<GlobalVariables>().get_selected_region();
	cus_file_manager->RefreshUnconvertedFiles(selected_region.data());
	ui->global<GlobalVariables>().set_unconverted_files(cus_file_manager->GetSlintModelUnconvertedFiles());

	ui->global<GlobalVariables>().on_request_refresh_files([&cus_file_manager, &ui]() {
		auto selected_region = ui->global<GlobalVariables>().get_selected_region();
		cus_file_manager->RefreshUnconvertedFiles(selected_region.data());
	});

	ui->run();

	return 0;
}