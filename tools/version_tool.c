#include "version_tool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#endif


#define BUFFER_SIZE 8192
#define REPO_URL "https://api.github.com/repos/Zange10/KMAT/releases/latest"

typedef struct {
	int major, minor, patch;
} KmatVersion;

KmatVersion current_version = {.major = 1, .minor = 3, .patch = 1};
KmatVersion latest_release = {1,0,0};

void get_current_version_string_incl_tool_name(char *string) {
	sprintf(string, "KMAT v%d.%d.%d", current_version.major, current_version.minor, current_version.patch);
}

void get_current_version_string(char *string) {
	sprintf(string, "v%d.%d.%d", current_version.major, current_version.minor, current_version.patch);
}

void get_latest_release_version_string(char *string) {
	sprintf(string, "v%d.%d.%d", latest_release.major, latest_release.minor, latest_release.patch);
}


char *extract_tag_name(const char *json) {
	const char *key = "\"tag_name\":";
	const char *start = strstr(json, key);
	if (!start) return NULL;

	// Find the first quote AFTER the colon
	start = strchr(start + strlen(key), '\"');
	if (!start) return NULL;
	start++; // skip the opening quote

	const char *end = strchr(start, '\"');
	if (!end) return NULL;

	size_t len = end - start;
	char *tag = malloc(len + 1);
	if (!tag) return NULL;

	strncpy(tag, start, len);
	tag[len] = '\0';
	return tag;
}

void get_latest_release_version_from_github() {
	#ifdef _WIN32
		HINTERNET hInternet = InternetOpen("version-checker", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (!hInternet) {
			fprintf(stderr, "InternetOpen failed.\n");
			return;
		}

		HINTERNET hFile = InternetOpenUrl(hInternet, REPO_URL, NULL, 0, INTERNET_FLAG_RELOAD, 0);
		if (!hFile) {
			fprintf(stderr, "InternetOpenUrl failed.\n");
			InternetCloseHandle(hInternet);
			return;
		}

		char buffer[BUFFER_SIZE];
		DWORD bytesRead;
		size_t total_read = 0;

		while (InternetReadFile(hFile, buffer + total_read, BUFFER_SIZE - total_read - 1, &bytesRead) && bytesRead != 0) {
			total_read += bytesRead;
			if (total_read >= BUFFER_SIZE - 1) break;
		}
		buffer[total_read] = '\0';

		InternetCloseHandle(hFile);
		InternetCloseHandle(hInternet);

		char *tag = extract_tag_name(buffer);
		if (tag) {
			sscanf(tag, "v%d.%d.%d", &latest_release.major, &latest_release.minor, &latest_release.patch);
			free(tag);
		} else {
			fprintf(stderr, "Failed to parse version from server response.\n");
		}

	#else
		char command[512];
		snprintf(command, sizeof(command), "wget -qO- --header='User-Agent: version-checker' \"%s\"", REPO_URL);

		FILE *fp = popen(command, "r");
		if (!fp) {
			perror("popen");
			return;
		}

		char buffer[BUFFER_SIZE];
		size_t total_read = fread(buffer, 1, BUFFER_SIZE - 1, fp);
		buffer[total_read] = '\0';
		pclose(fp);

		char *tag = extract_tag_name(buffer);
		if (tag) {
			sscanf(tag, "v%d.%d.%d", &latest_release.major, &latest_release.minor, &latest_release.patch);
			free(tag);
		} else {
			fprintf(stderr, "Failed to parse version from github.\n");
		}
	#endif
}

int is_latest_version() {
	get_latest_release_version_from_github();
	char current_version_string[16];
	char latest_release_version_string[16];
	get_current_version_string(current_version_string);
	get_latest_release_version_string(latest_release_version_string);
	printf("Current Version: %s\nVersion of latest release: %s\n", current_version_string, latest_release_version_string);
	if(current_version.major < latest_release.major ||
			(current_version.major == latest_release.major && current_version.minor < latest_release.minor) ||
			(current_version.major == latest_release.major && current_version.minor == latest_release.minor && current_version.patch < latest_release.patch))
		return 0;
	return 1;
}