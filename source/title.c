#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <ogcsys.h>
#include <ogc/es.h>
#include <ogc/aes.h>

#include "title.h"
#include "nand.h"
#include "sha1.h"
#include "utils.h"
#include "malloc.h"

s32 Title_ZeroSignature(signed_blob *p_sig)
{
	u8 *ptr = (u8 *)p_sig;

	/* Fill signature with zeroes */
	memset(ptr + 4, 0, SIGNATURE_SIZE(p_sig) - 4);

	return 0;
}

s32 Title_FakesignTik(signed_blob *p_tik)
{
	tik *tik_data = NULL;
	u16 fill;

	/* Zero signature */
	Title_ZeroSignature(p_tik);

	/* Ticket data */
	tik_data = (tik *)SIGNATURE_PAYLOAD(p_tik);

	for (fill = 0; fill < USHRT_MAX; fill++) {
		sha1 hash;

		/* Modify ticket padding field */
		tik_data->padding = fill;

		/* Calculate hash */
		SHA1((u8 *)tik_data, sizeof(tik), hash);

		/* Found valid hash */
		if (!hash[0])
			return 0;
	}

	return -1;
}

s32 Title_FakesignTMD(signed_blob *p_tmd)
{
	tmd *tmd_data = NULL;
	u16 fill;

	/* Zero signature */
	Title_ZeroSignature(p_tmd);

	/* TMD data */
	tmd_data = (tmd *)SIGNATURE_PAYLOAD(p_tmd);

	for (fill = 0; fill < USHRT_MAX; fill++) {
		sha1 hash;

		/* Modify TMD fill field */
		tmd_data->fill3 = fill;

		/* Calculate hash */
		SHA1((u8 *)tmd_data, TMD_SIZE(tmd_data), hash);

		/* Found valid hash */
		if (!hash[0])
			return 0;
	}

	return -1;
}

s32 Title_GetList(u64 **outbuf, u32 *outlen)
{
	u64 *titles = NULL;

	u32 nb_titles;
	s32 ret;

	/* Get number of titles */
	ret = ES_GetNumTitles(&nb_titles);
	if (ret < 0)
		return ret;


	/* Allocate memory */
	titles = memalign32(nb_titles * sizeof(u64));
	if (!titles)
		return -1;

	/* Get titles */
	ret = ES_GetTitles(titles, nb_titles);
	if (ret < 0)
		goto err;

	/* Set values */
	*outbuf = titles;
	*outlen = nb_titles;

	return 0;

err:
	/* Free memory */
	free(titles);

	return ret;
}

s32 Title_GetTicketViews(u64 tid, tikview **outbuf, u32 *outlen)
{
	tikview *views = NULL;

	u32 nb_views;
	s32 ret;

	/* Get number of ticket views */
	ret = ES_GetNumTicketViews(tid, &nb_views);
	if (ret < 0)
		return ret;

	/* Allocate memory */
	views = memalign32(sizeof(tikview) * nb_views);
	if (!views)
		return -1;

	/* Get ticket views */
	ret = ES_GetTicketViews(tid, views, nb_views);
	if (ret < 0)
		goto err;

	/* Set values */
	*outbuf = views;
	*outlen = nb_views;

	return 0;

err:
	/* Free memory */
	free(views);

	return ret;
}

s32 Title_GetTMD(u64 tid, signed_blob **outbuf, u32 *outlen)
{
	void *p_tmd = NULL;

	u32 len;
	s32 ret;

	/* Get TMD size */
	ret = ES_GetStoredTMDSize(tid, &len);
	if (ret < 0)
		return ret;

	/* Allocate memory */
	p_tmd = memalign32(len);
	if (!p_tmd)
		return -1;

	/* Read TMD */
	ret = ES_GetStoredTMD(tid, p_tmd, len);
	if (ret < 0)
		goto err;

	/* Set values */
	*outbuf = p_tmd;
	*outlen = len;

	return 0;

err:
	/* Free memory */
	free(p_tmd);

	return ret;
}

s32 Title_GetTMDContents(signed_blob *s_tmd, u32 **contents, u32 *num_contents)
{
	s32 ret;

	ret = ES_GetNumStoredTMDContents(s_tmd, SIGNED_TMD_SIZE(s_tmd), num_contents);
	if (ret < 0)
		return ret;

	*contents = memalign32(*num_contents * sizeof(u32));
	if (!*contents)
		return -1;

	ret = ES_GetStoredTMDContents(s_tmd, SIGNED_TMD_SIZE(s_tmd), *contents, *num_contents);
	if (ret >= 0)
		return ret;

	free(*contents);
	*contents = NULL;
	*num_contents = 0;
	return ret;
}

s32 Title_GetTMDView(u64 tid, tmd_view** outbuf, u32* outlen)
{
	s32 ret;
	u32 view_sz = 0;

	ret = ES_GetTMDViewSize(tid, &view_sz);
	if (ret < 0)
		return ret;

	tmd_view* view = memalign32(view_sz);
	if (!view)
		return -1;

	ret = ES_GetTMDView(tid, (u8*) view, view_sz);
	if (ret < 0)
		goto fail;

	*outbuf = view;
	*outlen = view_sz;
	return 0;

fail:
	*outbuf = NULL;
	*outlen = 0;
	free(view);
	return ret;
}

s32 Title_GetVersion(u64 tid, u16 *outbuf)
{
	s32 ret;
	tmd_view *view = NULL;
	u32 len;

	/* Get title TMD */
	ret = Title_GetTMDView(tid, &view, &len);
	if (ret < 0)
		return ret;

	/* Set values */
	*outbuf = view->title_version;

	/* Free memory */
	free(view);

	return 0;
}

s32 Title_GetSysVersion(u64 tid, u64 *outbuf)
{
	s32 ret;
	tmd_view *view = NULL;
	u32 len;

	/* Get title TMD */
	ret = Title_GetTMDView(tid, &view, &len);
	if (ret < 0)
		return ret;

	/* Set values */
	*outbuf = view->sys_version;

	/* Free memory */
	free(view);

	return 0;
}

s32 Title_GetSize(u64 tid, u32 *outbuf)
{
	s32 ret;
	tmd_view *view = NULL;
	u32 len, size = 0;

	/* Get title TMD */
	ret = Title_GetTMDView(tid, &view, &len);
	if (ret < 0)
		return ret;

	/* Calculate title size */
	for (int cnt = 0; cnt < view->num_contents; cnt++) {
		tmd_view_content *content = &view->contents[cnt];

		/* Add content size */
		size += content->size;
	}

	/* Set values */
	*outbuf = size;

	/* Free memory */
	free(view);

	return 0;
}

s32 Title_GetIOSVersions(u8 *outbuf, u32 *outlen)
{
	u8  *buffer = NULL;
	u64 *list   = NULL;

	u32 count, cnt, idx;
	s32 ret;

	/* Get title list */
	ret = Title_GetList(&list, &count);
	if (ret < 0)
		return ret;

	/* Copy IOS */
	for (cnt = idx = 0; idx < count; idx++) {
		u32 tidh = (list[idx] >> 32);
		u32 tidl = (list[idx] &  0xFFFFFFFF);

		/* Title is IOS */
		if ((tidh == 0x1) && (tidl >= 3) && (tidl <= 255))
			buffer[cnt++] = (u8)(tidl & 0xFF);
	}

	/* Set values */
	*outlen = cnt;

	/* Free memory */
	free(list);

	return ret;
}

s32 Title_GetcIOSInfo(int ios, cIOSInfo* out)
{
	u64 titleID = 0x100000000ULL | ios;
	tmd_view* view = NULL;
	u32 view_size = 0;
	int i;

	char path[ISFS_MAXPATH];
	u32 size = 0;

	cIOSInfo* buf = NULL;

	s32 ret = Title_GetTMDView(titleID, &view, &view_size);
	if (ret < 0)
		return ret;

	u32 content0 = 0;
	for (i = 0; i < view->num_contents; i++)
	{
		if (view->contents[i].index == 0) {
			content0 = view->contents[i].cid;
			break;
		}
	}
	free(view);

	// insert ES_OpenTitleContent here
	sprintf(path, "/title/00000001/%08x/content/%08x.app", ios, content0);
	buf = (cIOSInfo*)NANDLoadFile(path, &size);

	if (buf && size == 0x40 && buf->hdr_magic == CIOS_INFO_MAGIC && buf->hdr_version == CIOS_INFO_VERSION) {
		*out = *buf;
		ret = 0;
	}
	else {
		ret = -1;
	}

	free(buf);
	return ret;
}

#if 0
void Title_GetFreeSpace(u32* free, s32* user_free)
{
	// Based off Dolphin Emulator's code. Cool stuff
	static const char* const userDirs[10] = {
		"/meta", "/ticket",
		"/title/00010000", "/title/00010001",
		"/title/00010003", "/title/00010004", "/title/00010005",
		"/title/00010006", "/title/00010007", "/shared2/title" /* Wtf */
	};

	u32 stats[8];
	ISFS_GetStats(stats);
	u32 cluster_size  = stats[0], // Can just hardcode 16384 here but eh
		free_clusters = stats[1],
		used_clusters = stats[2];

	*free = free_clusters * cluster_size;

	static const u32 user_blocks_max = 2176, // 1 block = 128KiB (131072 bytes)
	               user_clusters_max = user_blocks_max * 8; // 1 cluster = 16KiB (16384)

	u32 user_clusters_used = 0;
	for (int i = 0; i < 10; i++) {

		//                               Clusters   Inodes
		ret = ISFS_GetUsage(userDirs[i], &stats[0], &stats[1]);
		if (ret == 0)
			user_clusters_used += stats[0];
	}

	s32 user_clusters_free = user_clusters_max - user_clusters_used;
                *user_free = user_clusters_free * cluster_size;
}
#endif
u32 vWiiCommonKey[4] ATTRIBUTE_ALIGN(0x20);

void Title_SetupCommonKeys(void)
{
	static bool keys_ok = false;
	if (keys_ok)
		return;

	ATTRIBUTE_ALIGN(0x20)
	static const u32 vWiiCommonKey_enc[4] = { 0x6E18DB23, 0x847CBA6C, 0x1931A417, 0x9BAF8E09 };
	u32 iv[4] = {};

	s32 ret = ES_Decrypt(ES_KEY_COMMON, iv, vWiiCommonKey_enc, sizeof(vWiiCommonKey_enc), vWiiCommonKey);

	if (ret != 0) {
		printf("ES_Encrypt -> %i\n", ret);
		sleep(10);
	}

	keys_ok = (ret == 0);
	return;
};
