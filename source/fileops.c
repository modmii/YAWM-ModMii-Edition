#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "fileops.h"
#include "utils.h"

bool FSOPFileExists(const char* file)
{
	struct stat st;
	return !stat(file, &st) && !S_ISDIR(st.st_mode);
}

bool FSOPFolderExists(const char* path)
{
	struct stat st;
	return !stat(path, &st) && S_ISDIR(st.st_mode);
}

size_t FSOPGetFileSizeBytes(const char* path)
{
	struct stat st;
	if (stat(path, &st) < 0) return 0;

	return st.st_size;
}

void FSOPDeleteFile(const char* file)
{
	if (FSOPFileExists(file))
		remove(file);
}

void FSOPMakeFolder(const char* path)
{
	if (FSOPFolderExists(path))
		return;

	char* pos = strchr(path, '/');
	s32 current = pos - path;
	current++;
	pos = strchr(path + current, '/');

	while (pos)
	{
		*pos = 0;
		mkdir(path, S_IREAD | S_IWRITE);
		*pos = '/';
		
		current = pos - path;
		current++;
		pos = strchr(path + current, '/');
	}

	mkdir(path, S_IREAD | S_IWRITE);
}

s32 FSOPReadOpenFile(FILE* fp, void* buffer, u32 offset, u32 length)
{
	fseek(fp, offset, SEEK_SET);
	if (!fread(buffer, length, 1, fp))
		return -errno ?: -1;

	return 0;
}

s32 FSOPReadOpenFileA(FILE* fp, void** buffer, u32 offset, u32 length)
{
	*buffer = memalign32(length);
	if (!*buffer)
		return -ENOMEM;

	s32 ret = FSOPReadOpenFile(fp, *buffer, offset, length);
	if (ret < 0)
	{
		free(*buffer);
		*buffer = NULL;
	}

	return ret;
}
