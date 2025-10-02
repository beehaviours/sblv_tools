#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <png.h>
#include <sblv.h>

int save_png(const char* filename, unsigned char* image_data, int width, int height) {
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep row_pointer;

    fp = fopen(filename, "wb");
    if (!fp) {
        return -1;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        return -1;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        return -1;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return -1;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr, width, height, 8, 
			PNG_COLOR_TYPE_GRAY, 
			PNG_INTERLACE_NONE, 
			PNG_COMPRESSION_TYPE_DEFAULT, 
			PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    row_pointer = image_data;

    for (int y = 0; y < height; y++) {
        png_write_row(png_ptr, row_pointer);
        row_pointer += width;
    }

    png_write_end(png_ptr, NULL);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    return 0;
}



int main( int argc, char** argv, char** envv ) {
	
	FILE* fhandle ;
	sblv_header header ;
	int pos ;

	if (argc != 3) {
		fprintf(stderr, "Missing argument.\n\n") ;
		fprintf(stderr, "Syntax: %s input.sblv output.png\n", argv[0] );
		return EXIT_FAILURE ;
	}

	fhandle = fopen( argv[1], "r" ) ;
	if ( fhandle == NULL ) {
		fprintf( stderr, "Could not Open %s\n", argv[1] ) ;
		return EXIT_FAILURE ;
	}
	pos = fseek( fhandle, -sizeof(sblv_header), SEEK_END );
	if (pos != 0) {
		fprintf(stderr, "Seek Error for %s\n", argv[1] ) ;
		return EXIT_FAILURE ;
	}
	fread( &header, sizeof(sblv_header), 1, fhandle ) ;

	printf("File: %s\n",argv[1]);
	printf("Sensor serial: %s\n", header.cam_serial );
	printf("Rows: %d\n", header.rows );
	printf("Cols: %d\n", header.cols );
	printf("Fps: %.2f\n", header.fps ) ;
	printf("Encoding: %d\n", header.encoding ) ; // TODO
	printf("Hive: %d\n", header.hive ) ;
	printf("Module: %d\n", header.module ) ;
	printf("Cam: %d\n", header.cam ) ;
	
	struct tm tm = *localtime(&(header.timestamp));
	printf( "Timestamp: %02d.%02d.%d %02d:%02d:%02d\n", 
				tm.tm_mday, 
				tm.tm_mon + 1, 
				tm.tm_year + 1900, 
				tm.tm_hour, 
				tm.tm_min, 
				tm.tm_sec);

	fseek( fhandle, 0, SEEK_SET ) ;

	unsigned char* img = malloc( header.cols*header.rows ) ;
	if ( !img ) {
		fprintf(stderr, "malloc error\n" ) ;
		return 0 ;
	}

	fread( img, header.cols * header.rows, 1, fhandle ) ;

	save_png( argv[2], img, header.cols, header.rows ) ;

	free(img) ;

	fclose(fhandle) ;


	return EXIT_SUCCESS ;
}

