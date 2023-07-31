//
// spc2_pack 0.2
// marshallh
// 1/18/2012
//

#include <stdio.h>
#include <string.h>
#include <io.h>
#include "types.h"
#include "spc_struct.h"
#include "spc_load.h"
#include "spc2_struct.h"
#include "spc2_write.h"

spc_struct		spc;
spc_idx6_table	idx6;
spc2_header		sh;
spc2_metadata	sm;

int main(int argc, char* argv[])
{
	struct _finddata_t spc_file;
	intptr_t dir;

	static char files[65535][260];
	static u32 filelen[65535];
	char path[260];
	char sp2filename[260];
	u16 file_count = 0;
	u16 song_count = 0;
	int prev_size = 0;
	int final_size;
	int i, j, k, l;

	printf("\n spc2_pack 0.4\n-------------------------------------------\n");

	printf(" Packs multiple Super Nintendo SPC sound\n");
	printf(" files to a single SPC2 \n\n");
	printf(" SPC2 format by kevtris, packer by marshallh\n");
	printf(" mods to packer by CaitSith2\n\n");
	if(argc < 2){
		//printf(" Error : No output file specified\n");
		//if(argc < 2) 
			printf(" Error : No directory specified\n");
		printf(" Try \"spc2_pack . output.sp2\" to compress everything\n");
		printf(" in the current folder\n\n");
		printf(" Also try dragging a directory on top of the program\n\n");
		return -1;
	}
	else if(argc < 3){
		k = 1;
		l = argc;
		goto StartLoop;
	}
	else if (argc == 3)
	{
		strcpy(sp2filename,argv[2]);
		if(strncmp(&sp2filename[strlen(sp2filename)-4],".sp2",4)==0)
		{
			k = 1;
			l = 2;
		}
		else
		{
			k = 1;
			l = argc;
			goto StartLoop;
		}
	}
	else
	{
		k = 1;
		l = argc;
	StartLoop:
		file_count = song_count = prev_size = 0;
		strcpy(sp2filename,argv[k]);
		if(sp2filename[strlen(sp2filename)-1]=='\\')
			sp2filename[strlen(sp2filename)-1]=0;
		strcat(sp2filename,".sp2");
	}
	
	
	// append the file search string
	strcpy(path, argv[k]);
	strcat(path, "\\*.spc");
	// use win32 directory API. not posix
	if( (dir = _findfirst( path, &spc_file )) == -1L ) {
		printf( "No *.spc files in current directory!\n" );
		return -1;
	}else{
		do{
			strncpy((char*)&files[file_count], (char*)&spc_file.name, 256);
			prev_size += spc_file.size;
			filelen[file_count] = spc_file.size;
			file_count++;
		} while( _findnext( dir, &spc_file ) == 0 );
	}
	_findclose(dir);

	// compress everything
	if(spc2_start())
	{
		printf("Couldn't allocate memory for SPC2 packing\n");
		return -1;
	}
	for(i = 0; i < file_count; i++){
		printf(" - %s", files[i]);
		strcpy(path, argv[k]);
		strcat(path, "\\");
		strcat(path, files[i]);
		if(spc_load(path, &spc, &idx6))
		{
			printf(" - Not a valid spc file\n");
			prev_size -= filelen[i];
			continue;
		}
		if(!spc2_write_spc(files[i], &spc, &idx6)) 
		{
			printf("\n");
			song_count++;
		}
		else
		{
			printf(" - Couldn't fit it into the collection\n");
			prev_size -= filelen[i];
		}
	}
	spc2_finish(&final_size, sp2filename, song_count);

	printf(" Finished\n");
	printf(" Packed %d files that took %dkb into %dkb\n", 
		song_count, prev_size/1024, final_size/1024); 
	if(file_count!=song_count)
		printf(" Couldn't fit remaining %d files into collection\n",
			file_count-song_count);

	if(++k<l)
		goto StartLoop;

	return 0;
}

