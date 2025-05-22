#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char *argv[])
#endif
{
	if (chdir("bin") != 0) {
#ifdef _WIN32
		MessageBoxA(NULL, "Failed to change directory to 'bin'", "Launcher Error", MB_OK | MB_ICONERROR);
#else
		perror("Failed to change directory to 'bin'");
#endif
		return 1;
	}

#ifdef _WIN32
	STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL; // or SW_HIDE to hide window

    // Command to run
    LPCSTR cmd = "KMAT.exe";

    if (!CreateProcessA(NULL, (LPSTR)cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        MessageBoxA(NULL, "Failed to launch KMAT.exe", "Launcher Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Optionally wait for the process to exit (remove if you want async launch)
    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
	int result = system("./KMAT");
	if (result != 0) {
		fprintf(stderr, "Failed to launch KMAT (error code %d).\n", result);
		return 1;
	}
#endif

	return 0;
}
