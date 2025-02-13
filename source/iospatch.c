// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// Copyright 2010 Joseph Jordan <joe.ftpii@psychlaw.com.au>
// Wii U vWii patches Copyright 2012/2013 damysteryman

/* memmem says hi */
#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>
#include <ogc/ipc.h>

#include "iospatch.h"
#include "sys.h"

#define MEM_REG_BASE 0xd8b4000
#define MEM_PROT (MEM_REG_BASE + 0x20a)

// I don't like hardcoding binary blobs like this either. We'll get serious later okay
static const u32 armCode[] = {
    0xE3A00536, /* mov     r0, #0x0D800000 */
    0xE5901060, /* ldr     r1, [r0, #0x60] */
    0xE3811008, /* orr     r1, #0x08 */
    0xE5801060, /* str     r1, [r0, #0x60] */
    0xE5901064, /* ldr     r1, [r0, #0x64] */
    0xE381113A, /* orr     r1, #0x8000000E */
    0xE3811EDF, /* orr     r1, #0x00000DF0 */
    0xE5801064, /* str     r1, [r0, #0x64] */
    0xE12FFF1E, /* bx      lr */
};

/*
 * https://github.com/WiiLink24/wfc-patcher-wii/blob/main/launcher/source/IOS.cpp#L168-L171
 */
int do_sha_exploit(u32 addr) {
	int    ret;
	int    fd = 0x10001;
	ioctlv vecs[3] ATTRIBUTE_ALIGN(0x20) = {};

	vecs[0].data = NULL;
	vecs[0].len  = 0;
	vecs[1].data = (void *)0xFFFE0028;
	vecs[1].len  = 0;

	u32 *mem1 = (u32 *)0x80000000;
	*mem1++ = 0x4903468D;
	*mem1++ = 0x49034788;
	*mem1++ = 0x49036209;
	*mem1++ = 0x47080000;
	*mem1++ = 0x10100000;
	*mem1++ = MEM_VIRTUAL_TO_PHYSICAL(addr);
	*mem1++ = 0xFFFF0014;

	vecs[2].data = mem1;
	vecs[2].len  = 0x20;

	ret = IOS_Ioctlv(fd, 0, 1, 2, vecs);

	return ret;
}

static u32 apply_patch(const void *old, u32 old_size, const void *patch, u32 patch_size, u32 patch_offset) {
	void *ptr_start = (void *)0x933E0000;
	void *ptr_end   = (void *)0x94000000;
	u32   found;

	for (found = 0; ptr_start < ptr_end; found++) {
		ptr_start = memmem(ptr_start, ptr_end - ptr_start, old, old_size);
		if (!ptr_start)
			break;

		memcpy(ptr_start + patch_offset, patch, patch_size);
		DCFlushRange(ptr_start + patch_offset, patch_size);
		ptr_start += patch_offset + patch_size;
	}

	return found;
}

const u8 ES_TitleVersionCheck_old[]   = { 0xD2, 0x01, 0x4E, 0x56 };
const u8 ES_TitleVersionCheck_patch[] = { 0xE0, 0x01, 0x4E, 0x56 };

const u8 ES_TitleDeleteCheck_old[]   = { 0xD8, 0x00, 0x4A, 0x04 };
const u8 ES_TitleDeleteCheck_patch[] = { 0xE0, 0x00, 0x4A, 0x04 };

const u16 ES_KeyslotPermissionCheck_old[]   = { 0x2d06, 0xd000, 0x4803 };
const u16 ES_KeyslotPermissionCheck_patch[] = { 0x2d06, 0xe000, 0x4803 };

const u8 isfs_permissions_old[]   = { 0x9B, 0x05, 0x40, 0x03, 0x99, 0x05, 0x42, 0x8B, };
const u8 isfs_permissions_patch[] = { 0x9B, 0x05, 0x40, 0x03, 0x1C, 0x0B, 0x42, 0x8B, };

//Following patches made my damysteryman for use with Wii U's vWii
const u8 Kill_AntiSysTitleInstallv3_pt1_old[] = { 0x68, 0x1A, 0x2A, 0x01, 0xD0, 0x05 };	// Make sure that the pt1
const u8 Kill_AntiSysTitleInstallv3_pt1_patch[] = { 0x68, 0x1A, 0x2A, 0x01, 0x46, 0xC0 };	// patch is applied twice. -dmm
const u8 Kill_AntiSysTitleInstallv3_pt2_old[] = { 0xD0, 0x02, 0x33, 0x06, 0x42, 0x9A, 0xD1, 0x01 };	// Make sure that the pt2 patch
const u8 Kill_AntiSysTitleInstallv3_pt2_patch[] = { 0x46, 0xC0, 0x33, 0x06, 0x42, 0x9A, 0xE0, 0x01 };	// is also applied twice. -dmm
const u8 Kill_AntiSysTitleInstallv3_pt3_old[] = { 0x68, 0xFB, 0x2B, 0x00, 0xDB, 0x01 };
const u8 Kill_AntiSysTitleInstallv3_pt3_patch[] = { 0x68, 0xFB, 0x2B, 0x00, 0xDB, 0x10 };

static inline bool validJumptablePtr(uint32_t x) {
	return (x >= 0xFFFF0040) && (x < 0xFFFFF000) && ((x & 3) == 0);
}

#define SRAMNOMIRR(x) (x - 0xF2B00000)
static u32 findTheSyscallTable(void) {
	u32 undfInstruction = read32(0xD4F0004);
	if ((undfInstruction & 0xFFFFF000) != 0xE59FF000 /* ldr pc, [pc, something] */)
		// SRNPROT is probably on
		return 0;

	u32 undfHandler = read32(0xD4F0004 + 8 /* pc is 2 steps ahead */ + (undfInstruction & 0xFFF));
	if (!validJumptablePtr(undfHandler))
		// Eh?
		return 0;

	undfHandler = SRAMNOMIRR(undfHandler);

	// arbitrary number. don't plan to go far
	for (int i = 0; i < 0x80; i += 4) {
		undfInstruction = read32(undfHandler + i);
		if ((undfInstruction & 0xFFFF0000) == 0xE59F0000) { // find the first thing loaded relative to PC
			u32 addr = undfHandler + i + 8 + (undfInstruction & 0xFFF);
			//          syscall instr mask   stack args cnt map  the actual syscall table (they are right next to each other)
			if (read32(addr) == 0xE6000010 && (read32(addr + 4) - read32(addr + 8)) < 0x400) { // this 0x400 (1024) is from the & 0xFF applied to the syscall # after it gets unmasked
				return SRAMNOMIRR(read32(addr + 8)); // the actual syscall table wooooooooo yeahh babyy
			}
		}
	}

	return 0;
}

u32 *ptr_syscallTable = NULL;
u32  ptr_IOSC_VerifyPublicKeySign = 0;
static const u16 return0[] = {
	0x2000, // mov r0, #0
	0x4770, // bx  lr
};

int IOSPATCH_Apply(void) {
	int fd = IOS_Open("/dev/dolphin", 0);
	IOS_Close(fd);
	if (fd >= 0)
		return 0;

	int ret = fd = IOS_Open("/dev/sha", 0);
	IOS_Close(fd);
	if (ret < 0)
		return (ret == IPC_ENOENT) ? 0 : ret;


	ret = do_sha_exploit((u32)armCode);

	if (ret < 0)
		return ret;

	int clock = 1000;
	while (!AHBPROT_DISABLED) {
		if (clock-- == 0)
			return -0x123;

		usleep(1000);
	}

	ptr_syscallTable = (u32 *)findTheSyscallTable();
	if (!ptr_syscallTable)
		return -0x105C;

	ptr_IOSC_VerifyPublicKeySign = read32((u32)&ptr_syscallTable[0x6C]);

	write16(MEM_PROT, 0);

	IOSPATCH_SetSignatureChecks(false);
	// apply_patch(hash_old, sizeof(hash_old), hash_patch, sizeof(hash_patch), 1);
	// apply_patch(new_hash_old, sizeof(new_hash_old), hash_patch, sizeof(hash_patch), 1);
	apply_patch(isfs_permissions_old, sizeof(isfs_permissions_old), isfs_permissions_patch, sizeof(isfs_permissions_patch), 0);
	apply_patch(ES_TitleVersionCheck_old, sizeof(ES_TitleVersionCheck_old), ES_TitleVersionCheck_patch, sizeof(ES_TitleVersionCheck_patch), 0);
	apply_patch(ES_TitleDeleteCheck_old, sizeof(ES_TitleDeleteCheck_old), ES_TitleDeleteCheck_patch, sizeof(ES_TitleDeleteCheck_patch), 0);
	apply_patch(ES_KeyslotPermissionCheck_old, sizeof(ES_KeyslotPermissionCheck_old), ES_KeyslotPermissionCheck_patch, sizeof(ES_KeyslotPermissionCheck_patch), 0);

	if (IS_WIIU)
	{
		apply_patch(Kill_AntiSysTitleInstallv3_pt1_old, sizeof(Kill_AntiSysTitleInstallv3_pt1_old), Kill_AntiSysTitleInstallv3_pt1_patch, sizeof(Kill_AntiSysTitleInstallv3_pt1_patch), 0);
		apply_patch(Kill_AntiSysTitleInstallv3_pt2_old, sizeof(Kill_AntiSysTitleInstallv3_pt2_old), Kill_AntiSysTitleInstallv3_pt2_patch, sizeof(Kill_AntiSysTitleInstallv3_pt2_patch), 0);
		apply_patch(Kill_AntiSysTitleInstallv3_pt3_old, sizeof(Kill_AntiSysTitleInstallv3_pt3_old), Kill_AntiSysTitleInstallv3_pt3_patch, sizeof(Kill_AntiSysTitleInstallv3_pt3_patch), 0);
	}

	return 0;
}

void IOSPATCH_SetSignatureChecks(bool enable)
{
	if (!ptr_syscallTable)
		return;

	u32 new = enable ? ptr_IOSC_VerifyPublicKeySign : (MEM_VIRTUAL_TO_PHYSICAL(return0) | 0x1);
	write32((u32)&ptr_syscallTable[0x6C], new);
}
