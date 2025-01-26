// Original file: source/video.c
// Original author: oreo639
// https://github.com/oreo639/3ds-theoraplayer
// Licensed under the zlib license. See "COPYING" for full information.

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#if !defined(_LARGEFILE_SOURCE)
#define _LARGEFILE_SOURCE
#endif
#if !defined(_LARGEFILE64_SOURCE)
#define _LARGEFILE64_SOURCE
#endif
#if !defined(_FILE_OFFSET_BITS)
#define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

#include "th_video.hpp"

#define VIDEO_DEFAULT_BUFFER_SIZE 4096

int audiofd = -1;

static inline int oggGetData(THEORA_Context *ctx)
{
	errno = 0;
	if(!(ctx->io.read_func))
		return -1;

	if(ctx->datasource) {
		char *buffer = ogg_sync_buffer(&ctx->sync, VIDEO_DEFAULT_BUFFER_SIZE);
		long bytes = ctx->io.read_func(buffer, 1, VIDEO_DEFAULT_BUFFER_SIZE, ctx->datasource);

		if(bytes>0)
			ogg_sync_wrote(&ctx->sync, bytes);

		if(bytes==0 && errno)
			return -1;

		return bytes;
	} else
		return 0;
}

static inline void oggQueuePage(THEORA_Context *ctx)
{
	if (ctx->tpackets) {
		ogg_stream_pagein(&ctx->tstream, &ctx->page);
	}
	if (ctx->vpackets) {
		ogg_stream_pagein(&ctx->vstream, &ctx->page);
	}
}

static inline int oggGetNextPacket(THEORA_Context *ctx, ogg_stream_state *stream, ogg_packet *packet)
{
	while (ogg_stream_packetout(stream, packet) <= 0)
	{
		const int rc = oggGetData(ctx);
		if (rc <= 0) {
			ctx->eos = 1;
			return 0;
		}
		else {
			while (ogg_sync_pageout(&ctx->sync, &ctx->page) > 0) {
				oggQueuePage(ctx);
			}
		}
	}
	return 1;
}

// Taken from libtremor
// https://gitlab.xiph.org/xiph/tremor/-/blob/master/asm_arm.h#L108
#if defined(ARM_ASM_CLIP15)
static inline ogg_int32_t CLIP_TO_15(ogg_int32_t x) {
  int tmp;
  asm volatile("subs	%1, %0, #32768\n\t"
	       "movpl	%0, #0x7f00\n\t"
	       "orrpl	%0, %0, #0xff\n"
	       "adds	%1, %0, #32768\n\t"
	       "movmi	%0, #0x8000"
	       : "+r"(x),"=r"(tmp)
	       :
	       : "cc");
  return(x);
}
#else
static inline ogg_int32_t CLIP_TO_15(ogg_int32_t x)
{
    int ret = x;
    ret -= ((x<=32767)-1)&(x-32767);
    ret -= ((x>=-32768)-1)&(x+32768);
    return ret;
}
#endif

int THEORA_CallbackCreate(THEORA_Context* ctx, void* datasource, THEORA_callbacks io)
{
	if (!ctx || !datasource)
		return 1;

	ogg_packet packet;
	th_setup_info *tsetup = NULL;

	memset(ctx, 0, sizeof(THEORA_Context));
	ctx->timer_calibrate = -1;
	ctx->datasource = datasource;
	ctx->io = io;

	ogg_sync_init(&ctx->sync);
	vorbis_info_init(&ctx->vinfo);
	vorbis_comment_init(&ctx->vcomment);
	th_info_init(&ctx->tinfo);
	th_comment_init(&ctx->tcomment);

	bool stateflag = false;
	while (!stateflag) {
		int ret = oggGetData(ctx);
		if (ret == 0)
			break;

		while (ogg_sync_pageout(&ctx->sync, &ctx->page) > 0) {
			ogg_stream_state test;

			if (!ogg_page_bos(&ctx->page))
			{
				/* Not a header! */
				oggQueuePage(ctx);
				stateflag=1;
				break;
			}

			ogg_stream_init(&test, ogg_page_serialno(&ctx->page));
			ogg_stream_pagein(&test, &ctx->page);
			ogg_stream_packetout(&test, &packet);
		
			/* identify the codec: try theora */
			if(!ctx->tpackets && th_decode_headerin(&ctx->tinfo, &ctx->tcomment, &tsetup, &packet)>=0){
				memcpy(&ctx->tstream, &test, sizeof(test));
				ctx->tpackets = 1;
			}else if(!ctx->vpackets && vorbis_synthesis_headerin(&ctx->vinfo, &ctx->vcomment, &packet)>=0){
				/* it is vorbis */
				memcpy(&ctx->vstream, &test, sizeof(test));
				ctx->vpackets = 1;
			}else{
				/* whatever it is, we don't care about it */
				ogg_stream_clear(&test);
			}
		}
	}

	/* we're expecting more header packets.
	   There are 2 more theora and 2 more vorbis headers next. */
	while((ctx->tpackets && (ctx->tpackets<3)) || (ctx->vpackets && (ctx->vpackets<3))){
		/* look for further theora headers */
		while(ctx->tpackets && (ctx->tpackets < 3)) {
			if (ogg_stream_packetout(&ctx->tstream, &packet) != 1) {
				/* Get more data? */
				break;
			}
			if(!th_decode_headerin(&ctx->tinfo, &ctx->tcomment, &tsetup, &packet)){
				fprintf(stderr,"Error parsing Theora stream headers; "
				"corrupt stream?\n");
				return 1;
			}
			ctx->tpackets++;
		}

		/* look for more vorbis header packets */
		while(ctx->vpackets && (ctx->vpackets < 3)) {
			if (ogg_stream_packetout(&ctx->vstream, &packet) != 1) {
				/* Get more data? */
				break;
			}
			if(vorbis_synthesis_headerin(&ctx->vinfo, &ctx->vcomment, &packet)){
				fprintf(stderr,"Error parsing Vorbis stream headers; corrupt stream?\n");
				return 1;
			}
			ctx->vpackets++;
		}

		/* The header pages/packets will arrive before anything else we
		care about, or the stream is not obeying spec */

		if(ogg_sync_pageout(&ctx->sync, &ctx->page)>0) {
			oggQueuePage(ctx); /* demux into the appropriate stream */
		} else{
			if(oggGetData(ctx)==0) {
				fprintf(stderr,"End of file while searching for codec headers.\n");
				return 1;
			}
		}
	}

	/* Set up Theora stream */
	if (ctx->tpackets) {
		/* th_decode_alloc() docs say to check for
		 * insanely large frames yourself.
		 */
		if((ctx->tinfo.frame_width > 99999) || (ctx->tinfo.frame_height > 99999))
			return 3;

		/* The decoder, at last! */
		ctx->tdec = th_decode_alloc(&ctx->tinfo, tsetup);

		// Set library post processing level.
		th_decode_ctl(ctx->tdec, TH_DECCTL_GET_PPLEVEL_MAX, &ctx->pp_level_max, sizeof(ctx->pp_level_max));
		ctx->pp_level = ctx->pp_level_max;
		th_decode_ctl(ctx->tdec, TH_DECCTL_SET_PPLEVEL, &ctx->pp_level, sizeof(ctx->pp_level));
	}

	/* Setup info structure for external use */
	if (ctx->tpackets) {
		double fps = 0;

		if (ctx->tinfo.fps_denominator)
			fps = ((double) ctx->tinfo.fps_numerator) / ((double) ctx->tinfo.fps_denominator);
		else
			fps = ctx->tinfo.fps_numerator;

		ctx->videoinfo.width = ((ctx->tinfo.pic_x + ctx->tinfo.frame_width + 1) & ~1) - (ctx->tinfo.pic_x & ~1); // ctx->tinfo.pic_width;
		ctx->videoinfo.height = ((ctx->tinfo.pic_y + ctx->tinfo.frame_height + 1) & ~1) - (ctx->tinfo.pic_y & ~1); // ctx->tinfo.pic_height;
		ctx->videoinfo.fps = fps;
		ctx->videoinfo.fmt = ctx->tinfo.pixel_fmt;
		ctx->videoinfo.colorspace = ctx->tinfo.colorspace;
	}

	if (ctx->vpackets) {
		ctx->audioinfo.channels = ctx->vinfo.channels;
		ctx->audioinfo.rate = ctx->vinfo.rate;
	}

	/* Done with this now */
	if (tsetup)
		th_setup_free(tsetup);

	/* Set up Vorbis stream */
	if (ctx->vpackets) {
		ctx->vdsp_init = vorbis_synthesis_init(&ctx->vdsp, &ctx->vinfo) == 0;
		ctx->vblock_init = vorbis_block_init(&ctx->vdsp, &ctx->vblock) == 0;
	}


	return 0;
}

int THEORA_Create(THEORA_Context* ctx, const char* filepath)
{
	THEORA_callbacks io =
	{
		.read_func = (size_t (*) (void*, size_t, size_t, void*)) fread,
		.seek_func = (int (*) (void*, ogg_int64_t, int)) fseeko,
		.close_func = (int (*) (void*)) fclose,
		.tell_func = (long int (*) (void*)) ftell,
	};
	FILE *fp = fopen(filepath, "rb");
	setvbuf(fp, NULL, _IOFBF, 128*1024);
	return THEORA_CallbackCreate(ctx, fp, io);
}

void THEORA_Close(THEORA_Context *ctx)
{
	/* Theora Data */
	if (ctx->tdec)
		th_decode_free(ctx->tdec);

	/* Vorbis Data */
	if (ctx->vblock_init)
		vorbis_block_clear(&ctx->vblock);
	if (ctx->vdsp_init)
		vorbis_dsp_clear(&ctx->vdsp);

	/* Stream Data */
	if (ctx->tpackets)
		ogg_stream_clear(&ctx->tstream);
	if (ctx->vpackets)
		ogg_stream_clear(&ctx->vstream);

	/* Metadata */
	th_info_clear(&ctx->tinfo);
	th_comment_clear(&ctx->tcomment);
	vorbis_comment_clear(&ctx->vcomment);
	vorbis_info_clear(&ctx->vinfo);

	/* Current State */
	ogg_sync_clear(&ctx->sync);

	/* I/O Data */
	if (ctx->io.close_func)
		ctx->io.close_func(ctx->datasource);
}

bool THEORA_HasVideo(THEORA_Context *ctx)
{
	return ctx->tpackets;
}

bool THEORA_HasAudio(THEORA_Context *ctx)
{
	return ctx->vpackets;
}

THEORA_videoinfo* THEORA_vidinfo(THEORA_Context *ctx)
{
	return &ctx->videoinfo;
}

THEORA_audioinfo* THEORA_audinfo(THEORA_Context *ctx)
{
	return &ctx->audioinfo;
}

int THEORA_eos(THEORA_Context *ctx)
{
	return ctx->eos;
}

void THEORA_reset(THEORA_Context *ctx)
{
	if (ctx->tpackets)
		ogg_stream_reset(&ctx->tstream);

	if (ctx->vpackets)
		ogg_stream_reset(&ctx->vstream);

	ogg_sync_reset(&ctx->sync);
	ctx->io.seek_func(ctx->datasource, 0, SEEK_SET);
	ctx->eos = 0;
	ctx->frames = 0;
	ctx->dropped = 0;
}

/* get relative time since beginning playback, compensating for A/V
   drift */
static double get_time(THEORA_Context *ctx)
{
	static ogg_int64_t last = 0;
	static ogg_int64_t up = 0;
	ogg_int64_t now;
	struct timeval tv;

	gettimeofday(&tv,0);
	now = tv.tv_sec*1000+tv.tv_usec/1000;

	if(ctx->timer_calibrate == -1)
		ctx->timer_calibrate = last = now;

	/* We can just use the system clock as a timer. */
	/* only one complication: If the process is suspended, we should
		reset timing to account for the gap in play time.  Do it the
		easy/hack way */
	if(now-last > 1000)
		ctx->timer_calibrate += (now-last);

	last=now;

	if(now-up > 200) {
		double timebase = (now-ctx->timer_calibrate)*.001;
		int hundredths  = timebase*100-(long)timebase*100;
		int seconds     = (long)timebase%60;
		int minutes     = ((long)timebase/60)%60;
		int hours       = (long)timebase/3600;

		fprintf(stderr,"   Playing: %d:%02d:%02d.%02d\n",
			hours,minutes,seconds,hundredths);
		up=now;
	}

	return (now-ctx->timer_calibrate)*.001;
}

int THEORAi_readvideo(THEORA_Context *ctx)
{
	//static int droppedinrow = 0;
	ogg_int64_t granulepos = 0;
	ogg_packet packet;
	int retval = 0;
	int rc;

	// Keep trying to get a usable packet
	if (!oggGetNextPacket(ctx, &ctx->tstream, &packet)) {
		// ... unless there's nothing left for us to read.
		return 0;
	}

	if(ctx->pp_inc) {
		ctx->pp_level += ctx->pp_inc;
		th_decode_ctl(ctx->tdec, TH_DECCTL_SET_PPLEVEL, &ctx->pp_level, sizeof(ctx->pp_level));
		ctx->pp_inc=0;
	}

	/*HACK: This should be set after a seek or a gap, but we might not have
	 a granulepos for the first packet (we only have them for the last
	 packet on a page), so we just set it as often as we get it.
	To do this right, we should back-track from the last packet on the
	 page and compute the correct granulepos for the first packet after
	 a seek or a gap.*/
	if(packet.granulepos>=0) {
		th_decode_ctl(ctx->tdec,TH_DECCTL_SET_GRANPOS,&packet.granulepos, sizeof(packet.granulepos));
	}
	if((rc = th_decode_packetin(ctx->tdec, &packet, &granulepos)) == 0) {
		ctx->videobuf_time=th_granule_time(ctx->tdec, granulepos);
		ctx->frames++;

		/*{
			double timebase = (ctx->videobuf_time);
			int hundredths  = timebase*100-(long)timebase*100;
			int seconds     = (long)timebase%60;
			int minutes     = ((long)timebase/60)%60;
			int hours       = (long)timebase/3600;

			fprintf(stderr,"   Granule time: %d:%02d:%02d.%02d\n",
				hours,minutes,seconds,hundredths);
		}*/

		/* is it already too old to be useful?	This is only actually
		 useful cosmetically after a SIGSTOP.	Note that we have to
		 decode the frame even if we don't show it (for now) due to
		 keyframing.	Soon enough libtheora will be able to deal
		 with non-keyframe seeks.	*/
		if(ctx->videobuf_time<get_time(ctx)) {
			/*If we are too slow, reduce the pp level.*/
			ctx->pp_inc=ctx->pp_level>0?-1:0;
			ctx->dropped++;
		} else {
			retval = 1;
		}
	}
	else if (rc != TH_DUPFRAME)
	{
		retval = 0;
	}

	return retval;
}

int THEORAi_decodevideo(THEORA_Context *ctx, th_ycbcr_buffer ybr) {
	double tdiff;
	tdiff=ctx->videobuf_time-get_time(ctx);

	//If we have lots of extra time, increase the post-processing level.
	if(tdiff>ctx->tinfo.fps_denominator*0.25/ctx->tinfo.fps_numerator) {
		ctx->pp_inc=ctx->pp_level<ctx->pp_level_max?1:0;
	}
	else if(tdiff<ctx->tinfo.fps_denominator*0.05/ctx->tinfo.fps_numerator) {
		ctx->pp_inc=ctx->pp_level>0?-1:0;
	}

	if (ctx->videobuf_time<=get_time(ctx)) {
		if (th_decode_ycbcr_out(ctx->tdec, ybr) != 0)
			return -1; // Uhh?!
		return 1;
	}

	return 0;
}

int THEORA_getvideo(THEORA_Context *ctx, th_ycbcr_buffer ybr) {
	if (ctx->vstate == 0)
		if (THEORAi_readvideo(ctx))
			ctx->vstate = 1;

	if (ctx->vstate == 1) {
		int ret = THEORAi_decodevideo(ctx, ybr);

		if (ret != 0)
			ctx->vstate = 0;

		return !!ret;
	}

	return 0;
}

static long ov_read(THEORA_Context *ctx, char *buffer, int bytes_req, int *bitstream) {
	int i,j;

	ogg_int32_t **pcm;
	long samples;

	if(!ctx->vpackets)
		return -1;

	while(1) {
		samples=vorbis_synthesis_pcmout(&ctx->vdsp, &pcm);
		if(samples)break;

		/* suck in another packet */
		{
			ogg_packet packet;
			if (!oggGetNextPacket(ctx, &ctx->vstream, &packet)) {
				/* ... unless there's nothing left for us to read. */
				return 0;
			}
			if (vorbis_synthesis(&ctx->vblock, &packet) == 0) {
				vorbis_synthesis_blockin(&ctx->vdsp, &ctx->vblock);
			}
		}

	}

	if(samples>0){

		/* yay! proceed to pack data into the byte buffer */
		long channels=ctx->vinfo.channels;

		if(samples>(bytes_req/(2*channels)))
			samples=bytes_req/(2*channels);

		/* It's faster in this order */
		for(i=0;i<channels;i++) {
			ogg_int32_t *src=pcm[i];
			short *dest=((short *)buffer)+i;
			for(j=0;j<samples;j++) {
				*dest=CLIP_TO_15(src[j]>>9);
				dest+=channels;
			}
		}

		vorbis_synthesis_read(&ctx->vdsp, samples);
		//vf->pcm_offset+=samples;
		//if(bitstream)*bitstream=vf->current_link;
		return(samples*2*channels);
	}else{
		return(samples);
	}
}

int THEORA_readaudio(THEORA_Context *ctx, char *bufferOut, int buffSize)
{
	uint64_t samplesRead = 0;
	int samplesToRead = buffSize;

	while(samplesToRead > 0)
	{
		//static int current_section;
		int samplesJustRead =
			ov_read(ctx, bufferOut,
					samplesToRead > 4096 ? 4096	: samplesToRead,
					NULL);

		if(samplesJustRead < 0)
			return samplesJustRead;
		else if(samplesJustRead == 0)
		{
			/* End of file reached. */
			break;
		}

		samplesRead += samplesJustRead;
		samplesToRead -= samplesJustRead;
		bufferOut += samplesJustRead;
	}

	return samplesRead / sizeof(int16_t);
}
