//
// spc2_pack 0.2
// marshallh
// 1/18/2012
//

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <glob.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>
#include "types.h"
#include "spc_struct.h"
#include "spc_load.h"
#include "spc2_struct.h"
#include "spc2_write.h"

spc_struct spc;  // zero-initialized by spc_load
spc_idx6_table idx6; // zero-initialized by spc_load

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
	int glob_rc = glob(escaped_pattern.c_str(), GLOB_TILDE, NULL, &glob_result);
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
			{   // Example: warn if both "Filename.spc" and "FileName.spc" exist
				std::cout << "Ignoring file[" << value << "] since it conflicts with [" << it.first->second << "]" << std::endl;
			}
		}
		for (const auto& x : filenames_no_case_sort)
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

int main(int argc, char* argv[])
{
	static char files[65535][260];
	static u32 filelen[65535];
	char sp2filename[260];
	u16 file_count = 0;
	u16 song_count = 0;
	int prev_size = 0;
	int final_size = 0;
	int i = 0, k = 0, l = 0;

	memset(&files, 0, sizeof(files));
	memset(&filelen, 0, sizeof(filelen));
	memset(&sp2filename, 0, sizeof(sp2filename));

	printf("\n spc2_pack 0.4\n-------------------------------------------\n");

	printf(" Packs multiple Super Nintendo SPC sound\n");
	printf(" files to a single SPC2 \n\n");
	printf(" SPC2 format by kevtris, packer by marshallh\n");
	printf(" mods to packer by CaitSith2\n\n");
	if (argc < 2)
	{
		printf(" Error : No directory specified\n");
		printf(" Try \"spc2_pack . output.sp2\" to compress everything\n");
		printf(" in the current folder\n\n");
		printf(" Also try dragging a directory on top of the program\n\n");
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

	std::vector< std::string > filenames = glob(path);
	if (filenames.empty())
	{
		printf( "No *.spc files in current directory!\n" );
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
				return -2;
			}
			const auto pos_start = ifs.tellg();
			ifs.seekg(0, std::ios::end);
			const auto pos_end = ifs.tellg();
			const auto spc_file_size = pos_end - pos_start;

			std::string name = getFileName(spc_file);
			strncpy((char*)&files[file_count], name.c_str(), 256);
			prev_size += spc_file_size;
			filelen[file_count] = spc_file_size;
			++file_count;
		}
	}

	// compress everything
	if (spc2_start())
	{
		printf("Couldn't allocate memory for SPC2 packing\n");
		return -1;
	}
	for (i = 0; i < file_count; ++i)
	{
		printf(" - %s", files[i]);
		path = argv[k];
		path += '/';
		path += files[i];
		if(spc_load(path.c_str(), &spc, &idx6))
		{
			printf(" - Not a valid spc file\n");
			prev_size -= filelen[i];
			continue;
		}
		if(!spc2_write_spc(files[i], &spc, &idx6))
		{
			printf("\n");
			++song_count;
		}
		else
		{
			printf(" - Couldn't fit it into the collection\n");
			prev_size -= filelen[i];
		}
	}
	spc2_finish(&final_size, sp2filename, song_count);

	printf(" Finished\n");
	printf(" Packed %d files that took %dkb into %dkb\n", song_count, prev_size/1024, final_size/1024);
	if (file_count!=song_count)
		printf(" Couldn't fit remaining %d files into collection\n", file_count-song_count);

	if (++k<l)
		goto StartLoop;

	return 0;
}

