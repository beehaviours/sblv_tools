#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <sblv.h>

int main( int argc, char** argv, char** envv ) {
	
	int i ;
	FILE* fhandle ;
	sblv_header header ;
	int pos ;

	if (argc < 2) {
		fprintf(stderr, "Missing argument.\n\n") ;
		fprintf(stderr, "Syntax: %s file1 [file2 ...]\n", argv[0] );
		return EXIT_FAILURE ;
	}

	for (i=1; i<argc;i++) {
		fhandle = fopen( argv[i], "r" ) ;
		if ( fhandle == NULL ) {
			fprintf( stderr, "Could not Open %s\n", argv[i] ) ;
			return EXIT_FAILURE ;
		}
		pos = fseek( fhandle, -sizeof(sblv_header), SEEK_END );
		if (pos != 0) {
			fprintf(stderr, "Seek Error for %s\n", argv[i] ) ;
			return EXIT_FAILURE ;
		}
		fread( &header, sizeof(sblv_header), 1, fhandle ) ;

		printf("File: %s\n",argv[i]);
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
	}
	return EXIT_SUCCESS ;
}

