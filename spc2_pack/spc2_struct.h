

#include "types.h"

PACK( typedef struct {
	u8	magic[4];
	u8	eof_char;
	u8	rev_major;
	u8	rev_minor;
	u16	spc_count;
	u8	reserved[7];
} spc2_header );


PACK( typedef struct {
	u16	block_offset[256];
	u8	dsp_regs[128];
	u8	ipl_rom[64];
	u8	cpu_pcl;
	u8	cpu_pch;
	u8	cpu_a;
	u8	cpu_x;
	u8	cpu_y;
	u8	cpu_psw;
	u8	cpu_sp;
	u8	chan_enable;
	u32	date;
	u32	song_length_16th;
	u32	fade_length_16th;
	u32	amp_value;
	u8	emulator;
	u8	ost_disk;
	u16	ost_track;
	u16	year_binary;
	u8	padding[34];
	u8	song_name[32];
	u8	game_name[32];
	u8	artist_name[32];
	u8	dumper_name[32];
	u8	comments[32];
	u8	official_title[32];
	u8	publisher_name[32];
	u8	spc_filename[28];
	u32	ptr_extended;
} spc2_metadata );

PACK( typedef struct {
	u8 type;
	u8 length;
	u8 data[255];
} spc2_chunk );
