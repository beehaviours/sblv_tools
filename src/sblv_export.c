#include <stdio.h>
#include <time.h>

#include <sbl.h>
#include <sblv.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>

int main( int argc, char** argv ) {

	FILE* input_file = NULL ;
	const char* output_file = argv[2] ;
	sblv_header header ;
	int ret ;
	long data_size ;

	if ( argc != 3 ) {
		fprintf(stderr,"Wrong number of arguments.\n\n");
		fprintf(stderr,"Usage: %s <file.sblv> <output.mp4>\n", argv[0]);
		return -1 ;
	}

	input_file = fopen(argv[1], "r");
	if ( input_file == NULL ) {
		fprintf(stderr, "Could not open %s\n", argv[1]) ;
		return -1 ;
	}

	// ********************************
	// Lecture du header
	// ********************************

	ret = fseek( input_file, -sizeof(sblv_header), SEEK_END ) ;
	if ( ret != 0 ) {
		fprintf( stderr, "Seek Error (SEEK_END)\n" ) ;
		return -1 ;
	}

	data_size = ftell( input_file ) ;
	fread( &header, sizeof(sblv_header), 1, input_file ) ;

	printf("Rows: %d\n", header.rows );
	printf("Cols: %d\n", header.cols );
	printf("Fps: %.2f\n", header.fps ) ;
	printf("Encoding: %d\n", header.encoding ) ; 

	ret = fseek( input_file, 0L, SEEK_SET ) ;

	// ********************************
	// Checks
	// ********************************
	
	if ( header.encoding != 0 ) {
		fprintf(stderr, "Encoding %d is not supported (yet).\n", header.encoding ) ;
		return -1 ;
	}

	if ( (data_size % (header.rows * header.cols)) != 0 ) {
		fprintf( stderr, " [Warning] Short Read expected.\n") ;
	} else {
		fprintf( stdout, "%ld frames will be converted\n", 
				data_size / (header.rows * header.cols) ) ;
	}

	// ********************************
	// Encoding
	// ********************************
	
	AVFormatContext *fmt_ctx = NULL;
    AVStream *video_st = NULL;
    AVCodecContext *c = NULL;
    const AVCodec *codec;
    AVFrame *frame;
    AVPacket *pkt;
	int frame_index = 0;

	// Allocation du contexte de sortie (MP4)
    avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, output_file);
    if (!fmt_ctx) {
        fprintf(stderr, "Impossible de créer le contexte de sortie\n");
        return -1;
    }

    // Sélection du codec H.264
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "Codec H.264 non trouvé\n");
        return -1;
    }

	// Ajout d’un flux vidéo dans le conteneur
    video_st = avformat_new_stream(fmt_ctx, NULL);
    if (!video_st) {
        fprintf(stderr, "Impossible de créer le flux vidéo\n");
        return -1;
    }
    video_st->id = fmt_ctx->nb_streams - 1;

    // Création du contexte codec
    c = avcodec_alloc_context3(codec);
    if (!c) return -1;

	c->codec_id = codec->id;
    c->width = header.cols;
    c->height = header.rows;
    c->time_base = (AVRational){1, header.fps};
    c->framerate = (AVRational){header.fps, 1};
    c->gop_size = 12;
    c->max_b_frames = 2;
    c->pix_fmt = AV_PIX_FMT_GRAY8;

    if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    av_opt_set(c->priv_data, "preset", "veryslow", 0);
	av_opt_set(c->priv_data, "crf", "18", 0);

    // Ouvre le codec
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Erreur ouverture codec\n");
        return -1;
    }

	ret = avcodec_parameters_from_context(video_st->codecpar, c);
    if (ret < 0) return -1;

    // Ouvre le fichier de sortie
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmt_ctx->pb, output_file, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Impossible d’ouvrir le fichier de sortie\n");
            return -1;
        }
    }

    // Écrit l’entête du conteneur MP4
    ret = avformat_write_header(fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Erreur écriture entête MP4\n");
        return -1;
    }

    pkt = av_packet_alloc();
    frame = av_frame_alloc();
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;
    av_frame_get_buffer(frame, 32);

    int frame_size = header.rows*header.cols;
    uint8_t *buffer = (uint8_t*)malloc(frame_size);

    while (fread(buffer, 1, frame_size, input_file) == frame_size) {
        ret = av_frame_make_writable(frame);
        if (ret < 0) break;

        for (int y = 0; y < header.rows; y++) {
            memcpy(frame->data[0] + y * frame->linesize[0],
                   buffer + y * header.cols, header.cols);
        }

        frame->pts = frame_index++;

        // Envoi au codec
        ret = avcodec_send_frame(c, frame);
        if (ret < 0) break;

        // Récupération des paquets encodés
        while (ret >= 0) {
            ret = avcodec_receive_packet(c, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            else if (ret < 0) break;

            pkt->stream_index = video_st->index;
            av_packet_rescale_ts(pkt, c->time_base, video_st->time_base);
            av_interleaved_write_frame(fmt_ctx, pkt);
            av_packet_unref(pkt);
        }
    }

    // Flush du codec
    avcodec_send_frame(c, NULL);
    while (avcodec_receive_packet(c, pkt) == 0) {
        pkt->stream_index = video_st->index;
        av_packet_rescale_ts(pkt, c->time_base, video_st->time_base);
        av_interleaved_write_frame(fmt_ctx, pkt);
        av_packet_unref(pkt);
    }

    av_write_trailer(fmt_ctx);

    // Nettoyage
    free(buffer);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&c);
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&fmt_ctx->pb);
    avformat_free_context(fmt_ctx);


	// ********************************
	// Fermeture des fichiers
	// ********************************
	
	fclose( input_file ) ;

	return 0 ;
}
