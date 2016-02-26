#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdarg.h>

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
#define COLOR_MAX 255

#if 0
/*
 * Constants for the biCompression field...
 */

#  define BI_RGB       0             /* No compression - straight BGR data */
#  define BI_RLE8      1             /* 8-bit run-length compression */
#  define BI_RLE4      2             /* 4-bit run-length compression */
#  define BI_BITFIELDS 3             /* RGB bitmap with RGB masks */

typedef struct                       /**** Colormap entry structure ****/
    {
    unsigned char  rgbBlue;          /* Blue value */
    unsigned char  rgbGreen;         /* Green value */
    unsigned char  rgbRed;           /* Red value */
    unsigned char  rgbReserved;      /* Reserved */
    } RGBQUAD;

typedef struct                       /**** Bitmap information structure ****/
    {
    BITMAPINFOHEADER bmiHeader;      /* Image header */
    RGBQUAD          bmiColors[256]; /* Image colormap */
    } BITMAPINFO;
#endif

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

typedef struct {
	byte b;
	byte g;
	byte r;
} Pixel;

// PixelFloat is an struct representing a pixel where each color is a
// floating-point number between 0.0 and 1.0
typedef struct {
	union {
		float b;
		float y;
		float Cr;
	};
	union {
		float g;
		float m;
		float Cb;
	};
	union {
		float r;
		float c;
		float Y;
	};
} PixelFloat;

#define get_matrix_value(matrix, line, column) ((matrix).values[line * (matrix).n_columns + column])
typedef struct {
	float* values;
	uint n_lines;
	uint n_columns;
} FloatMatrix;

typedef struct {
	PixelFloat* values;
	uint n_lines;
	uint n_columns;
} PixelFloatMatrix;

typedef enum {
	Red 			= 0x01,
	Green 			= 0x02,
	Blue 			= 0x04,
	RedGreen		= Red 	| Green,
	GreenRed		= Red 	| Green,
	RedBlue			= Red 	| Blue,
	BlueRed			= Red 	| Blue,
	GreenBlue		= Green | Blue,
	BlueGreen		= Green | Blue,
	RedGreenBlue	= Red 	| Green | Blue
} Colors;

void apply_transformation_to_pixel(PixelFloat* pixel, FloatMatrix* transform) {
	PixelFloat tmp_pixel = {};
	tmp_pixel.r = get_matrix_value(*transform, 0, 0) * pixel->r +
				  get_matrix_value(*transform, 0, 1) * pixel->g +
				  get_matrix_value(*transform, 0, 2) * pixel->b;

	tmp_pixel.g = get_matrix_value(*transform, 1, 0) * pixel->r +
				  get_matrix_value(*transform, 1, 1) * pixel->g +
				  get_matrix_value(*transform, 1, 2) * pixel->b;

	tmp_pixel.b = get_matrix_value(*transform, 2, 0) * pixel->r +
				  get_matrix_value(*transform, 2, 1) * pixel->g +
				  get_matrix_value(*transform, 2, 2) * pixel->b;

	pixel->r = tmp_pixel.r;
	pixel->g = tmp_pixel.g;
	pixel->b = tmp_pixel.b;
}

void apply_transformation_image(PixelFloatMatrix* image, FloatMatrix* transform) {
	for(uint line = 0; line < image->n_lines; line++) {
		for(uint column = 0; column < image->n_columns; column++) {
			PixelFloat* pixel = &(get_matrix_value(*image, line, column));
			apply_transformation_to_pixel(pixel, transform);
		}
	}
}

void assert(bool condition, char* message, ...) {
	va_list args_list;
	va_start(args_list, message);

	if(!condition) {
		vfprintf(stderr, message, args_list);
		// *((int*)NULL) = 0;
		va_end(args_list);
		exit(1);
	}
	va_end(args_list);
}

void convert_image_to_float(byte* colors, float* float_colors, uint n_bytes) {
	for(uint byte_index = 0; byte_index < n_bytes; byte_index++) {
		*float_colors++ = (float) *colors++ / (float) COLOR_MAX;
	}
}

void convert_image_from_float(float* float_colors, byte* colors, uint n_bytes) {
	for(uint byte_index = 0; byte_index < n_bytes; byte_index++) {
		*colors++ = (byte) (*float_colors++ * (float) COLOR_MAX);
	}
}

#define convert_cmy_to_rgb convert_rgb_to_cmy
void convert_rgb_to_cmy(PixelFloatMatrix* image) {
	for(uint line = 0; line < image->n_lines; line++) {
		for(uint column = 0; column < image->n_columns; column++) {
			PixelFloat* current_pixel = &(get_matrix_value(*image, line, column));
			current_pixel->c = 1.0 - current_pixel->r;
			current_pixel->m = 1.0 - current_pixel->g;
			current_pixel->y = 1.0 - current_pixel->b;
		}
	}
}

// NOTE(erick): https://msdn.microsoft.com/en-us/library/ff635643.aspx
void convert_rgb_to_yCbCr(PixelFloatMatrix* image) {
	static float conversion_values[] =
	{
		 0.299,     0.587,     0.114,
		-0.168935, -0.331665,  0.50059,
		 0.499813, -0.418531, -0.081282
	};
	static FloatMatrix conversion = {conversion_values, 3, 3};
	apply_transformation_image(image, &conversion);
}
void convert_yCbCr_to_rgb(PixelFloatMatrix* image) {
	static float conversion_values[] =
	{
		1.0     ,  -0.0    ,   1.40252,
   		0.999997,  -0.34373,  -0.714401,
   		1.00002 ,   1.76991,  -0.000012644
	};
	static FloatMatrix conversion = {conversion_values, 3, 3};
	apply_transformation_image(image, &conversion);
}

void yCbCr_image_to_black_and_white(PixelFloatMatrix* image) {
	for(uint line = 0; line < image->n_lines; line++) {
		for(uint column = 0; column < image->n_columns; column++) {
			get_matrix_value(*image, line, column).Cb = 0.0f;
			get_matrix_value(*image, line, column).Cr = 0.0f;
		}
	}
}

PixelFloat apply_filter_at_selectively(PixelFloatMatrix* image, FloatMatrix* filter, uint i, uint j,
									   Colors affected_colors) {
	PixelFloat result = {};

	int line_base = i - (filter->n_lines / 2);
	int column_base = j - (filter->n_columns / 2);

	for(uint line = 0; line < filter->n_lines; line++) {
		int line_pos = line_base + line;
		// NOTE(erick): Assuming that the filter is well behaved and isn't bigger
		//              then the image.
#if WRAP_AROUND
		if(line_pos < 0) line_pos += image->n_lines;
		if(line_pos >= image->n_lines) line_pos -= image->n_lines;
#else // Repeating
		if(line_pos < 0) line_pos = 0;
		if(line_pos >= image->n_lines) line_pos = image->n_lines - 1;
#endif
		for(uint column = 0; column < filter->n_columns; column++) {
			int column_pos = column_base + column;
#if WRAP_AROUND
			if(column_pos < 0) column_pos += image->n_columns;
			if(column_pos >= image->n_columns) column_pos -= image->n_columns;
#else // Repeating
			if(column_pos < 0) column_pos = 0;
			if(column_pos >= image->n_columns) column_pos = image->n_columns - 1;
#endif
			// NOTE(erick): The image position is assumed to be sanitized by now
			PixelFloat pixel = get_matrix_value(*image, line_pos, column_pos);
			float filter_value = get_matrix_value(*filter, line, column);
			if(affected_colors & Red)   result.r +=  pixel.r * filter_value;
			if(affected_colors & Green) result.g +=  pixel.g * filter_value;
			if(affected_colors & Blue)  result.b +=  pixel.b * filter_value;
		}
	}

	if(!(affected_colors & Red)) 	result.r = get_matrix_value(*image, i, j).r;
	if(!(affected_colors & Green)) 	result.g = get_matrix_value(*image, i, j).g;
	if(!(affected_colors & Blue))  	result.b = get_matrix_value(*image, i, j).b;

	return result;
}

void apply_filter_to_image_selectively(PixelFloatMatrix* input_image, PixelFloatMatrix* output_image,
									   FloatMatrix* filter, Colors affected_colors) {
	for(uint line = 0; line < input_image->n_lines; line++) {
		for(uint column = 0; column < input_image->n_columns; column++) {
			PixelFloat applied = apply_filter_at_selectively(input_image, filter, line, column, affected_colors);
			PixelFloat* output_pixel = &(get_matrix_value(*output_image, line, column));
			output_pixel->r = applied.r;
			output_pixel->g = applied.g;
			output_pixel->b = applied.b;
		}
	}
}

void apply_filter_to_image(PixelFloatMatrix* input_image, PixelFloatMatrix* output_image,
						   FloatMatrix* filter) {
	apply_filter_to_image_selectively(input_image, output_image, filter, RedGreenBlue);
}

void apply_sobel_operator(PixelFloatMatrix* input_image, PixelFloatMatrix* output_image) {
	static float sobel_x_values[] =
	{-1,  0,  1,
	 -2,  0,  2,
	 -1,  0,  1};
	static float sobel_y_values[] =
	{-1, -2, -1,
	  0,  0,  0,
	  1,  2,  1};

	static FloatMatrix sobel_x = {sobel_x_values, 3, 3};
	static FloatMatrix sobel_y = {sobel_y_values, 3, 3};

	convert_rgb_to_yCbCr(input_image);
	yCbCr_image_to_black_and_white(input_image);
	for(uint line = 0; line < input_image->n_lines; line++) {
		for(uint column = 0; column < input_image->n_columns; column++) {
			// TODO(erick): Red should be Luminescence
			PixelFloat applied_x = apply_filter_at_selectively(input_image, &sobel_x, line, column, Red);
			PixelFloat applied_y = apply_filter_at_selectively(input_image, &sobel_y, line, column, Red);

			float g_x = applied_x.Y;
			float g_y = applied_y.Y;

			float magnitude = sqrtf(g_x * g_x + g_y * g_y) / 8.0f; // NOTE(erick):Normalized
			float angle = atan2f(g_y, g_x);

			get_matrix_value(*output_image, line, column).Y = magnitude;
			get_matrix_value(*output_image, line, column).Cb = cos(angle) / 2.0 * magnitude;
			get_matrix_value(*output_image, line, column).Cr = 0.0f;
		}
	}
	convert_yCbCr_to_rgb(output_image);
}

void normalize_filter(FloatMatrix* filter) {
	float acc = 0;
	for(uint line = 0; line < filter->n_lines; line++) {
		for(uint column = 0; column < filter->n_columns; column++) {
			acc += get_matrix_value(*filter, line, column);
		}
	}
	for(uint line = 0; line < filter->n_lines; line++) {
		for(uint column = 0; column < filter->n_columns; column++) {
			get_matrix_value(*filter, line, column) /= acc;
		}
	}
}

void print_filter(FloatMatrix* filter) {
	for(uint line = 0; line < filter->n_lines; line++) {
		for(uint column = 0; column < filter->n_columns; column++) {
			printf("%f ", get_matrix_value(*filter, line, column));
		}
		printf("\n");
	}
}

byte* load_bitmap_from_path(char* file_path,
							BitmapFileHeader* file_header,
						    BitmapInfoHeader* bmp_header) {
	FILE* file = fopen(file_path, "rb");
	assert(file != NULL,"Could not open file to read\n");

	fread(file_header, sizeof(BitmapFileHeader), 1, file);
	assert(file_header->type == BMP_MAGIC_NUMBER, "This is not a bitmap file\n");
	fread(bmp_header, sizeof(BitmapInfoHeader), 1, file);
	assert(bmp_header->compression_type == 0, "This program can not handle compressed bitmaps\n");

	fseek(file, file_header->byte_offset, SEEK_SET);

	byte* result = (byte*) malloc(bmp_header->image_size);
	assert(result != NULL, "Could not allocate memory\n");
	fread(result,bmp_header->image_size, 1, file);

	fclose(file);
	return result;
}

void swap_colors(Pixel* pixel, Colors swapping) {
	byte* left;
	byte* right;

	switch(swapping) {
		case RedGreen :
			left = &(pixel->r);
			right = &(pixel->g);
			break;
		case RedBlue :
			left = &(pixel->r);
			right = &(pixel->b);
			break;
		case GreenBlue :
			left = &(pixel->g);
			right = &(pixel->b);
			break;
		default:
			return;
	}

	*left  ^= *right;
	*right ^= *left;
	*left  ^= *right;
}

void write_bitmap_to_path(char* path, byte* bmp_data,
						  BitmapFileHeader* file_header,
						  BitmapInfoHeader* bmp_header) {
	FILE* file = fopen(path, "wb");
	assert(file != NULL, "Could not open file to write\n");

	fwrite(file_header, sizeof(BitmapFileHeader), 1, file);
	fwrite(bmp_header, sizeof(BitmapInfoHeader), 1, file);

	fwrite(bmp_data, bmp_header->image_size, 1, file);

	fclose(file);
}

int main(int argc, char** argv){

	assert(argc == 3, "Usage:\n%s <input> <output>\n", argv[0]);

	char* input_file_path = argv[1];
	char* output_file_path= argv[2];

	BitmapFileHeader file_header = {};
	BitmapInfoHeader bmp_header = {};
	byte* image = load_bitmap_from_path(input_file_path, &file_header, &bmp_header);

	float* image_float = (float*) calloc(bmp_header.image_size, sizeof(float));
	float* image_float_applied = (float*) calloc(bmp_header.image_size, sizeof(float));

	convert_image_to_float(image, image_float, bmp_header.image_size);

	// float filter_values[] =
	// {1,  4,  6,  4, 1,
	//  4, 16, 24, 16, 4,
	//  6, 24, 36, 24, 6,
	//  4, 16, 24, 16, 4,
	//  1,  4,  6,  4, 1};

	// float filter_values[] =
	// { 1,   6,  15,  20,  15,   6,  1,
	//   6,  36,  90, 120,  90,  36,  6,
	//  15,  90, 225, 300, 225,  90, 15,
	//  20, 120, 300, 400, 300, 120, 20,
	//  15,  90, 225, 300, 225,  90, 15,
	//   6,  36,  90, 120,  90,  36,  6,
	//   1,   6,  10,  20,  10,   6,  1};

	float filter_values[] =
	{
		0.00000067, 0.00002292,	0.00019117,	0.00038771,	0.00019117,	0.00002292,	0.00000067,
		0.00002292, 0.00078634,	0.00655965,	0.01330373,	0.00655965,	0.00078633,	0.00002292,
		0.00019117, 0.00655965,	0.05472157,	0.11098164,	0.05472157,	0.00655965,	0.00019117,
		0.00038771, 0.01330373,	0.11098164,	0.22508352,	0.11098164,	0.01330373,	0.00038771,
		0.00019117, 0.00655965,	0.05472157,	0.11098164,	0.05472157,	0.00655965,	0.00019117,
		0.00002292, 0.00078633,	0.00655965,	0.01330373,	0.00655965,	0.00078633,	0.00002292,
		0.00000067, 0.00002292,	0.00019117,	0.00038771,	0.00019117,	0.00002292,	0.00000067
	};


	FloatMatrix filter = {};
	filter.values = filter_values;
	filter.n_lines = 7;
	filter.n_columns = 7;

	PixelFloatMatrix input_image = {};
	input_image.values = (PixelFloat*) image_float;
	input_image.n_lines = bmp_header.image_height;
	input_image.n_columns = bmp_header.image_width;

	PixelFloatMatrix output_image = {};
	output_image.values = (PixelFloat*) image_float_applied;
	output_image.n_lines = bmp_header.image_height;
	output_image.n_columns = bmp_header.image_width;

	normalize_filter(&filter);
	apply_filter_to_image(&input_image, &output_image, &filter);
	apply_filter_to_image(&output_image, &input_image, &filter);
	// apply_filter_to_image(&input_image, &output_image, &filter);
	// apply_filter_to_image(&output_image, &input_image, &filter);
	// apply_filter_to_image(&input_image, &output_image, &filter);
	// apply_filter_to_image(&output_image, &input_image, &filter);

	// for(uint pixel_index = 0; pixel_index < bmp_header.image_size; pixel_index += 3) {
	// 	Pixel* pixel = (Pixel*) (image + pixel_index);
	// 	switch_colors(pixel, RedBlue);
	// 	switch_colors(pixel, GreenBlue);
	// }
	// convert_rgb_to_cmy(&output_image);
	// convert_cmy_to_rgb(&output_image);

	// convert_rgb_to_yCbCr(&output_image);
	apply_sobel_operator(&input_image, &output_image);
	// convert_yCbCr_to_rgb(&output_image);

	convert_image_from_float(image_float_applied, image, bmp_header.image_size);

	BitmapFileHeader file_header_to_write = {};
	BitmapInfoHeader bmp_header_to_write = {};

	file_header_to_write.type = BMP_MAGIC_NUMBER;
	file_header_to_write.file_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + bmp_header.image_size;
	file_header_to_write.byte_offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);

	bmp_header_to_write.header_size = sizeof(BitmapInfoHeader);
	bmp_header_to_write.image_width = bmp_header.image_width;
	bmp_header_to_write.image_height = bmp_header.image_height;
	bmp_header_to_write.n_color_planes = 1;
	bmp_header_to_write.compression_type = 0;
	bmp_header_to_write.bits_per_pixel = bmp_header.bits_per_pixel;
	bmp_header_to_write.image_size = bmp_header.image_size;
	bmp_header_to_write.x_pixels_per_meter = bmp_header.x_pixels_per_meter;
	bmp_header_to_write.y_pixels_per_meter = bmp_header.y_pixels_per_meter;
	bmp_header_to_write.colors_used = bmp_header.colors_used;
	bmp_header_to_write.colors_important = bmp_header.colors_important;

	printf("Image has %dx%dx%d %dbits and %x compression\n", bmp_header.image_width, bmp_header.image_height, bmp_header.colors_important, bmp_header.bits_per_pixel, bmp_header.compression_type);

	write_bitmap_to_path(output_file_path, image, &file_header_to_write, &bmp_header_to_write);

    return 0;
}
