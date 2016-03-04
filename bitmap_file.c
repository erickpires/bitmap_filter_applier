#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdarg.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef unsigned int uint;
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

typedef struct {
	float r;
	float i;
}complexf;

complexf complexf_sum(complexf left, complexf right) {
	complexf result = {left.r + right.r, left.i + right.i};
	return result;
}

//NOTE(erick): (a+bi)*(c+di) = ac+adi+bci-db = (ac-db)+(ad+bc)i
complexf complexf_mul(complexf left, complexf right) {
	complexf result;
	result.r = left.r * right.r - left.i * right.i;
	result.i = left.r * right.i + left.i * right.r;
	return result;
}

complexf complexf_square(complexf number) {
	complexf result = complexf_mul(number, number);
	return result;
}

float complexf_norm_sq(complexf number) {
	float result = number.r * number.r + number.i * number.i;
	return result;
}

float complexf_norm(complexf number) {
	float result = sqrtf(complexf_norm_sq(number));
	return result;
}

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
		float Y;
	};
	union {
		float g;
		float m;
		float Cb;
		float h;
	};
	union {
		float r;
		float c;
		float Cr;
		float s;
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
	PixelFloat tmp_pixel = {0};
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


void convert_image_to_float(byte* colors, PixelFloatMatrix* float_image, uint n_bytes) {
	PixelFloat* pixel = float_image->values;
	for(uint byte_index = 0; byte_index < n_bytes; byte_index += 3) {
		pixel->b = (float) *colors++ / (float) COLOR_MAX;
		pixel->g = (float) *colors++ / (float) COLOR_MAX;
		pixel->r = (float) *colors++ / (float) COLOR_MAX;
		pixel++;
	}
}

void convert_image_from_float(PixelFloatMatrix* float_image, byte* colors, uint n_bytes) {
	PixelFloat* pixel = float_image->values;
	for(uint byte_index = 0; byte_index < n_bytes; byte_index += 3) {
		*colors++ = (byte) (pixel->b * (float) COLOR_MAX);
		*colors++ = (byte) (pixel->g * (float) COLOR_MAX);
		*colors++ = (byte) (pixel->r * (float) COLOR_MAX);
		pixel++;
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

//NOTE(erick): References
//			   http://kickjava.com/src/org/eclipse/swt/graphics/RGB.java.htm
//			   http://www.docjar.com/html/api/java/awt/Color.java.html
PixelFloat convert_rgb_pixel_to_hsb(PixelFloat* pixel) {
	PixelFloat result = {0};

    float r = pixel->r;
    float g = pixel->g;
    float b = pixel->b;
    float max = fmaxf(fmaxf(r, g), b);
    float min = fminf(fminf(r, g), b);
    float delta = max - min;

	result.b = max;
	result.s = max == 0 ? 0 : delta / max;
	if(delta != 0) {
	    if(r == max) {
	        result.h = (g - b) / delta;
	    }
	    else if (g == max) {
	            result.h = 2 + (b - r) / delta;
	        }
	    else {
	        result.h = 4 + (r - g) / delta;
	    }

	    result.h *= 60;
    	if (result.h < 0) result.h += 360;
    }

    return result;
}

//NOTE(erick): From the code used as reference, hue ranges from 0 to 360
//			   brightness from 0 to 1 and saturation from 0 to 1
//TODO(erick): Verify these ranges
PixelFloat convert_hsb_pixel_to_rgb(PixelFloat* pixel) {
    PixelFloat result;
    float hue = pixel->h;
    float saturation = pixel->s;
    float brightness = pixel->b;
    if (saturation == 0) {
        result.r = brightness;
        result.g = brightness;
        result.b = brightness;
    }
    else {
        if (hue == 360) hue = 0;
        hue /= 60;
        int i = (int)hue;
        float f = hue - i;
        float p = brightness * (1 - saturation);
        float q = brightness * (1 - saturation * f);
        float t = brightness * (1 - saturation * (1 - f));
        switch(i) {
            case 0:
                result.r = brightness;
                result.g = t;
                result.b = p;
                break;
            case 1:
                result.r = q;
                result.g = brightness;
                result.b = p;
                break;
            case 2:
                result.r = p;
                result.g = brightness;
                result.b = t;
                break;
            case 3:
                result.r = p;
                result.g = q;
                result.b = brightness;
                break;
            case 4:
                result.r = t;
                result.g = p;
                result.b = brightness;
                break;
            case 5:
            default:
                result.r = brightness;
                result.g = p;
                result.b = q;
                break;
        }
    }
    return result;
}

void convert_rgb_to_hsb(PixelFloatMatrix* image) {
	for(uint line = 0; line < image->n_lines; line++) {
		for(uint column = 0; column < image->n_columns; column++) {
			PixelFloat* pixel = &(get_matrix_value(*image, line, column));
			PixelFloat new_pixel = convert_rgb_pixel_to_hsb(pixel);
			pixel->h = new_pixel.h;
			pixel->s = new_pixel.s;
			pixel->b = new_pixel.b;
		}
	}
}

void convert_hsb_to_rgb(PixelFloatMatrix* image) {
	for(uint line = 0; line < image->n_lines; line++) {
		for(uint column = 0; column < image->n_columns; column++) {
			PixelFloat* pixel = &(get_matrix_value(*image, line, column));
			PixelFloat new_pixel = convert_hsb_pixel_to_rgb(pixel);
			pixel->r = new_pixel.r;
			pixel->g = new_pixel.g;
			pixel->b = new_pixel.b;
		}
	}
}


// NOTE(erick): https://msdn.microsoft.com/en-us/library/ff635643.aspx
//				modified because our RGB and YCbCr doesn't match
void convert_rgb_to_yCbCr(PixelFloatMatrix* image) {
	static float conversion_values[] =
	{
		 0.499813, -0.418531, -0.081282,
		-0.168935, -0.331665,  0.50059 ,
		 0.299   ,  0.587   ,  0.114
	};
	static FloatMatrix conversion = {conversion_values, 3, 3};
	apply_transformation_image(image, &conversion);
}
void convert_yCbCr_to_rgb(PixelFloatMatrix* image) {
	static float conversion_values[] =
	{
		 1.40252    ,  -0.0    ,  1.0     ,
   		-0.714401   ,  -0.34373,  0.999997,
   		-0.000012644,   1.76991,  1.00002
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
	PixelFloat result = {0};

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
			// TODO(erick): Blue should be Luminescence
			PixelFloat applied_x = apply_filter_at_selectively(input_image, &sobel_x, line, column, Blue);
			PixelFloat applied_y = apply_filter_at_selectively(input_image, &sobel_y, line, column, Blue);

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

uint mandelbrot_point_escape(complexf point, uint max_iterations, complexf* escape_point) {
	complexf z = {0.0f, 0.0f};
	uint current_iteration = 0;

	do {
		z = complexf_sum(complexf_square(z), point);
		current_iteration++;
	} while(complexf_norm_sq(z) <= 4 && current_iteration < max_iterations);

	*escape_point = z;
	return current_iteration;
}

void mandelbrot_simple_coloring(PixelFloat* pixel, uint escape_interation, uint max_iterations) {
	float hue_per_unit = (float) 360.0f / (float) max_iterations;

	float hue_value = 240.0f + hue_per_unit * escape_interation;
	if(hue_value > 360.0f) hue_value -= 360.0f;

	pixel->h = hue_value;
	pixel->s = 1.0f;
	pixel->b = 0.8f;
}

void mandelbrot_smooth_coloring(PixelFloat* pixel, uint escape_interation, uint max_iterations, complexf escape_point) {
	float smooth_value = escape_interation + 1 - logf(logf(complexf_norm(escape_point))) / logf(2);
	smooth_value /= max_iterations;
	smooth_value *= 360.0f;

	float hue_value = 240.0f + smooth_value;
	if(hue_value > 360.0f) hue_value -= 360.0f;

	pixel->h = hue_value;
	pixel->s = 0.7f;
	pixel->b = 0.8f;
}

void draw_mandelbrot(PixelFloatMatrix* image, float scale, complexf center, uint max_iterations) {
	int image_center_r = image->n_columns / 2;
	int image_center_i = image->n_lines / 2;

	for(int line = 0; line < image->n_lines; line++) {
		float line_in_complex = (float) (line - image_center_i) / scale + center.i;
		for (int column = 0; column < image->n_columns; column++) {
			complexf escape_point;
			float column_in_complex = (float) (column - image_center_r) / scale + center.r;

			complexf current_point = {column_in_complex, line_in_complex};
			uint escape_interation = mandelbrot_point_escape(current_point, max_iterations, &escape_point);
			if(escape_interation < max_iterations) {
				PixelFloat* pixel = &(get_matrix_value(*image, line, column));
				mandelbrot_simple_coloring(pixel, escape_interation, max_iterations);
				// mandelbrot_smooth_coloring(pixel, escape_interation, max_iterations, escape_point);
			}
		}
	}
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

void copy_pixel(PixelFloatMatrix* input, PixelFloatMatrix* output, uint line, uint column) {
	PixelFloat input_pixel = (get_matrix_value(*input, line, column));
	PixelFloat* output_pixel = &(get_matrix_value(*output, line, column));
	output_pixel->r = input_pixel.r;
	output_pixel->g = input_pixel.g;
	output_pixel->b = input_pixel.b;
}

void copy_line(PixelFloatMatrix* input, PixelFloatMatrix* output, uint line) {
	// NOTE(erick): input->n_columns must be smaller than output->n_columns
	//              it is asserted in the beginning of combine_images
	for(uint column = 0; column < input->n_columns; column++) {
		copy_pixel(input, output, line, column);
	}
}

void combine_images(PixelFloatMatrix* input_1, PixelFloatMatrix* input_2,
					PixelFloatMatrix* output, float input_1_percentage) {
	float input_2_percentage = 1.0f - input_1_percentage;
	uint max_lines = fmaxf(input_1->n_lines, input_2->n_lines);
	uint max_columns = fmaxf(input_1->n_columns, input_2->n_columns);

	assert(output->n_lines >= max_lines, "Output should have %d lines, but has only %d.", max_lines, output->n_lines);
	assert(output->n_columns >= max_columns, "Output should have %d columns, but has only %d.", max_columns, output->n_columns);

	for(uint line = 0; line < max_lines; line++) {
		//NOTE(erick): if input_1->n_lines is smaller than max_lines, than input_2->n_lines IS max_lines
		if(line > input_1->n_lines) copy_line(input_2, output, line);
		else if(line > input_2->n_lines) copy_line(input_1, output, line);
		else {
			for(uint column = 0; column < max_columns; column++) {
				//NOTE(erick): The argument above also apply to columns
				if(column > input_1->n_columns) copy_pixel(input_2, output, line, column);
				else if(column > input_2->n_columns) copy_pixel(input_1, output, line, column);
				else {
					PixelFloat pixel_1 = (get_matrix_value(*input_1, line, column));
					PixelFloat pixel_2 = (get_matrix_value(*input_2, line, column));
					PixelFloat* output_pixel = &(get_matrix_value(*output, line, column));
					output_pixel->r = pixel_1.r * input_1_percentage + pixel_2.r * input_2_percentage;
					output_pixel->g = pixel_1.g * input_1_percentage + pixel_2.g * input_2_percentage;
					output_pixel->b = pixel_1.b * input_1_percentage + pixel_2.b * input_2_percentage;
				}
			}
		}
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

	char* input_file_path_2 = "test.bmp";

	BitmapFileHeader file_header = {0};
	BitmapInfoHeader bmp_header = {0};
	byte* image = load_bitmap_from_path(input_file_path, &file_header, &bmp_header);

	BitmapFileHeader file_header_2 = {0};
	BitmapInfoHeader bmp_header_2 = {0};
	byte* image_2 = load_bitmap_from_path(input_file_path_2, &file_header_2, &bmp_header_2);

	float* image_float = (float*) calloc(bmp_header.image_size, sizeof(float));
	float* image_float_2 = (float*) calloc(bmp_header_2.image_size, sizeof(float));
	float* image_float_applied = (float*) calloc(bmp_header.image_size, sizeof(float));


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


	FloatMatrix filter = {0};
	filter.values = filter_values;
	filter.n_lines = 7;
	filter.n_columns = 7;

	PixelFloatMatrix input_image = {0};
	input_image.values = (PixelFloat*) image_float;
	input_image.n_lines = bmp_header.image_height;
	input_image.n_columns = bmp_header.image_width;

	PixelFloatMatrix input_image_2 = {0};
	input_image_2.values = (PixelFloat*) image_float_2;
	input_image_2.n_lines = bmp_header_2.image_height;
	input_image_2.n_columns = bmp_header_2.image_width;

	convert_image_to_float(image, &input_image, bmp_header.image_size);
	convert_image_to_float(image_2, &input_image_2, bmp_header_2.image_size);

	PixelFloatMatrix output_image = {0};
	output_image.values = (PixelFloat*) image_float_applied;
	output_image.n_lines = bmp_header.image_height;
	output_image.n_columns = bmp_header.image_width;

	normalize_filter(&filter);
	// apply_filter_to_image(&input_image, &output_image, &filter);
	// apply_filter_to_image(&output_image, &input_image, &filter);
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
	// apply_sobel_operator(&input_image_2, &output_image);
	// convert_yCbCr_to_rgb(&output_image);

	// output_image = input_image;

	// convert_rgb_to_yCbCr(&input_image_2);
	// convert_rgb_to_yCbCr(&input_image);
	// combine_images(&input_image, &input_image_2, &output_image, 0.3f);
	// yCbCr_image_to_black_and_white(&output_image);
	// convert_yCbCr_to_rgb(&output_image);

	output_image = input_image;
	// convert_rgb_to_hsb(&output_image);
	// convert_hsb_to_rgb(&output_image);

	// combine_images(&input_image, &input_image_2, &output_image, 0.45);

	convert_rgb_to_yCbCr(&output_image);
	yCbCr_image_to_black_and_white(&output_image);
	convert_yCbCr_to_rgb(&output_image);

	convert_image_from_float(&output_image, image, bmp_header.image_size);

	BitmapFileHeader file_header_to_write = {0};
	BitmapInfoHeader bmp_header_to_write = {0};

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

	printf("Image has %dx%dx%d %dbits and %x compression\n", bmp_header.image_width, bmp_header.image_height,
	 bmp_header.colors_important, bmp_header.bits_per_pixel, bmp_header.compression_type);

	write_bitmap_to_path(output_file_path, image, &file_header_to_write, &bmp_header_to_write);

    return 0;
}
	// float* image_float_3 = (float*) calloc(bmp_header.image_size, sizeof(float));
	// PixelFloatMatrix image_3 = {(PixelFloat*) image_float_3, input_image.n_lines, input_image.n_columns};

	// complexf center = {-1.772f, 0.0f};
	// draw_mandelbrot(&image_3, 10200.0f, center, 500);
	// convert_hsb_to_rgb(&image_3);

	// combine_images(&input_image, &image_3, &output_image, 0.4);
