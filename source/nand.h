#ifndef _NAND_H_
#define _NAND_H_

/* 'NAND Device' structure */
typedef struct {
	/* Device name */
	char *name;

	/* Mode value */
	u32 mode;

	/* Un/mount command */
	u32 mountCmd;
	u32 umountCmd;
} nandDevice;

typedef struct
{
	char name[ISFS_MAXPATH];
	int type;
} NameList;

extern nandDevice ndevList[];
extern const int ndevCount;

/* Prototypes */
s32 Nand_Mount(nandDevice *);
s32 Nand_Unmount(nandDevice *);
s32 Nand_Enable(nandDevice *);
s32 Nand_Disable(void);
bool NANDInitialize();
u8* NANDReadFromFile(const char* path, u32 offset, u32 length, u32* size);
u8* NANDLoadFile(const char* path, u32* size);
s32 NANDWriteFileSafe(const char* path, u8* data, u32 size);
s32 NANDBackUpFile(const char* src, const char* dst, u32* size);
s32 NANDGetFileSize(const char* path, u32* size);
s32 NANDDeleteFile(const char* path);

#endif
