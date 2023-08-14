#pragma once
#include "types.h"

#pragma pack(push)
#pragma pack(1)

struct spc_header
{
	u8	header[33];
	u8	pad_26[2];
	u8	id3_tag_present;
	u8	version_minor;
};

struct spc_cpu_regs
{
	u8	cpu_pcl;
	u8	cpu_pch;
	u8	cpu_a;
	u8	cpu_x;
	u8	cpu_y;
	u8	cpu_psw;
	u8	cpu_sp;
	u16	reserved;
};

struct spc_id666_text
{
	u8	song_title[32];
	u8	game_title[32];
	u8	dumper_name[16];
	u8	comments[32];
	u8	date_dumped[11];
	u8	song_length_secs[3];
	u8	fade_length_ms[5];
	u8	song_artist[32];
	u8	channel_disable;
	u8	emulator;
	u8	reserved[45];
};

struct spc_id666_bin
{
	u8	song_title[32];
	u8	game_title[32];
	u8	dumper_name[16];
	u8	comments[32];
	u32	date_dumped;
	u8	unused[7];
	u8	song_length_secs[3];
	u8	fade_length_ms[4];
	u8	song_artist[32];
	u8	channel_disable;
	u8	emulator;
	u8	reserved[46];
};

struct spc_ram_store
{
	u8	ram[64 * 1024];
	u8	dsp_regs[128];
	u8	unused[64];
	u8	extra_ram[64];
};

struct spc_idx6_table
{
	u8	song_title[256];
	u8	game_title[256];
	u8	song_artist[256];
	u8	dumper_name[256];
	u8	date_dumped[4];
	u8	emulator;
	u8	comments[256];
	u8	ost_title[256];
	u8	ost_disc;
	u16	ost_track;
	u8	pub_name[256];
	u16	copyright;
	u32	intro_len;
	u32	loop_len;
	s32	end_len;
	u32	fade_len;
	u8	muted_channels;
	u8	loop_count;
	u32	amp_val;
};

struct spc_idx6_header
{
	u8 header[4];
	u32 size;
	u8 data[65536];
};

struct spc_idx6_sub_header
{
	u8 ID;
	u8 Type;
	u16 Length;
	union {
		u8 data[4];
		u32 val;
	};
};

struct spc_struct
{
	spc_header		header;		// 37 bytes
	spc_cpu_regs	cpu_regs;	// 9 bytes
	union { 
		spc_id666_text	tag_text; 
		spc_id666_bin	tag_binary;
	}; // 210 bytes
	// 256 bytes so far

	spc_ram_store	ram_dumps;	// 65536 bytes

	spc_idx6_table	extended;

	// the following variables aren't part of the file, 
	// but are helpful in storing information about it
	u8				tag_format;
	u32				date;	// normalized for SPC2		
	u32				song_length;
	u32				fade_length;
};

#pragma pack(pop)

#define	SPC_TAG_TEXT	1
#define SPC_TAG_BINARY	2
#define SPC_TAG_PREFER_BINARY 3

enum {
	IDX6_SONGNAME = 0x1,
	IDX6_GAMENAME,
	IDX6_ARTISTNAME,
	IDX6_DUMPERNAME,
	IDX6_DATEDUMPED,
	IDX6_EMULATOR,
	IDX6_COMMENTS,
	IDX6_OSTTITLE = 0x10,
	IDX6_OSTDISC,
	IDX6_OSTTRACK,
	IDX6_PUBNAME,
	IDX6_COPYRIGHT,
	IDX6_INTROLEN = 0x30,
	IDX6_LOOPLEN,
	IDX6_ENDLEN,
	IDX6_FADELEN,
	IDX6_MUTECHAN,
	IDX6_LOOPNUM,
	IDX6_AMPVAL
};
