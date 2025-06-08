#include "CusManager.h"
#include "Debug.h"
#include "OperatingSystemFunctions.h"

#include <app-window.h>
#include <windows.h>


void enable_console_debugging(LPSTR lpCmdLine) {
	// Allocate a console for this GUI application
	AllocConsole();

	// Redirect stdout, stdin, stderr to console
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
	freopen_s((FILE**)stdin, "CONIN$", "r", stdin);

	// Make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
	// point to console as well
	std::ios::sync_with_stdio(true);

	// Now you can use printf, cout, etc.
	std::cout << "Debug console is now available!" << std::endl;
	printf("Command line: %s\n", lpCmdLine);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	_putenv("SLINT_BACKEND=winit-skia");

	const std::unique_ptr<CusManager> cus_file_manager = std::make_unique<CusManager>();

	if (!cus_file_manager->LoadFiles()) {
		DEBUG_LOG("Failed to load .cus files!");
		return EXIT_FAILURE;
	}

	std::vector<CusFile> files = cus_file_manager->GetFiles();
	auto ui = AppWindow::create();

	ui->global<GlobalVariables>().set_files(cus_file_manager->GetSlintModel());
	ui->global<GlobalVariables>().set_local_region(slint::SharedString(OperatingSystemFunctions::GetLocalizationRegion()));
	ui->run();

	return 0;
}