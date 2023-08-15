
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "spc_struct.h"
#include "spc2_struct.h"
#include "spc2_write.h"
#include "sha1.h"

FILE	*fp;
void	*heap = NULL;
void	*heap_ptr;
void	*collection = NULL;
void	*coll_ptr;
void	*ext_tags = NULL;
void	*ext_tags_ptr;
u8		hash_results[65536][20];
SHA1Context sha1;
u32		spc_ext_tags_table[1024];
u32		num_tag_blocks;
u32		num_blocks;

static u8 magic[] = {'K','S','P','C', 0x1a};

int spc2_start()
{
	num_blocks = 0;
	num_tag_blocks = 0;

    memset(&hash_results, 0, sizeof(hash_results));
    memset(&spc_ext_tags_table, 0, sizeof(spc_ext_tags_table));

	// eat 1mb memory.
	heap = heap_ptr = malloc(64 * 1024 * 1024);
	// nom nom
	collection = coll_ptr = malloc(16 * 1024 * 1024);
	// nom nom
	ext_tags = ext_tags_ptr = malloc(16 * 1024 * 1024);

	// null ptr
	if(!heap || !collection || !ext_tags) return -1;

	return 0;
}

int spc2_finish(int *final_size, char *filename, u16 num_spc)
{
	int i=0;

	if(!heap || !collection || !ext_tags) return -1;

	fp = fopen(filename, "wb");
	if(fp == NULL){
		// fail
		return -1;
	}
	spc2_write_header(num_spc);

	// flush everything to file
	for(i=0;i<num_spc;i++)
	{
		if(((spc2_metadata*)heap)[i].ptr_extended == 0xFFFFFFFF)
		{
			((spc2_metadata*)heap)[i].ptr_extended = 0;
			continue;
		}
		((spc2_metadata*)heap)[i].ptr_extended += (u8*)heap_ptr-(u8*)heap + (u8*)coll_ptr-(u8*)collection + 16;
		//Must finalize the extended tag info offsets.
	}

	fwrite(heap, (u8*)heap_ptr-(u8*)heap, 1, fp);
	fwrite(collection, (u8*)coll_ptr-(u8*)collection, 1, fp);
	fwrite(ext_tags, (u8*)ext_tags_ptr-(u8*)ext_tags, 1, fp);

	*final_size = (u8*)heap_ptr-(u8*)heap + (u8*)coll_ptr-(u8*)collection + (u8*)ext_tags_ptr-(u8*)ext_tags + 16;

	fclose(fp);
	if(heap) 
		free(heap);
	if(collection) 
		free(collection);
	if(ext_tags)
		free(ext_tags);
	return 0;
}


int spc2_write_header(u16 num_spc)
{
	spc2_header h;
	
	memset(&h, 0, sizeof(spc2_header));
	memcpy(&h.magic, magic, 5);
	h.rev_major = 1;
	h.spc_count = num_spc;
	fwrite(&h, sizeof(spc2_header), 1, fp);

	return 0;
}

int spc2_write_metadata(char *filename, spc2_metadata *o, spc_struct *s, spc_idx6_table *t)
{
	u8 temp_tag_block[3000];
    memset(&temp_tag_block, 0, sizeof(temp_tag_block));
	u32 temp_tag_length=0;
	u32 i;
	memcpy(&o->dsp_regs, &s->ram_dumps.dsp_regs, 128);
	memcpy(&o->ipl_rom, &s->ram_dumps.extra_ram, 64);
	memcpy(&o->cpu_pcl, &s->cpu_regs.cpu_pcl, 7); // as long as the struct isn't reordered, this works
	memcpy(&o->date, &s->date, 4);
	if(t->intro_len)
		o->song_length_16th = t->intro_len;
	else
		o->song_length_16th = s->song_length;
	if(t->fade_len)
		o->fade_length_16th = t->fade_len;
	else
		o->fade_length_16th = s->fade_length;

	memcpy(&o->year_binary, &t->copyright, 2);
	memcpy(&o->ost_disk, &t->ost_disc, 1);
	memcpy(&o->ost_track, &t->ost_track, 2);
	

	for(i=0;i<4;i++)
		filename[strlen(filename)-1]=0;	//Chop off the .spc extension.
	memcpy(&o->spc_filename,filename,28);

	if(s->tag_format == SPC_TAG_BINARY){
		if(t->muted_channels)
			o->chan_enable = t->muted_channels;
		else
			o->chan_enable = s->tag_binary.channel_disable;
		o->emulator = s->tag_binary.emulator;
		memcpy(&o->artist_name, &s->tag_binary.song_artist, 32);
		memcpy(&o->game_name, &s->tag_binary.game_title, 32);
		memcpy(&o->song_name, &s->tag_binary.song_title, 32);
		memcpy(&o->dumper_name, &s->tag_binary.dumper_name, 16);
		memcpy(&o->dumper_name[16], &t->dumper_name, 16);
		if(t->comments[0])
			memcpy(&o->comments, &t->comments, 32);	//Comments might contain a newline.
		else
			memcpy(&o->comments, &s->tag_binary.comments, 32);
	}else{
		if(t->muted_channels)
			o->chan_enable = t->muted_channels;
		else
			o->chan_enable = s->tag_text.channel_disable;
		o->emulator = s->tag_text.emulator;
		memcpy(&o->artist_name, &s->tag_text.song_artist, 32);
		memcpy(&o->game_name, &s->tag_text.game_title, 32);
		memcpy(&o->song_name, &s->tag_text.song_title, 32);
		memcpy(&o->dumper_name, &s->tag_text.dumper_name, 16);
		memcpy(&o->dumper_name[16], &t->dumper_name[16], 16);
		if(t->comments[0])
			memcpy(&o->comments, &t->comments, 32);
		else
			memcpy(&o->comments, &s->tag_text.comments, 32);
	}
	memcpy(&o->official_title, &t->ost_title, 32);
	memcpy(&o->publisher_name, &t->pub_name, 32);

	if(t->song_title[32])
	{
		temp_tag_block[temp_tag_length++]=1;
		temp_tag_block[temp_tag_length++]= strlen(&t->song_title[32]);
		memcpy(&temp_tag_block[temp_tag_length], &t->song_title[32],strlen(&t->song_title[32]));
		temp_tag_length+=strlen(&t->song_title[32]);
	}
	if(t->game_title[32])
	{
		temp_tag_block[temp_tag_length++]=2;
		temp_tag_block[temp_tag_length++]= strlen(&t->game_title[32]);
		memcpy(&temp_tag_block[temp_tag_length], &t->game_title[32],strlen(&t->game_title[32]));
		temp_tag_length+=strlen(&t->game_title[32]);
	}
	if(t->song_artist[32])
	{
		temp_tag_block[temp_tag_length++]=3;
		temp_tag_block[temp_tag_length++]= strlen(&t->song_artist[32]);
		memcpy(&temp_tag_block[temp_tag_length], &t->song_artist[32],strlen(&t->song_artist[32]));
		temp_tag_length+=strlen(&t->song_artist[32]);
	}
	if(t->dumper_name[32])	
	{
		//Original spec for dumper in main tag was 16 bytes. SP2 spec is 32 bytes.
		temp_tag_block[temp_tag_length++]=4;
		temp_tag_block[temp_tag_length++]= strlen(&t->dumper_name[32]);
		memcpy(&temp_tag_block[temp_tag_length], &t->dumper_name[32],strlen(&t->dumper_name[32]));
		temp_tag_length+=strlen(&t->dumper_name[32]);
	}
	if(t->comments[32])
	{
		//Comments might contain a \n (new line).
		temp_tag_block[temp_tag_length++]=5;
		temp_tag_block[temp_tag_length++]= strlen(&t->comments[32]);
		memcpy(&temp_tag_block[temp_tag_length], &t->comments[32],strlen(&t->comments[32]));
		temp_tag_length+=strlen(&t->comments[32]);
	}
	if(t->ost_title[32])
	{
		temp_tag_block[temp_tag_length++]=6;
		temp_tag_block[temp_tag_length++]= strlen(&t->ost_title[32]);
		memcpy(&temp_tag_block[temp_tag_length], &t->ost_title[32],strlen(&t->ost_title[32]));
		temp_tag_length+=strlen(&t->ost_title[32]);
	}
	if(t->pub_name[32])
	{
		temp_tag_block[temp_tag_length++]=7;
		temp_tag_block[temp_tag_length++]= strlen(&t->pub_name[32]);
		memcpy(&temp_tag_block[temp_tag_length], &t->pub_name[32],strlen(&t->pub_name[32]));
		temp_tag_length+=strlen(&t->pub_name[32]);
	}
	if(filename[28])
	{
		temp_tag_block[temp_tag_length++]=8;
		temp_tag_block[temp_tag_length++]= strlen(&filename[28]);
		memcpy(&temp_tag_block[temp_tag_length], &filename[28],strlen(&filename[28]));
		temp_tag_length+=strlen(&filename[28]);
	}
	
	if(temp_tag_length)
	{
		//Define end of extended tag if there is one.
		temp_tag_block[temp_tag_length++]=0;
		temp_tag_block[temp_tag_length++]=0;

		//Check to see if tag is a duplicate of an existing block.
		for(i=0;i<num_tag_blocks;i++)
		{
			if(memcmp(&((u8*)ext_tags)[spc_ext_tags_table[i]], &temp_tag_block[0], temp_tag_length) == 0){
				// found a matching tag.
				o->ptr_extended = spc_ext_tags_table[i];
				goto SkipAddTag;
			}
		}
		//Tag block not found.
		spc_ext_tags_table[num_tag_blocks++]=(u8*)ext_tags_ptr-(u8*)ext_tags;
		o->ptr_extended = (u8*)ext_tags_ptr-(u8*)ext_tags;
		memcpy(ext_tags_ptr,&temp_tag_block,temp_tag_length);
		(u8*)ext_tags_ptr += temp_tag_length;
	}
	else
	{
		o->ptr_extended = 0xFFFFFFFF;	//We need to know if this spc has extended tags or not.
	}

SkipAddTag:
	if(t->amp_val)
		o->amp_value = t->amp_val;
	else
		o->amp_value = 0x10000;

	memcpy(heap_ptr, o, sizeof(spc2_metadata));
	(u8*)heap_ptr += sizeof(spc2_metadata);

	return 0;
}

int spc2_write_spc(char *filename, spc_struct *s, spc_idx6_table *t)
{
	int i;
	int b;
	int added=0;

	spc2_metadata m;
	memset(&m, 0, sizeof(spc2_metadata));

	if(!heap || !collection || !ext_tags) return -1;

	// 64k ram dump has 256 blocks
	for(b = 0; b < 256; b++){
		// look for spc ram block
		SHA1Reset(&sha1);
		SHA1Input(&sha1,&s->ram_dumps.ram[b*256],256);
		SHA1Result(&sha1);
		for(i = 0; (u32)i < num_blocks; i++){
			//if(memcmp(&((u8*)collection)[i*256], &s->ram_dumps.ram[b*256], 256) == 0){
			if(memcmp(&hash_results[i],&sha1.Message_Digest,20) == 0){
				// found a block!
				m.block_offset[b] = i;
				goto skip_add;
			}
		}
		// didn't find the block
		if(num_blocks == 65536)
		{
			//max blocks reached with this SPC, therefore, not adding this spc.
			num_blocks -= added;	//Back off the number of blocks to previous amount.
			(u8*)coll_ptr -= (added * 256);	//Maybe another spc can fill without overfill.
			return -1;
		}
		m.block_offset[b] = num_blocks & 0xFFFF;
		memcpy(coll_ptr, &s->ram_dumps.ram[b*256], 256);
		(u8*)coll_ptr += 256;
		memcpy(&hash_results[num_blocks & 0xFFFF],&sha1.Message_Digest,20);
		num_blocks ++;
		added ++;
skip_add:
		i = 0;
	}
	spc2_write_metadata(filename, &m, s, t);
	
	return 0;
}
