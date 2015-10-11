/*
SUSIE32 '00IN' Plug-in for BPG
*/

#ifndef spiFLIF_ex_h
#define spiFLIF_ex_h

#include "spi00in.h"

/*-------------------------------------------------------------------------*/
/* この Plugin の情報 */
/*-------------------------------------------------------------------------*/
static const char *pluginfo[] = {
	"00IN",				/* Plug-in API バージョン */
	"FLIF to DIB ver 0",		/* Plug-in名,バージョン及び copyright */
	"*.flif",			/* 代表的な拡張子 ("*.JPG" "*.JPG;*.JPEG" など) */
	"FLIF file(*.FLIF)",	/* ファイル形式名 */
};

#pragma pack(push)
#pragma pack(1) //構造体のメンバ境界を1バイトにする
typedef struct BMP_FILE {
	BITMAPFILEHEADER	header;
	BITMAPINFOHEADER	info;
	RGBQUAD				pal[1];
} BMP_FILE;
#pragma pack(pop)

//ヘッダチェック等に必要なサイズ.2KB以内で.
#define HEADBUF_SIZE 12

BOOL IsSupportedEx(char *filename, char *data);
int GetPictureInfoEx(char *data, struct PictureInfo *lpInfo);
int GetPictureEx(long datasize, HANDLE *pHBInfo, HANDLE *pHBm,
			 SPI_PROGRESS lpPrgressCallback, long lData, char *data);

#endif
