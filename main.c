#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

//비트맵 헤더를 묶음 (file header, info header)
typedef struct tagBITMAPHEADER {
	BITMAPFILEHEADER bf;
	BITMAPINFOHEADER bi;
}BITMAPHEADER;

typedef struct tagBITMAPDATASIZE {
	int size;
	int width;
	int height;
	int bitcount;
	int offbits;
	int direction;
}BITMAPDATASIZE;

void MergeFourBitmapFile(const char* inputFileName1, const char* inputFileName2, const char* inputFileName3,
	const char* inputFileName4, const char* outputFileName);
//비트맵을 읽어와서 bmp파일 data의 포인터를 리턴
BYTE* LoadBitmapFile(BITMAPHEADER* bitmapHeader, BITMAPDATASIZE* imgSize, const char* fileName);
//비트맵 파일을 쓰기
void WriteBitmapFile(BITMAPHEADER outputHeader, BYTE* output, BITMAPDATASIZE imgSize, const char* fileName);



void MergeFourBitmapFile(const char* inputFileName1, const char* inputFileName2, const char* inputFileName3,
	const char* inputFileName4, const char* outputFileName) {
	//비트맵의 헤더부분을 파일에서 읽어 저장할 구조체
	BITMAPHEADER firstHeader, secondHeader, thirdHeader, fourthHeader;	
	//합쳐진 헤더부분을 저장할 구조체
	BITMAPHEADER outputHeader;		
	// input bmp파일의 크기를 저장할 변수
	BITMAPDATASIZE firstSize, secondSize, thirdSize, fourthSize;	
	//output bmp파일의 크기를 저장할 변수
	BITMAPDATASIZE outputSize;		

	//비트맵파일을 읽어 이미지 정보(header, data)를 저장
	BYTE* image = LoadBitmapFile(&firstHeader, &firstSize, inputFileName1);
	if (image == NULL) {
		return ;
	}
	BYTE* image2 = LoadBitmapFile(&secondHeader, &secondSize, inputFileName2);
	if (image2 == NULL) {
		return ;
	}
	BYTE* image3 = LoadBitmapFile(&thirdHeader, &thirdSize, inputFileName3);
	if (image3 == NULL) {
		return ;
	}
	BYTE* image4 = LoadBitmapFile(&fourthHeader, &fourthSize, inputFileName4);
	if (image4 == NULL) {
		return ;
	}
	
	// header 정보 수정
	outputHeader = firstHeader;
	// row direction이 Top - Bottom 일 경우 header에 저장되어 있는 height이 음수이기에 양수로 바꿔줌
	if (firstSize.direction == 0) {
		outputHeader.bi.biHeight = firstSize.height;
	}
	outputHeader.bf.bfSize += secondHeader.bf.bfOffBits + secondSize.size;
	outputHeader.bf.bfSize += thirdHeader.bf.bfOffBits + thirdSize.size;
	outputHeader.bf.bfSize += fourthHeader.bf.bfOffBits + fourthSize.size;

	outputHeader.bi.biWidth += secondSize.width;
	outputHeader.bi.biHeight += thirdSize.height;

	outputHeader.bi.biSizeImage += secondHeader.bf.bfOffBits + secondSize.size;
	outputHeader.bi.biSizeImage += thirdHeader.bf.bfOffBits + thirdSize.size;
	outputHeader.bi.biSizeImage += fourthHeader.bf.bfOffBits + fourthSize.size;

	//저장할 데이터 크기 지정
	outputSize.size = (firstSize.width + secondSize.width) * (firstSize.height + thirdSize.height) * firstSize.bitcount;

	// 결과값을 지정할 포인터 선언 및 메모리 할당
	BYTE* outputs = (BYTE*)malloc(outputSize.size + firstSize.width * firstSize.bitcount);
	BYTE* output = outputs;

	memset(outputs, 0x00, outputSize.size + firstSize.width * firstSize.bitcount);
	output += outputSize.size;

	// row direction에 따라 데이터를 읽는 방향을 조절하여 복사
	for (int i = 0; i < firstSize.height; i++) {
		memcpy(output, image + abs(firstSize.direction - i) * firstSize.width * firstSize.bitcount + firstSize.offbits,
			firstSize.width * firstSize.bitcount);
		output -= firstSize.width * firstSize.bitcount;

		memcpy(output, image2 + abs(secondSize.direction - i) * secondSize.width * secondSize.bitcount + secondSize.offbits,
			secondSize.width * secondSize.bitcount);
		output -= secondSize.width * secondSize.bitcount;
	}
	for (int i = 0; i < thirdSize.height; i++) {
		memcpy(output, image3 + abs(thirdSize.direction - i) * thirdSize.width * thirdSize.bitcount + thirdSize.offbits,
			thirdSize.width * thirdSize.bitcount);
		output -= thirdSize.width * thirdSize.bitcount;

		memcpy(output, image4 + abs(fourthSize.direction - i) * fourthSize.width * fourthSize.bitcount + fourthSize.offbits,
			fourthSize.width * fourthSize.bitcount);
		output -= fourthSize.width * fourthSize.bitcount;
	}

	//bmp 파일 생성
	WriteBitmapFile(outputHeader, outputs, outputSize, "result.bmp");

	//메모리 해제
	free(image);
	free(image2);
	free(image3);
	free(image4);
	free(outputs);

	return ;
}


BYTE* LoadBitmapFile(BITMAPHEADER* bitmapHeader, BITMAPDATASIZE* imgSize, const char* fileName)
{
	//이미지 파일을 이진 읽기 모드로 열기
	FILE* fp = fopen(fileName, "rb");
	long size;
	if (fp == NULL) {
		printf("Fail to load %s file \n", fileName);
		return NULL;
	} else {
		if (fread(&bitmapHeader->bf, sizeof(BITMAPFILEHEADER), 1, fp) < 1) {
			fclose(fp);
			return NULL;
		}
		if (fread(&bitmapHeader->bi, sizeof(BITMAPINFOHEADER), 1, fp) < 1) {
			fclose(fp);
			return NULL;
		}

		//문제의 조건인 24bit , width : 400, height : 300 검사
		if (bitmapHeader->bi.biBitCount != 24) {
			printf("%s's bitcount is not 24 \n", fileName);
			fclose(fp);
			return NULL;
		}
		if (bitmapHeader->bi.biWidth != 400) {
			printf("%s's width is not 400 pixel \n", fileName);
			fclose(fp);
			return NULL;
		}
		// 높이가 300이 아닐 경우에 높이 값이 -300 인지 검사
		// row direction이 top - bottom 일 경우 height가 음수가 되기 때문
		if (bitmapHeader->bi.biHeight != 300) {
			if (bitmapHeader->bi.biHeight == -300) {
				printf("%s's row direction is Top - Bottom \n", fileName);
				imgSize->direction = 0;
			} else {
				printf("%s's height is not 300 pixel \n", fileName);
				fclose(fp);
				return NULL;
			}
		} else {
			// row direction이 bottom - top (default)
			imgSize->direction = bitmapHeader->bi.biHeight - 1;
		}

		//BITMAPDATASIZE에 관련 요소 저장
		size = bitmapHeader->bi.biSizeImage;
		imgSize->width = bitmapHeader->bi.biWidth;
		imgSize->height = abs(bitmapHeader->bi.biHeight);
		imgSize->bitcount = bitmapHeader->bi.biBitCount / 8;

		// jpg -> bmp으로 변환한 그림 이용시 offbits가 54가 아닌 138인 경우가 존재하였기에 이를 수정하기 위한 값 추가/변경
		imgSize->offbits = bitmapHeader->bf.bfOffBits - (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
		bitmapHeader->bf.bfOffBits -= imgSize->offbits;
		bitmapHeader->bf.bfSize -= imgSize->offbits;
		bitmapHeader->bi.biSize -= imgSize->offbits;

		// 압축되지 않은 RGB bmp 파일은 biSizeImage가 0인 경우가 있어 0인 경우 값 바꿔주기
		if (size == 0) {
			size = bitmapHeader->bi.biWidth * abs(bitmapHeader->bi.biHeight) * bitmapHeader->bi.biBitCount / 8;
		}

		imgSize->size = size;

		// 이미지의 data 부분을 저장하기 위해서 메모리 할당
		BYTE* image = (BYTE*)malloc(size);
		//이미지 크기만큼 파일에서 읽어오기
		fread(image, size, 1, fp);
		if (image == NULL) {
			printf("Couldn't get image data \n");
			fclose(fp);
			return NULL;
		}

		// fopen 닫기
		fclose(fp);

		return image;
	}
}


void WriteBitmapFile(BITMAPHEADER outputHeader, BYTE* output, BITMAPDATASIZE imgSize, const char* fileName)
{
	FILE* fp = fopen(fileName, "wb");

	fwrite(&outputHeader.bf, sizeof(BITMAPFILEHEADER), 1, fp);
	fwrite(&outputHeader.bi, sizeof(BITMAPINFOHEADER), 1, fp);
	fwrite(output, 1, imgSize.size, fp);

	printf("file create! \n");
	fclose(fp);
}


int main() {
	const char* inputFileName1 = "1.bmp";
	const char* inputFileName2 = "2.bmp";
	const char* inputFileName3 = "3.bmp";
	const char* inputFileName4 = "4.bmp";
	const char* outputFileName = "result.bmp";

	MergeFourBitmapFile(inputFileName1, inputFileName2, inputFileName3, inputFileName4,
		outputFileName);

	return 0;
}
