//
// spc2_pack 0.5
// marshallh, CaitSith2, tjanas
// 2023-08-15
//

#include <stdio.h>
#include <stdlib.h>
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

struct spc_file_info
{
	char filename[260];
	unsigned int filelen;
};

int spc_file_info_compare(const void* v0, const void* v1)
{
	return strcmp( ((struct spc_file_info*)v0)->filename, ((struct spc_file_info*)v1)->filename );
}

int main(int argc, char* argv[])
{
	struct _finddata_t spc_file;
	intptr_t dir;

    struct spc_file_info* files = (struct spc_file_info*) calloc(65535, sizeof(struct spc_file_info));
	if (NULL == files)
	{
		printf("Error: calloc\n");
		return -1;
	}

	char path[260];
	char sp2filename[260];
	u16 file_count = 0;
	u16 song_count = 0;
	int prev_size = 0;
	int final_size;
	int i = 0, k = 0, l = 0;

	memset(&sp2filename, 0, sizeof(sp2filename));

	printf("\n spc2_pack 0.5 (2023-08-15)\n-------------------------------------------\n");

	printf(" Packs multiple Super Nintendo SPC sound\n");
	printf(" files to a single SPC2 \n\n");
	printf(" SPC2 format by kevtris, packer by marshallh\n");
	printf(" mods to packer by CaitSith2 and tjanas\n\n");
	if(argc < 2){
		//printf(" Error : No output file specified\n");
		//if(argc < 2) 
			printf(" Error : No directory specified\n");
		printf(" Try \"spc2_pack . output.sp2\" to compress everything\n");
		printf(" in the current folder\n\n");
		printf(" Also try dragging a directory on top of the program\n\n");
        free(files);
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
        free(files);
		return -1;
	}else{
		do{
			strncpy(files[file_count].filename, spc_file.name, 256);
			prev_size += spc_file.size;
			files[file_count].filelen = spc_file.size;
			file_count++;
		} while( _findnext( dir, &spc_file ) == 0 );
	}
	_findclose(dir);

    // sort files
    qsort(files, file_count, sizeof(files[0]), spc_file_info_compare);

	// compress everything
	if(spc2_start())
	{
		printf("Couldn't allocate memory for SPC2 packing\n");
        free(files);
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

    free(files);
	return 0;
}

