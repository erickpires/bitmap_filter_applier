#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uint8_t byte;

typedef int bool;
#define FALSE 0
#define TRUE  1


#define BMP_MAGIC_NUMBER 0x4D42

#pragma pack(push, 1)
typedef struct {
    uint16 bfType;  //specifies the file type
    uint32 bfSize;  //specifies the size in bytes of the bitmap file
    uint16 bfReserved1;  //reserved; must be 0
    uint16 bfReserved2;  //reserved; must be 0
    uint32 bOffBits;  //species the offset in bytes from the bitmapfileheader to the bitmap bits
}BitmapFileHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint32 biSize;  //specifies the number of bytes required by the struct
    int32 biWidth;  //specifies width in pixels
    int32 biHeight;  //species height in pixels
    uint16 biPlanes; //specifies the number of color planes, must be 1
    uint16 biBitCount; //specifies the number of bit per pixel
    uint32 biCompression;//specifies the type of compression
    uint32 biSizeImage;  //size of image in bytes
    int32 biXPelsPerMeter;  //number of pixels per meter in x axis
    int32 biYPelsPerMeter;  //number of pixels per meter in y axis
    uint32 biClrUsed;  //number of colors used by the bitmap
    uint32 biClrImportant;  //number of colors that are important
}BitmapInfoHeader;
#pragma pack(pop)

void assert(bool condition, char* message) {
	if(!condition) {
		fprintf(stderr, "%s\n", message);
		exit(1);
	}
}

// README(erick) : Is it interesting to pass the file header as argument?
byte* load_bitmap_from_path(char* file_path,
						    BitmapInfoHeader* bmp_header) {
	BitmapFileHeader fileHeader = {};
	FILE* file = fopen(file_path, "rb");
	assert(file != NULL,"Could not open file");

	fread(&fileHeader, sizeof(BitmapFileHeader), 1, file);
	assert(fileHeader.bfType == BMP_MAGIC_NUMBER, "This is not a bitmap file");
	fread(bmp_header, sizeof(BitmapInfoHeader), 1, file);

	fseek(file, fileHeader.bOffBits, SEEK_SET);

	byte* result = (byte*) malloc(bmp_header->biSizeImage);
	assert(result != NULL, "Could not allocate memory");
	fread(result, sizeof(byte), bmp_header->biSizeImage, file);

	fclose(file);
	return result;
}

int main(int argc, char** argv){

	BitmapInfoHeader bmp_header = {};
	byte* image = load_bitmap_from_path("test.bmp", &bmp_header);

	printf("Image has %dx%dx%d %dbits and %x compression\n", bmp_header.biWidth, bmp_header.biHeight, bmp_header.biClrImportant, bmp_header.biBitCount, bmp_header.biCompression);

    return 0;
}
