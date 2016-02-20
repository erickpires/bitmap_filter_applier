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
    uint16 type;  // file magic number
    uint32 file_size;
    uint16 reserved1;  // must be 0
    uint16 reserved2;  // must be 0
    uint32 byte_offset;
}BitmapFileHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint32 header_size;
    int32 image_width;
    int32 image_height;
    uint16 n_color_planes; // must be 1
    uint16 bits_per_pixel;
    uint32 compression_type;
    uint32 image_size;  // size of image in bytes
    int32 x_pixels_per_meter;
    int32 y_pixels_per_meter;
    uint32 colors_used;
    uint32 colors_important;
}BitmapInfoHeader;
#pragma pack(pop)

void assert(bool condition, char* message) {
	if(!condition) {
		fprintf(stderr, "%s\n", message);
		exit(1);
	}
}

byte* load_bitmap_from_path(char* file_path,
							BitmapFileHeader* file_header,
						    BitmapInfoHeader* bmp_header) {
	FILE* file = fopen(file_path, "rb");
	assert(file != NULL,"Could not open file to read");

	fread(file_header, sizeof(BitmapFileHeader), 1, file);
	assert(file_header->type == BMP_MAGIC_NUMBER, "This is not a bitmap file");
	fread(bmp_header, sizeof(BitmapInfoHeader), 1, file);
	assert(bmp_header->compression_type == 0, "This program can not handle compressed bitmaps");

	fseek(file, file_header->byte_offset, SEEK_SET);

	byte* result = (byte*) malloc(bmp_header->image_size);
	assert(result != NULL, "Could not allocate memory");
	fread(result,bmp_header->image_size, 1, file);

	fclose(file);
	return result;
}

void write_bitmap_to_path(char* path, byte* bmp_data,
						  BitmapFileHeader* file_header,
						  BitmapInfoHeader* bmp_header) {
	FILE* file = fopen(path, "wb");
	assert(file != NULL, "Could not open file to write");

	fwrite(file_header, sizeof(BitmapFileHeader), 1, file);
	fwrite(bmp_header, sizeof(BitmapInfoHeader), 1, file);

	fwrite(bmp_data, bmp_header->image_size, 1, file);

	fclose(file);
}

int main(int argc, char** argv){

	BitmapFileHeader file_header = {};
	BitmapInfoHeader bmp_header = {};
	byte* image = load_bitmap_from_path("test.bmp", &file_header, &bmp_header);

	printf("Image has %dx%dx%d %dbits and %x compression\n", bmp_header.image_width, bmp_header.image_height, bmp_header.colors_important, bmp_header.bits_per_pixel, bmp_header.compression_type);

	write_bitmap_to_path("test2.bmp", image, &file_header, &bmp_header);

    return 0;
}
