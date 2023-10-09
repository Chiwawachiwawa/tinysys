#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "core.h"
#include "apu.h"
#include "gpu.h"
#include "dma.h"
#include "leds.h"

#include "micromod/micromod.h"

static struct EVideoContext s_vx;
static struct EVideoSwapContext s_sc;
static uint8_t *s_framebufferA;
static uint8_t *s_framebufferB;
static uint8_t *s_rasterBuffer;

#define SAMPLING_FREQ  44100	// 44.1khz
#define REVERB_BUF_LEN 4110		// 100ms
#define OVERSAMPLE     2		// 2x oversampling
#define NUM_CHANNELS   2		// Stereo
#define BUFFER_SAMPLES 1024		// buffer size (max: 2048 bytes i.e. 1024 words)

static short mix_buffer[ BUFFER_SAMPLES * NUM_CHANNELS * OVERSAMPLE ];
static short reverb_buffer[ REVERB_BUF_LEN ];
static short *apubuffer;
static long reverb_len = REVERB_BUF_LEN, reverb_idx = 0, filt_l = 0, filt_r = 0;
static long samples_remaining = 0;

/*
	2:1 downsampling with simple but effective anti-aliasing.
	Count is the number of stereo samples to process, and must be even.
	input may point to the same buffer as output.
*/
static void downsample( short *input, short *output, long count ) {
	long in_idx, out_idx, out_l, out_r;
	in_idx = out_idx = 0;
	while( out_idx < count ) {	
		out_l = filt_l + ( input[ in_idx++ ] >> 1 );
		out_r = filt_r + ( input[ in_idx++ ] >> 1 );
		filt_l = input[ in_idx++ ] >> 2;
		filt_r = input[ in_idx++ ] >> 2;
		output[ out_idx++ ] = out_l + filt_l;
		output[ out_idx++ ] = out_r + filt_r;
	}
}

/* Simple stereo cross delay with feedback. */
static void reverb( short *buffer, long count ) {
	long buffer_idx, buffer_end;
	if( reverb_len > 2 ) {
		buffer_idx = 0;
		buffer_end = buffer_idx + ( count << 1 );
		while( buffer_idx < buffer_end ) {
			buffer[ buffer_idx ] = ( buffer[ buffer_idx ] * 3 + reverb_buffer[ reverb_idx + 1 ] ) >> 2;
			buffer[ buffer_idx + 1 ] = ( buffer[ buffer_idx + 1 ] * 3 + reverb_buffer[ reverb_idx ] ) >> 2;
			reverb_buffer[ reverb_idx ] = buffer[ buffer_idx ];
			reverb_buffer[ reverb_idx + 1 ] = buffer[ buffer_idx + 1 ];
			reverb_idx += 2;
			if( reverb_idx >= reverb_len ) {
				reverb_idx = 0;
			}
			buffer_idx += 2;
		}
	}
}

/* Reduce stereo-separation of count samples. */
static void crossfeed( short *audio, int count ) {
	int l, r, offset = 0, end = count << 1;
	while( offset < end ) {
		l = audio[ offset ];
		r = audio[ offset + 1 ];
		audio[ offset++ ] = ( l + l + l + r ) >> 2;
		audio[ offset++ ] = ( r + r + r + l ) >> 2;
	}
}

static long read_file( const char *filename, void *buffer, long length ) {
	FILE *file;
	long count;
	count = -1;
	file = fopen( filename, "rb" );
	if( file != NULL ) {
		count = fread( buffer, 1, length, file );
		if( count < length && !feof( file ) ) {
			printf("Unable to read file '%s'.\n", filename );
			count = -1;
		}
		if( fclose( file ) != 0 ) {
			printf("Unable to close file '%s'.\n", filename );
		}
	}
  else
    printf("Unable to find file '%s'.\n", filename );
	return count;
}

static void print_module_info() {
	int inst;
	char string[ 23 ];
	for( inst = 0; inst < 16; inst++ ) {
		micromod_get_string( inst, string );
		printf( "%02i - %-22s ", inst, string );
		micromod_get_string( inst + 16, string );
		printf( "%02i - %-22s\n", inst + 16, string );
	}
}

static long read_module_length( const char *filename ) {
	long length;
	signed char header[ 1084 ];
	length = read_file( filename, header, 1084 );
	if( length == 1084 ) {
		length = micromod_calculate_mod_file_len( header );
		if( length < 0 ) {
			printf("Module file type not recognised.\n");
		}
	} else {
		printf("Unable to read module file '%s'.\n", filename );
		length = -1;
	}
	return length;
}

void DrawWaveform()
{
	int16_t *src = (int16_t *)apubuffer;

	RPUSetColor(0x00); // Black
	{
		// Clear raster surface
		// TODO: Fix precision so we can do just one triangle here
		SPrimitive prim;
		prim.x0 = 0;
		prim.y0 = 0;
		prim.x1 = 319;
		prim.y1 = 239;
		prim.x2 = 319;
		prim.y2 = 0;
		RPUPushPrimitive(&prim);
		RPURasterizePrimitive();

		prim.x0 = 319;
		prim.y0 = 239;
		prim.x1 = 0;
		prim.y1 = 0;
		prim.x2 = 0;
		prim.y2 = 239;
		RPUPushPrimitive(&prim);
		RPURasterizePrimitive();
	}

	RPUSetColor(0x37); // Blue - Left
	for (uint32_t x=0; x<320/*BUFFER_SAMPLES/2*/; ++x)
	{
		int16_t L = src[x*2+0]/256;

		SPrimitive prim;
		prim.x0 = x+1;
		prim.y0 = 120+L;
		prim.x1 = x;
		prim.y1 = 120;
		prim.x2 = x+2;
		prim.y2 = 120;

		RPUPushPrimitive(&prim);
		RPURasterizePrimitive();
	}

	RPUSetColor(0x28); // Red - Right
	for (uint32_t x=0; x<320/*BUFFER_SAMPLES/2*/; ++x)
	{
		int16_t R = src[x*2+1]/256;

		SPrimitive prim;
		prim.x0 = x+1;
		prim.y0 = 120+R;
		prim.x1 = x+2;
		prim.y1 = 120;
		prim.x2 = x;
		prim.y2 = 120;

		RPUPushPrimitive(&prim);
		RPURasterizePrimitive();
	}

	RPUFlushCache();
	RPUInvalidateCache();
	RPUBarrier();
	RPUWait();
	DMAResolveTiles((uint32_t)s_rasterBuffer, (uint32_t)s_sc.writepage);

	GPUSwapPages(&s_vx, &s_sc);
}

static long play_module( signed char *module )
{
	long result;

	result = micromod_initialise( module, SAMPLING_FREQ * OVERSAMPLE );
	if( result == 0 )
	{
		print_module_info();
		samples_remaining = micromod_calculate_song_duration();
		printf( "Song Duration: %li seconds.\n", samples_remaining / ( SAMPLING_FREQ * OVERSAMPLE ) );
		fflush( NULL );

		// Set up buffer size for all future transfers
		APUSetBufferSize(BUFFER_SAMPLES);
		APUSetSampleRate(ASR_44_100_Hz);
		uint32_t prevframe = APUFrame();

		int playing = 1;
		while( playing )
		{
			int count = BUFFER_SAMPLES * OVERSAMPLE;
			if( count > samples_remaining )
				count = samples_remaining;

			// Anything above 19ms stalls this
			__builtin_memset( mix_buffer, 0, BUFFER_SAMPLES * NUM_CHANNELS * OVERSAMPLE * sizeof( short ) );
			micromod_get_audio( mix_buffer, count );
			downsample( mix_buffer, apubuffer, BUFFER_SAMPLES * OVERSAMPLE );
			crossfeed( apubuffer, BUFFER_SAMPLES );
			reverb( apubuffer, BUFFER_SAMPLES );

			samples_remaining -= count;

			// NOTE: Buffer size is in multiples of 32bit words
			// h/w loops over (wordcount/4)-1 sample addresses and reads blocks of 16 bytes each time
			// There are two audio pages; every call to APUSwapBuffers() switches the read/write pages

			// Make sure the writes are visible by the DMA
			CFLUSH_D_L1;

			// Fill current write buffer with new mix data
			APUStartDMA((uint32_t)apubuffer);

			// Wait for the APU to finish playing back current read buffer
			// Meanwhile the playback buffer will still be going without interruptions
			uint32_t currframe;
			do
			{
				// Still working on same frame?
				currframe = APUFrame();
			} while (currframe == prevframe);

			// Read buffer drained, swap to new read buffer with no gap
			// NOTE: We probably need to signal early, long enough for
			// the CPU to read, send the swap, and catch things in time for gapless playback
			APUSwapBuffers();

			// Remember this frame
			prevframe = currframe;

			if( samples_remaining <= 0 || result != 0 )
				playing = 0;

			DrawWaveform();
		}
	}
	else
		printf("micromod_initialise failed\n");

	return result;
}

void PlayMODFile(const char *fname)
{
	signed char *module;
	long count, length;

	/* Read module file.*/
	length = read_module_length( fname );
	if( length > 0 )
	{
		printf( "Module Data Length: %li bytes.\n", length );
		module = (signed char*)calloc( length, 1 );
		if( module != NULL )
		{
			count = read_file( fname, module, length );
			if( count < length )
				printf("Module file is truncated. %li bytes missing.\n", length - count );
			play_module( module );
			free( module );
		}
	}
}

int main(int argc, char *argv[])
{
	s_framebufferB = GPUAllocateBuffer(320*240);
	s_framebufferA = GPUAllocateBuffer(320*240);
	s_rasterBuffer = GPUAllocateBuffer(80*60*16); // Tile buffer
	memset(s_rasterBuffer, 0, 80*60*16);
	memset(s_framebufferA, 0, 320*240);
	memset(s_framebufferB, 0, 320*240);
	s_vx.m_cmode = ECM_8bit_Indexed;
	s_vx.m_vmode = EVM_320_Wide;
	GPUSetVMode(&s_vx, EVS_Enable);
	GPUSetDefaultPalette(&s_vx);
	RPUSetTileBuffer((uint32_t)s_rasterBuffer);

	s_sc.cycle = 0;
	s_sc.framebufferA = s_framebufferA;
	s_sc.framebufferB = s_framebufferB;
	GPUSwapPages(&s_vx, &s_sc);

	CFLUSH_D_L1;

	apubuffer = (short*)APUAllocateBuffer(BUFFER_SAMPLES*NUM_CHANNELS*sizeof(short));
	printf("\nAPU mix buffer: 0x%.8x\n", (unsigned int)apubuffer);

	char currpath[30] = "sd:";
	if (getcwd(currpath, 30))
		printf("Working directory:%s\n", currpath);
	else
	{
		strcpy(currpath, "sd:/");
		printf("Using default working directory\n");
	}

	if (argc<=1)
	{
		printf("Loading and playing module test.mod\n");
		PlayMODFile("sd:/test.mod");
	}
	else
	{
		char fullpath[128];
		strcpy(fullpath, currpath);
		strcat(fullpath, argv[1]);
		printf("Loading and playing module %s\n", fullpath);
		PlayMODFile(fullpath);
	}

	printf("Playback complete\n");

	// Stop audio output
	APUStop();
	APUSwapBuffers();
	APUStop();
	APUSwapBuffers();

	return 0;
}
