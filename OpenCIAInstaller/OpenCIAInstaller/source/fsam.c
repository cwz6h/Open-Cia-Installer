
/*
 * Author: hackNcode
 * License: MIT
 * Purpose: All FS and AM related shenanigance
 */

#include "libs.h"
#include "fsam.h"


int list_diritems(char *directory)
{

	DIR *items = opendir(directory);
    struct dirent *item;
    int count = 1;

	while((item = readdir(items))) {

		/* COUNT) FILENAME */
		printf("\x1b[%i;0H%i) %s", count + 8, count, item->d_name);
		++count;

	}

	closedir(items);
	return count - 1;

}

char *get_item_in_dir(char *directory, int number)
{

	DIR *items = opendir(directory);
    struct dirent *item;
    int count = 0;

	while((item = readdir(items))) {
		if(count == number)
			break;
		++count;
	}

	closedir(items);
	return item->d_name;

}

/*
 * path: the path to open a handle to
 * handle: A ref to a handle to write to
 */
int create_file_handle_read(char *path, Handle *handle)
{

	FS_ArchiveID id = ARCHIVE_SDMC;
	u32 flags = FS_OPEN_READ;
	u32 attribs = 0;

	Result res;

	FS_Path pathToArchive = fsMakePath(PATH_EMPTY, "");
	FS_Path pathToFile = fsMakePath(PATH_ASCII, path);

	res = FSUSER_OpenFileDirectly(handle, id, pathToArchive, pathToFile, flags, attribs);
	if(R_FAILED(res)) {
		print_error("Failed opening a handle for the file", res);
		pause_3ds();
		return -1;
	}

	return 0;

}

// https://github.com/Universal-Team/Universal-Updater/blob/master/source/utils/cia.cpp#L67
// Thanks for showing how2install cias
int install_cia(char *path, int line, bool ask)
{

	line = line + 2;

	FS_MediaType media = MEDIATYPE_SD;
	AM_TitleEntry title;

	Handle file;
	Result res;

	int fileHandleRes = create_file_handle_read(path, &file);
	if(fileHandleRes == -1)
		return -1;

	res = AM_GetCiaFileInfo(media, &title, file);

	if(R_FAILED(res)) {
		print_error("Failed reading cia metadata", res);
		pause_3ds();
		return -1;
	}

	// lu = u32, 016llx = 16 length hex (thanks Timm)
	printf("\x1b[%i;0HTitle ID: %016llx", line, title.titleID);
	printf("\x1b[%i;0HVersion: %i", line + 1, title.version);
	printf("\x1b[%i;0HSize: %.2fMiB", line + 2, title.size / pow(1024, 2));

	if(ask) {
		formatted_print("Press [B] to cancel or [A] to continue ...", 4, 26);
		while(aptMainLoop()) {
			hidScanInput();
			u32 kDown = hidKeysDown();
			if(kDown & KEY_B)
				return 0;
			else if(kDown & KEY_A)
				break;
		}
		formatted_print("                                           ", 4, 26);
	}

	// Real stuff starts here
	media = get_title_location(title.titleID);

	// For you Timm, since you hate commas
	u32 written;
	u32 read;

	u64 offset = 0;
	u64 size = 0;
	Handle cia;

	res = FSFILE_GetSize(file, &size);
	if(R_FAILED(res)) {
		print_error("Failed getting size of file", res);
		pause_3ds();
		return -1;
	}

	res = AM_StartCiaInstall(media, &cia);
	if(R_FAILED(res)) {
		print_error("Failed to start installing cia", res);
		pause_3ds();
		return -1;
	}

	// INSTALL_BUFFER_SIZE --> libs.h
	u8 *buffer = malloc(INSTALL_BUFFER_SIZE);
	// i dont think this should happen
	if(buffer == NULL) {
		// 0xD8C3FBF3 = 000-1011 = Out of memory
		print_error("Failed allocating memory", 0xD8C3FBF3);
		pause_3ds();
		return -1;
	}

	printf("\x1b[29;0HPercent: 0%% (0MiB/unk)");

	do {

		// So basically read file and then write to cia handle
		FSFILE_Read(file, &read, offset, buffer, INSTALL_BUFFER_SIZE);
		FSFILE_Write(cia, &written, offset, buffer, INSTALL_BUFFER_SIZE, FS_WRITE_FLUSH);
		offset += read;

		hidScanInput();
		u32 kDown = hidKeysDown();
		if(EXIT_KEYS) {
			AM_CancelCIAInstall(cia);
			printf("\x1b[29;0H                                        ");
			printf("\x1b[29;0HCancelled cia installation");
			pause_3ds();
			return 0;
		}

		// idk whai this worcc
		printf("\x1b[29;0HInstalling: %.2f%% (%llu/%llu)", ((float)offset / size * 100), offset / (u64)pow(1024, 2), size / (u64)pow(1024, 2));

	}
	while(offset < size);
	free(buffer);

	res = AM_FinishCiaInstall(cia);
	if(R_FAILED(res)) {
		print_error("Failed finishing the cia install", res);
		pause_3ds();
		return -1;
	}

	res = FSFILE_Close(file);
	if(R_FAILED(res)) {
		print_error("Failed closing file handle", res);
		pause_3ds();
		return -1;
	}

	formatted_print("Finished installation of cia", 0, 29);
	pause_3ds();

	return 0;

}

// https://github.com/Universal-Team/Universal-Updater/blob/master/source/utils/cia.cpp#L55
// Fucking bitwise
FS_MediaType get_title_location(u64 titleId)
{

	u16 platform = (u16) ((titleId >> 48) & 0xFFFF);
	u16 category = (u16) ((titleId >> 32) & 0xFFFF);
	u8 variation = (u8) (titleId & 0xFF);

	FS_MediaType type = platform == 0x0003
						|| (platform == 0x0004 && ((category & 0x8011) != 0
						|| (category == 0x0000 && variation == 0x02)))
						? MEDIATYPE_NAND
						: MEDIATYPE_SD;

	return type;

}
