//
// spc2_pack 0.52
// marshallh, CaitSith2, tjanas
// 2023-08-18
//

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <io.h>
#else
#include <glob.h>
#endif

#include "types.hpp"
#include "spc_struct.hpp"
#include "spc_load.hpp"
#include "spc2_struct.hpp"
#include "spc2_write.hpp"

struct spc_file_info
{
	char filename[260];
	unsigned int filelen;
};

#ifdef _WIN32
int spc_file_info_compare(const void* v0, const void* v1)
{
	return strcmp( ((spc_file_info*)v0)->filename, ((spc_file_info*)v1)->filename );
}
#else
// Helper function to obtain .spc filenames within a directory
std::vector< std::string > glob(const std::string& pattern)
{
	// If a directory name contains square brackets, they have to be escaped
	// with backslashes for glob pattern matching.
	std::string escaped_pattern = pattern;
	for (std::size_t pos = 0; ; pos += 2)
	{
		pos = escaped_pattern.find("[", pos);
		if (pos == std::string::npos)
			break;
		escaped_pattern.insert(pos, "\\");
	}
	for (std::size_t pos = 0; ; pos += 2)
	{
		pos = escaped_pattern.find("]", pos);
		if (pos == std::string::npos)
			break;
		escaped_pattern.insert(pos, "\\");
	}

	glob_t glob_result;
	memset(&glob_result, 0, sizeof(glob_result));

	std::vector< std::string > filenames;

	// do the glob operation
	int glob_rc = glob(escaped_pattern.c_str(), GLOB_ERR, NULL, &glob_result);
	if (glob_rc == 0)
	{
		// Collect all the filenames.
		// Case-insensitive sort (to match Microsoft Windows) by creating
		// a map where keys are the uppercased filename, with the values
		// being the unmodified filename.
		filenames.reserve(glob_result.gl_pathc);
		std::map< std::string, std::string > filenames_no_case_sort;
		for (std::size_t i = 0; i < glob_result.gl_pathc; ++i)
		{
			std::string value(glob_result.gl_pathv[i]);
			std::string key = value;
			std::transform(key.begin(), key.end(), key.begin(), ::toupper);

			auto it = filenames_no_case_sort.insert(std::make_pair(key, value));
			if (!it.second) // insert failed; key already exists
			{	// Example: warn if both "Filename.spc" and "FileName.spc" exist
				printf("Ignoring file[%s] since it conflicts with [%s]\n", value.c_str(), it.first->second.c_str());
			}
		}
		for (auto& x : filenames_no_case_sort)
		{   // now sorted, case-insensitive
			filenames.emplace_back(std::move(x.second));
		}
	}

	// cleanup
	globfree(&glob_result);

	// done
	return filenames;
}

std::string getFileName(const std::string& input)
{
	std::string output;
	auto pos = input.rfind('/',input.length());
	if (pos != std::string::npos)
		output = input.substr(pos+1, input.length() - pos);
	return output;
}
#endif

int main(int argc, char* argv[])
{
	spc_struct spc;  // zero-initialized by spc_load
	spc_idx6_table idx6; // zero-initialized by spc_load
	spc_file_info* files = (spc_file_info*) calloc(65535, sizeof(spc_file_info));
	if (NULL == files)
	{
		printf("Error: calloc\n");
		return -1;
	}

	char sp2filename[260];
	u16 file_count = 0;
	u16 song_count = 0;
	int prev_size = 0;
	int final_size = 0;
	int i = 0, k = 0, l = 0;

	memset(&sp2filename, 0, sizeof(sp2filename));

	printf("\n spc2_pack 0.52 (2023-08-18)\n-------------------------------------------\n");

	printf(" Packs multiple Super Nintendo SPC sound\n");
	printf(" files to a single SPC2 \n\n");
	printf(" SPC2 format by kevtris, packer by marshallh\n");
	printf(" mods to packer by CaitSith2 and tjanas\n\n");
	if (argc < 2)
	{
		printf(" Error : No directory specified\n");
		printf(" Try \"spc2_pack . output.sp2\" to compress everything\n");
		printf(" in the current folder\n\n");
		printf(" Also try dragging a directory on top of the program\n\n");
		free(files);
		return -1;
	}
	else if (argc < 3)
	{
		k = 1;
		l = argc;
		goto StartLoop;
	}
	else if (argc == 3)
	{
		strcpy(sp2filename,argv[2]);
		if (strncmp(&sp2filename[strlen(sp2filename)-4],".sp2",4)==0)
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
		if(sp2filename[strlen(sp2filename)-1]=='/')
			sp2filename[strlen(sp2filename)-1]=0;
		strcat(sp2filename,".sp2");
	}


	// append the file search string
	std::string path = argv[k];
	path += "/*.spc";


#ifdef _WIN32
	// use win32 directory API. not posix
	struct _finddata_t spc_file;
	intptr_t dir;
	if( (dir = _findfirst( path.c_str(), &spc_file )) == -1L ) {
		printf( "No *.spc files in current directory!\n" );
		free(files);
		return -1;
	}else{
		do{
			strncpy(files[file_count].filename, spc_file.name, 259);
			prev_size += spc_file.size;
			files[file_count].filelen = spc_file.size;
			++file_count;
		} while( _findnext( dir, &spc_file ) == 0 );
	}
	_findclose(dir);

	// sort files
	qsort(files, file_count, sizeof(files[0]), spc_file_info_compare);
#else
	std::vector< std::string > filenames = glob(path);
	if (filenames.empty())
	{
		printf( "No *.spc files in current directory!\n" );
		free(files);
		return -1;
	}
	else
	{
		for (const auto& spc_file : filenames)
		{
			// Get file size
			std::ifstream ifs(spc_file, std::ios::binary);
			if (!ifs.is_open())
			{
				printf( "Could not open %s\n", spc_file.c_str() );
				free(files);
				return -2;
			}
			const auto pos_start = ifs.tellg();
			ifs.seekg(0, std::ios::end);
			const auto pos_end = ifs.tellg();
			const auto spc_file_size = pos_end - pos_start;

			std::string name = getFileName(spc_file);
			strncpy(files[file_count].filename, name.c_str(), 256);
			prev_size += spc_file_size;
			files[file_count].filelen = spc_file_size;
			++file_count;
		}
	}
#endif

	// compress everything
	if (spc2_start())
	{
		printf("Couldn't allocate memory for SPC2 packing\n");
		free(files);
		return -1;
	}
	for (i = 0; i < file_count; ++i)
	{
		printf(" - %s", files[i].filename);
		path = argv[k];
		path += '/';
		path += files[i].filename;
		if(spc_load(path.c_str(), &spc, &idx6))
		{
			printf(" - Not a valid spc file\n");
			prev_size -= files[i].filelen;
			continue;
		}
		if(!spc2_write_spc(files[i].filename, &spc, &idx6))
		{
			printf("\n");
			++song_count;
		}
		else
		{
			printf(" - Couldn't fit it into the collection\n");
			prev_size -= files[i].filelen;
		}
	}
	spc2_finish(&final_size, sp2filename, song_count);

	printf(" Finished\n");
	printf(" Packed %d files that took %dkb into %dkb\n", song_count, prev_size/1024, final_size/1024);
	if (file_count!=song_count)
		printf(" Couldn't fit remaining %d files into collection\n", file_count-song_count);

	if (++k<l)
		goto StartLoop;

	free(files);
	return 0;
}

