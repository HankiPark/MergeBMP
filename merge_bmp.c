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
//파일이름 설정
char* SetInputFileName(char* word);
//파일 width, height 비교
int SizeComparison(BITMAPDATASIZE* firstSize, BITMAPDATASIZE* secondSize, BITMAPDATASIZE* thirdSize, BITMAPDATASIZE* fourthSize)


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
	
	if (SizeComparison(&firstSize, &secondSize, &thirdSize, &fourthSize) == 0) {
		printf("The file size are different. \n");
		free(image);
		free(image2);
		free(image3);
		free(image4);
		return;
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
	WriteBitmapFile(outputHeader, outputs, outputSize, outputFileName);

	//메모리 해제
	free(image);
	free(image2);
	free(image3);
	free(image4);
	free(outputs);

	return ;
}

int SizeComparison(BITMAPDATASIZE* firstSize, BITMAPDATASIZE* secondSize, BITMAPDATASIZE* thirdSize, BITMAPDATASIZE* fourthSize) {

	if (firstSize->width != secondSize->width || firstSize->width != thirdSize->width || firstSize->width != fourthSize->width) {
		printf("The widths of the four files are not the same. \n");
		return 0;
	}
	if (firstSize->height != secondSize->height || firstSize->height != thirdSize->height || firstSize->height != fourthSize->height) {
		printf("The heights of the four files are not the same. \n");
		return 0;
	}


	return 1;
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

		// row direction이 top - bottom 일 경우 height가 음수가 되기 때문에 방향 값 지정
		if (bitmapHeader->bi.biHeight < 0) {
			printf("%s's row direction is Top - Bottom \n", fileName);
			imgSize->direction = 0;
		} else {
			imgSize->direction = bitmapHeader->bi.biHeight - 1;
		}

		// bitcount가 24인지 검사
		if (bitmapHeader->bi.biBitCount != 24) {
			printf("%s's bitcount is not 24 \n", fileName);
			fclose(fp);
			return NULL;
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

char* SetFileName(char* word) {
	char temp[255];
	char* p;

	printf("\nPlease enter the \"%s\" file name or full path (max length : 255)\n ", word);
	printf("Note\n \t1.skip file extension (ex : 1.bmp(X) 1(O))\n \t2.use only English and numbers\n");
	fflush(stdout);
	fgets(temp, sizeof(temp), stdin);
	if ((p = strchr(temp, '\n')) != NULL) {
		*p = '\0';
	}
	size_t len = strlen(temp);
	char* input = malloc (len + 5);
	strcpy(input, temp);
	input[len] = '.';
	input[len + 1] = 'b';
	input[len + 2] = 'm';
	input[len + 3] = 'p';
	input[len + 4] = '\0';

	//fflush(stdout);
	return input;
}

int main() {
	const char* inputFileName1 = SetFileName("first");
	const char* inputFileName2 = SetFileName("second");
	const char* inputFileName3 = SetFileName("third");
	const char* inputFileName4 = SetFileName("fourth");
	const char* outputFileName = SetFileName("output");

	MergeFourBitmapFile(inputFileName1, inputFileName2, inputFileName3, inputFileName4,
		outputFileName);

	return 0;
}
