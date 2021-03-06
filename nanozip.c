//usage:				 "nanozip [-r] [[-x EXCLUDED_PATH] ...] OUTPUT_NAME.zip INPUT_PATH [...]"

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE 1
#include <stdio.h>

#define __USE_XOPEN_EXTENDED 1
#include <ftw.h>

#include "miniz.h"

enum { MAX_FILE_PATH_LENGTH = 1024, MAX_EXCLUDE_PATHS = 16, MAX_INPUT_PATHS = 16, USE_FDS = 15 };

char* exclude[MAX_EXCLUDE_PATHS];
char* input[MAX_INPUT_PATHS];
char file_path_buffer[MAX_FILE_PATH_LENGTH];
char* output;

int recurse, num_input, num_exclude;

void* ptr_zip;

int proc_entry(const char *file_path_src, const struct stat *info, const int typeflag, struct FTW *pathinfo)
{
	char *file_path_dst;
    int add;
    
    add = 1;

    for(int i = 0; i < num_exclude; i++)
        if(0 == strcmp(exclude[i], file_path_src))
            add = 0;
    
    printf("deflating [%s]\n", file_path_src);

    if(add)
    {
        file_path_dst = file_path_src;

        if(typeflag == FTW_D)
        {
            if(file_path_src[strlen(file_path_src) - 1] != '/')
            {
                file_path_dst = file_path_buffer;
                snprintf(file_path_dst, MAX_FILE_PATH_LENGTH, "%s/", file_path_src);
            }
            mz_zip_writer_add_mem(ptr_zip, file_path_dst[0] == '/' ? file_path_dst + 1 : file_path_dst, NULL, 0, 0);
        }
        else if(typeflag == FTW_F)
            mz_zip_writer_add_file(ptr_zip, file_path_dst[0] == '/' ? file_path_dst + 1 : file_path_dst, file_path_src, "", 0, MZ_BEST_COMPRESSION);
    }
    
    return 0;
}

int main(int argc, char *argv[])
{
    mz_zip_archive zip;
    struct stat st;

    recurse = num_input = num_exclude = 0;
    for(int i = 1; i < argc; i++)
    {
        if(0 == strcmp("-r", argv[i]))
            recurse = 1;
        else if(0 == strcmp("-x", argv[i]))
        {
            assert(i + 1 < argc);
            exclude[num_exclude++] = argv[++i];
        }
        else if(output == NULL)
            output = argv[i];
        else
            input[num_input++] = argv[i];
    }

	remove(output);

    ptr_zip = &zip;
	memset(ptr_zip, 0, sizeof(zip));
    mz_zip_writer_init_file(ptr_zip, output, 0);

    for (int i = 0; i < num_input; i++)
	{
        if(stat(input[i], &st) == 0 && (st.st_mode & S_IFDIR) != 0)
        {
            if(recurse == 1)
                nftw(input[i], proc_entry, USE_FDS, FTW_PHYS);
        }
        else
            proc_entry(input[i], NULL, FTW_F, NULL);
	}

    mz_zip_writer_finalize_archive(ptr_zip);
    mz_zip_writer_end(ptr_zip);
	
    return 0;
}
