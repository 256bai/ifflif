/*
SUSIE32 '00IN' Plug-in for BPG
*/
#include <io.h>
#include <Fcntl.h>

#define NOMINMAX
#include "flif/image/image.h"
#include "flif/flif-dec.h"
#include "flif/fileio.h"
#include "flif/common.h"
#include <inttypes.h>
#include "spiFLIF_ex.h"

/* エントリポイント */
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}


/***************************************************************************
 * 画像形式依存の処理.
 * ～Ex関数を画像形式に合わせて書き換える.
 ***************************************************************************/
 
/*
ヘッダを見て対応フォーマットか確認.
対応しているものならtrue,そうでなければfalseを返す.
filnameはファイル名(NULLが入っていることもある).
dataはファイルのヘッダで,サイズは HEADBUF_SIZE.
*/
BOOL IsSupportedEx(char *filename, char *data)
{

	if (data[0] != 'F' ||data[1] != 'L' ||data[2] != 'I'||data[3] != 'F')
        return FALSE;
	return TRUE;
}

/*
画像ファイルの情報を書き込みエラーコードを返す.
dataはファイルイメージで,サイズはdatasizeバイト.
*/
int GetPictureInfoEx(char *data, struct PictureInfo *lpInfo)
{
	int offset = 5;
    int c = data[4];
	c-=-' ';
    if (c > 47)
        offset++;
	int width=((unsigned char)data[offset++]) << 8;
    width += ((unsigned char)data[offset++]);
    int height=((unsigned char)data[offset++]) << 8;
    height += ((unsigned char)data[offset]);

	lpInfo->left		= 0;
	lpInfo->top			= 0;
	lpInfo->width		= width;
	lpInfo->height		= height;
	lpInfo->x_density	= 0;
	lpInfo->y_density	= 0;
	lpInfo->colorDepth	= 24;
	lpInfo->hInfo		= NULL;

	return SPI_ALL_RIGHT;
}
void rgb2bgr(uint8_t* rgb){
	uint8_t tmp = rgb[0];
	rgb[0]=rgb[2];
	rgb[2]=tmp;
}
/*
画像ファイルをDIBに変換し,エラーコードを返す.
dataはファイルイメージで,サイズはdatasizeバイト.
*/

void freeImg(Images &images){
	for (Image& image : images) {
		image.reset();
	}
	images.clear();
}

int GetPictureEx(long datasize, HANDLE *pHBInfo, HANDLE *pHBm,
			 SPI_PROGRESS lpPrgressCallback, long lData, char *data)
{

	int pipefd[2];
	int r=_pipe(pipefd, lData, _O_RDONLY |_O_BINARY);
	if (r == -1)
		return SPI_NOT_SUPPORT;
	_write(pipefd[1], data, datasize);
	_close(pipefd[1]);
	FILE *fp = _fdopen(pipefd[0], "rb");
	if (fp == 0){
		_close(pipefd[0]);
		return SPI_NOT_SUPPORT;
	}

	//FLIF
	FileIO fio(fp, "pipe");
    Images images;
	pixels_todo=0;
	pixels_done=0;


	unsigned int infosize;
	int height,y;

	unsigned int width, bit, linesize, imgsize,x;
	BITMAPINFO *pinfo;
	unsigned char *pbmdat,*tmp;

	if (lpPrgressCallback != NULL)
		if (lpPrgressCallback(0, 5, lData)){ /* 0% */
			return SPI_ABORT;
		}

	//FLIF
	if (!flif_decode(fio, images, 100, 1))
		return SPI_NOT_SUPPORT;

	Image& image = images[0];
	if (image.max(0) > 255){
		freeImg(images);
		return SPI_NOT_SUPPORT;
	}

	int nbplanes = image.numPlanes();
	if ( nbplanes == 4 ) nbplanes=3;
	if ( nbplanes != 1 && nbplanes != 3 ){
		freeImg(images);
		return SPI_NOT_SUPPORT;
	}
	int bit_depth = 8, bytes_per_value=1;

	width = image.cols();
	height = image.rows();

	bit = 24;
	infosize = sizeof(BITMAPINFOHEADER);

	linesize = (width * (bit / 8) +3) & ~3; /*4byte境界*/
	imgsize = linesize * abs(height);

	*pHBInfo = LocalAlloc(LMEM_MOVEABLE, infosize);
	*pHBm    = LocalAlloc(LMEM_MOVEABLE, imgsize);

	if (*pHBInfo == NULL || *pHBm == NULL) {
		if (*pHBInfo != NULL) LocalFree(*pHBInfo);
		if (*pHBm != NULL) LocalFree(*pHBm);
		freeImg(images);
		return SPI_NO_MEMORY;
	}

	pinfo  = (BITMAPINFO *)LocalLock(*pHBInfo);
	pbmdat = (unsigned char *)LocalLock(*pHBm);
	if (pinfo == NULL || pbmdat == NULL) {
		LocalFree(*pHBInfo);
		LocalFree(*pHBm);
		freeImg(images);
		return SPI_MEMORY_ERROR;
	}

	/* *pHBInfoにはBITMAPINFOを入れる */
	pinfo->bmiHeader.biSize				= sizeof(BITMAPINFOHEADER);
	pinfo->bmiHeader.biWidth			= width;
	pinfo->bmiHeader.biHeight			= height;
	pinfo->bmiHeader.biPlanes			= 1;
	pinfo->bmiHeader.biBitCount			= bit;
	pinfo->bmiHeader.biCompression		= BI_RGB;
	pinfo->bmiHeader.biSizeImage		= 0;
	pinfo->bmiHeader.biXPelsPerMeter	= 0;
	pinfo->bmiHeader.biYPelsPerMeter	= 0;
	pinfo->bmiHeader.biClrUsed			= 0;
	pinfo->bmiHeader.biClrImportant		= 0;

	
	/* *pHBmにはビットマップデータを入れる */
	if(nbplanes == 3){
		int h=0;
		for (y = height-1; y >= 0 ; --y){
			tmp = pbmdat + linesize*y;
			for (x=0; x < width; x++) {
				*tmp++ = (unsigned char) (image(2,h,x));
				*tmp++ = (unsigned char) (image(1,h,x));
				*tmp++ = (unsigned char) (image(0,h,x));
			}
			h++;
		}
	}else if(nbplanes == 1){
		int h=0;
		for (y = height-1; y >= 0 ; --y){
			tmp = pbmdat + linesize*y;
			for (x=0; x < width; x++) {
				*tmp = (unsigned char) (image(0,h,x));
				tmp[1] = tmp[0];
				tmp[2] = tmp[0];
				tmp+=3;
			}
			h++;
		}
	}

	freeImg(images);

	//メモリをアンロック
	LocalUnlock(*pHBInfo);
	LocalUnlock(*pHBm);

	if (lpPrgressCallback != NULL)
		if (lpPrgressCallback(1,1, lData)) /* 100% */
			return SPI_ABORT;

	return SPI_ALL_RIGHT;
}
//---------------------------------------------------------------------------
