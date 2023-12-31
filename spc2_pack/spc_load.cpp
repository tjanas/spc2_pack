#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <type_traits>
#include "types.hpp"
#include "spc_struct.hpp"
#include "spc_load.hpp"

int IsNumeric(const char* str, u32 length)
{
	u32 c = 0;
	while (c<length && isdigit(str[c])) ++c;
	if(c==length || str[c]==0)
		return c;
	else
		return -1;
}

int CountNumbers(const char* str, u32 length)
{
	u32 c = 0;
	while (c<length && isdigit(str[c])) ++c;
	return c;
}

int IsDate(const char* str, u32 length)
{
	u32 c = 0;
	while (c<length && (isdigit(str[c]) || str[c]=='/' || str[c]=='-')) ++c;
	if(c==length || str[c]==0)
		return c;
	else
		return -1;
}

int spc_load(const char* filename, spc_struct *s, spc_idx6_table *t)
{
	if (NULL == s)
		return SPC_LOAD_UNDEFINED;
	if (NULL == t)
		return SPC_LOAD_UNDEFINED;

	u8 buf[8];
	memset(&buf, 0, sizeof(buf));
	spc_header *h = &s->header;
	spc_id666_text *it = &s->tag_text;
	spc_id666_bin *ib = &s->tag_binary;

	spc_idx6_header	idx6h;
	memset(&idx6h, 0, sizeof(idx6h));

	int i=0, j=0, k=0, l=0, m=0, d=0, y=0;

	FILE* fp = fopen(filename, "rb");
	if(fp == NULL)
	{	// invalid
		fprintf(stderr, "*** ERROR spc_load(): couldn't open \"%s\"\n", filename);
		return SPC_LOAD_FILEERR;
	}

	memset(s, 0, sizeof(spc_struct));
	memset(t, 0, sizeof(spc_idx6_table));

	// read header
	if (fread(h, 1, 37, fp) != 37)
	{	// fread error
		fprintf(stderr, "*** ERROR spc_load(): could not read header; filename: \"%s\"\n", filename);
		fclose(fp);
		return SPC_LOAD_INVALID;
	}
	if(strncmp( (const char*)(h->header), "SNES-SPC700 Sound File Data", 27) != 0)
	{	// not an SPC!
		fprintf(stderr, "*** ERROR spc_load(): invalid header; filename: \"%s\"\n", filename);
		fclose(fp);
		return SPC_LOAD_INVALID;
	}

	// read cpu registers
	if (fread(&s->cpu_regs, 1, 9, fp) != 9)
	{	// fread error
		fprintf(stderr, "*** ERROR spc_load(): could not read CPU registers; filename: \"%s\"\n", filename);
		fclose(fp);
		return SPC_LOAD_INVALID;
	}

	if (fread(it, 1, 210, fp) != 210)
	{	// fread error
		fprintf(stderr, "*** ERROR spc_load(): could not read 210 bytes starting at offset 0x2E; filename: \"%s\"\n", filename);
		fclose(fp);
		return SPC_LOAD_INVALID;
	}
	if (h->id3_tag_present)
	{
		// start by assuming it's binary
		s->tag_format = SPC_TAG_PREFER_BINARY;

		i = IsNumeric( (const char*)(it->song_length_secs), 3);
		j = IsNumeric( (const char*)(it->fade_length_ms), 5);
		k = IsDate( (const char*)(it->date_dumped), 11);

		if (!(i | j | k))
		{
			if (it->channel_disable == 1 && it->emulator == 0)
				s->tag_format = SPC_TAG_BINARY;

			if (ib->reserved[0] >= '0' && ib->reserved[0] <= '2')
				s->tag_format = SPC_TAG_TEXT;

			if ((it->fade_length_ms[4] == 0) && (isascii(it->song_artist[0]) && (it->song_artist[0] != 0)))
				s->tag_format = SPC_TAG_TEXT; //Conclusively text.
		}
		else
		{
			if (i != -1 && j != -1)
			{
				if (k > 0)
					s->tag_format = SPC_TAG_TEXT;
				else
				{
					if (k == -1)
					{
						if (!((u32*)&it->date_dumped)[1])
							s->tag_format = SPC_TAG_BINARY;
						else
							s->tag_format = SPC_TAG_TEXT;
					}
					if ((it->fade_length_ms[4] == 0) && (isascii(it->song_artist[0]) && (it->song_artist[0] != 0)))
						s->tag_format = SPC_TAG_TEXT; //Conclusively text.
					if ((i == 3) || (j == 5))
						s->tag_format = SPC_TAG_TEXT;
				}
			}
		}

		if(s->tag_format == SPC_TAG_PREFER_BINARY && ib->reserved[0] >= '0' && ib->reserved[0] <= '2')
			s->tag_format = SPC_TAG_TEXT;

		if(s->tag_format == SPC_TAG_PREFER_BINARY)
			s->tag_format = SPC_TAG_BINARY;


		if(s->tag_format == SPC_TAG_TEXT){
			//printf("tag format : text\n");
			i = CountNumbers( (const char*)(&it->date_dumped[0]), 11);
			j = CountNumbers( (const char*)(&it->date_dumped[i+1]), 11-i-1);
			k = CountNumbers( (const char*)(&it->date_dumped[i+1+j+1]), 11-j-1);

			if((i==4 && j>0 && j<=2 && k>0 && k<=2) || (i>0 && i<=2 && j>0 && j<=2 && k==4))
			{

				if(i==4) //YYYY.MM.DD format.
				{
					for(l=0,y=0;l<i;++l)
					{
						y*=10;
						y+=(it->date_dumped[l]-48);
					}
					for(l=(i+1),m=0;l<(i+1+j);++l)
					{
						m*=10;
						m+=(it->date_dumped[l]-48);
					}
					for(l=(i+1+j+1),d=0;l<(i+1+j+1+k);++l)
					{
						d*=10;
						d+=(it->date_dumped[l]-48);
					}
				}
				else
				{
					for(l=0,m=0;l<i;++l)
					{
						m*=10;
						m+=(it->date_dumped[l]-48);
					}
					for(l=(i+1),d=0;l<(i+1+j);++l)
					{
						d*=10;
						d+=(it->date_dumped[l]-48);
					}
					for(l=(i+1+j+1),y=0;l<(i+1+j+1+k);++l)
					{
						y*=10;
						y+=(it->date_dumped[l]-48);
					}
				}
				if(m > 12)
				{
					i = m;
					m = d;
					d = i;
				}

				if ((m == 0) || (m > 12) || (d == 0) || (d > 31) || (y < 1900) || (y > 2999))
					s->date = 0;
				else
					s->date = ((m / 10) << 28) |
							((m % 10) << 24) |
							((d / 10) << 20) |
							((d % 10) << 16) |
							(((y / 100) / 10) << 12) |
							(((y / 100) % 10) << 8) |
							(((y % 100) / 10) << 4) |
							(((y % 100) % 10) << 0);
			}
			else
				s->date = 0; //Impossible or zero.

			// impossible, or zero
			if(s->date > 0x99999999) s->date = 0;
			//printf("date: %08x\n", s->date);

			memcpy(buf, &it->song_length_secs, 3); buf[4] = 0;
			s->song_length = atoi((const char*)buf)*64000;
			memcpy(buf, &it->fade_length_ms, 5); buf[6] = 0;
			s->fade_length = atoi((const char*)buf)*64;
		}else{
			// binary
			//printf("tag format : binary\n");

			y = (ib->date_dumped >> 16) & 0xffff;
			m = (ib->date_dumped >> 8) & 0xff;
			d = (ib->date_dumped >> 0) & 0xff;

			if(m > 12)
			{
				i = m;
				m = d;
				d = i;
			}

			if ((m == 0) || (m > 12) || (d == 0) || (d > 31) || (y < 1900) || (y > 2999))
				s->date = 0;
			else
				s->date = ((m / 10) << 28) |
						((m % 10) << 24) |
						((d / 10) << 20) |
						((d % 10) << 16) |
						(((y / 100) / 10) << 12) |
						(((y / 100) % 10) << 8) |
						(((y % 100) / 10) << 4) |
						(((y % 100) % 10) << 0);

			//printf("date: %08x\n", s->date);

			s->song_length = 0;
			memcpy(&s->song_length, &ib->song_length_secs, 3);
			s->song_length &= 0x00FFFFFF;
			s->song_length *= 64000;
			memcpy(&s->fade_length, &ib->fade_length_ms, 4);
			s->fade_length *= 64;
		}
		//printf("song title: %s\ngame title: %s\n", it->song_title, it->game_title);
	}

	// read ram dumps
	if (fread(&s->ram_dumps.ram, 1, 65536, fp) != 65536)
	{	// fread error
		fprintf(stderr, "*** ERROR spc_load(): could not read 64KB RAM bytes; filename: \"%s\"\n", filename);
		fclose(fp);
		return SPC_LOAD_INVALID;
	}
	if (fread(&s->ram_dumps.dsp_regs, 1, 128, fp) != 128)
	{	// fread error
		fprintf(stderr, "*** ERROR spc_load(): could not read DSP Registers; filename: \"%s\"\n", filename);
		fclose(fp);
		return SPC_LOAD_INVALID;
	}
	if (fread(&s->ram_dumps.unused, 1, 64, fp) != 64)
	{	// fread error
		fprintf(stderr, "*** ERROR spc_load(): could not read 64 bytes at offset 0x10180; filename: \"%s\"\n", filename);
		fclose(fp);
		return SPC_LOAD_INVALID;
	}
	if (fread(&s->ram_dumps.extra_ram, 1, 64, fp) != 64)
	{	// fread error
		fprintf(stderr, "*** ERROR spc_load(): could not read Extra RAM bytes; filename: \"%s\"\n", filename);
		fclose(fp);
		return SPC_LOAD_INVALID;
	}

	// Read extended ID666 info
	if ( (fread(&idx6h.header, 1, 8, fp) == 8) &&
	     (strncmp((const char*)&idx6h.header, "xid6", 4) == 0) )
	{
		std::size_t bytes_read = fread(&idx6h.data[0], 1, idx6h.size, fp);
		if (bytes_read != idx6h.size)
		{
			fprintf(stderr, "*** WARN spc_load(): xid6 header chunk size is %u but only read %zu bytes; filename: \"%s\"\n", idx6h.size, bytes_read, filename);
		}

		u32 offset = 0;
		while ( (offset < idx6h.size) && (offset < bytes_read) )
		{
			spc_idx6_sub_header* idx6sh = (spc_idx6_sub_header*)&idx6h.data[offset];
			switch(idx6sh->ID)
			{
			case IDX6_SONGNAME:
				memcpy(&t->song_title, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), ((idx6sh->Type)?idx6sh->Length:2));
				break;
			case IDX6_ARTISTNAME:
				memcpy(&t->song_artist, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), ((idx6sh->Type)?idx6sh->Length:2));
				break;
			case IDX6_COMMENTS:
				memcpy(&t->comments, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), ((idx6sh->Type)?idx6sh->Length:2));
				break;
			case IDX6_DUMPERNAME:
				memcpy(&t->dumper_name, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), ((idx6sh->Type)?idx6sh->Length:2));
				break;
			case IDX6_PUBNAME:
				memcpy(&t->pub_name, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), ((idx6sh->Type)?idx6sh->Length:2));
				break;
			case IDX6_GAMENAME:
				memcpy(&t->game_title, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), ((idx6sh->Type)?idx6sh->Length:2));
				break;
			case IDX6_OSTTITLE:
				memcpy(&t->ost_title, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), ((idx6sh->Type)?idx6sh->Length:2));
				break;
			case IDX6_OSTDISC:
				memcpy(&t->ost_disc, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), 1);
				break;
			case IDX6_OSTTRACK:
				memcpy(&t->ost_track, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), 2);
				break;
			case IDX6_COPYRIGHT:
				memcpy(&t->copyright, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), 2);
				break;
			case IDX6_INTROLEN:
				memcpy(&t->intro_len, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), ((idx6sh->Type)?idx6sh->Length:2));
				break;
			case IDX6_LOOPLEN:
				memcpy(&t->loop_len, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), ((idx6sh->Type)?idx6sh->Length:2));
				break;
			case IDX6_ENDLEN:
				memcpy(&t->end_len, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), ((idx6sh->Type)?idx6sh->Length:2));
				break;
			case IDX6_FADELEN:
				memcpy(&t->fade_len, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), ((idx6sh->Type)?idx6sh->Length:2));
				break;
			case IDX6_MUTECHAN:
				memcpy(&t->muted_channels, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), 1);
				break;
			case IDX6_LOOPNUM:
				memcpy(&t->loop_count, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), 1);
				break;
			case IDX6_AMPVAL:
				memcpy(&t->amp_val, ((idx6sh->Type)?idx6sh->data:(u8*)&idx6sh->Length), ((idx6sh->Type)?idx6sh->Length:2));
				break;
			default:
				fprintf(stderr, "*** WARN spc_load(): unknown/malformed xid6 tag ID (0x%02x); filename: \"%s\"\n", idx6sh->ID, filename);
				break;
			}
			offset += 4 + ((idx6sh->Type)?((idx6sh->Length+3)&(~3)):0);
		}
	}

	fclose(fp);
	return SPC_LOAD_SUCCESS;
}
