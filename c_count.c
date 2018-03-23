/**
 *
 * dircnt.c - a fast file-counting program.
 *
 * Written 2015-02-06 by Christopher Schultz as a programming demonstration
 * for a StackOverflow answer:
 * https://stackoverflow.com/questions/1427032/fast-linux-file-count-for-a-large-number-of-files/28368788#28368788
 *
 * Please see the README.md file for compilation and usage instructions.
 *
 * Thanks to FlyingCodeMonkey, Gary R. Van Sickle, and Jonathan Leffler for
 * various suggestions and improvements to the original code. Any additional
 * contributors can be found by looking at the GitHub revision history from
 * this point forward..
 *
 * 20180323: Changed for use in a Python package by Gertjan van den Burg.
 *   - Removed main
 *   - Added options for recurse, hidden, and quiet
 *   - Removed debug code
 *   - Reformatted long lines
 *
 */
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdbool.h>

#if defined(WIN32) || defined(_WIN32)
#include <Windows.h>
#define ON_WINDOWS
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

/* A custom structure to hold separate file and directory counts */
struct filecount {
	long dirs;
	long files;
};


bool str_startswith(const char *str, const char *pre)
{
	size_t lenpre = strlen(pre),
	       lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}


/*
 * counts the number of files and directories in the specified directory.
 *
 * path - relative pathname of a directory whose files should be counted
 * counts - pointer to struct containing file/dir counts
 */
void count(char *path, struct filecount *counts, bool recursive, bool hidden, 
		bool quiet) {
	long len;
	/* dir structure we are reading */
	DIR *dir;
	/* directory entry currently being processed */
	struct dirent *ent;
	/* buffer for building complete subdir and file names */
	char subpath[PATH_MAX];

	/* Some systems don't have dirent.d_type field; we'll have to use 
	 * stat() instead */
	#if !defined ( _DIRENT_HAVE_D_TYPE )
	/* buffer for stat() info */
	struct stat statbuf;
	#endif

	dir = opendir(path);

	/* opendir failed... file likely doesn't exist or isn't a directory */
	if(NULL == dir) {
		if (!quiet)
			perror(path);
		return;
	}

	while((ent = readdir(dir))) {
		len = strlen(path) + 1 + strlen(ent->d_name);
		if (len > PATH_MAX) {
			if (!quiet)
				fprintf(stdout, "path too long (%ld) %s%c%s", 
						len, path, PATH_SEP, ent->d_name);
			return;
		}

		if (!hidden) {
			#ifdef ON_WINDOWS
			sprintf(subpath, "%s%c%s", path, PATH_SEP, ent->d_name);
			if (GetFileAttribute(_T(subpath)) & FILE_ATTRIBUTE_HIDDEN)
				continue;
			#else
			if (str_startswith(ent->d_name, "."))
				continue;
			#endif
		}

		/* Use dirent.d_type if present, otherwise use stat() */
		#if ( defined ( _DIRENT_HAVE_D_TYPE ))
		if(DT_DIR == ent->d_type) {
		#else
		sprintf(subpath, "%s%c%s", path, PATH_SEP, ent->d_name);
		if(lstat(subpath, &statbuf)) {
			if (!quiet)
				perror(subpath);
			return;
		}

		if(S_ISDIR(statbuf.st_mode)) {
		#endif
			/* Skip "." and ".." directory entries... they are not
			 * "real" directories */
			if (strcmp(".", ent->d_name) == 0)
				continue;
			if (strcmp("..", ent->d_name) == 0)
				continue;

			// if we have d_type, we don't have subpath yet
			#if ( defined ( _DIRENT_HAVE_D_TYPE ))
			sprintf(subpath, "%s%c%s", path, PATH_SEP, ent->d_name);
			#endif
			counts->dirs++;
			if (recursive)
				count(subpath, counts, recursive, hidden, quiet);
		} else {
			counts->files++;
		}
	}
	closedir(dir);
}


void c_fast_file_count(char *path, long l_recursive, long l_hidden,
		long l_quiet, long *result_files, long *result_dirs)
{
	bool recursive = l_recursive;
	bool hidden = l_hidden;
	bool quiet = l_quiet;

	struct filecount counts;
	counts.files = 0;
	counts.dirs = 0;

	count(path, &counts, recursive, hidden, quiet);

	*result_files = counts.files;
	*result_dirs = counts.dirs;
}
