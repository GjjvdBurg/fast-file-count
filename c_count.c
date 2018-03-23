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
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
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
void count(char *path, struct filecount *counts, bool recursive, bool hidden, bool quiet) {
	DIR *dir;                /* dir structure we are reading */
	struct dirent *ent;      /* directory entry currently being processed */
	char subpath[PATH_MAX];  /* buffer for building complete subdir and file names */
	/* Some systems don't have dirent.d_type field; we'll have to use stat() instead */
#if !defined ( _DIRENT_HAVE_D_TYPE )
	struct stat statbuf;     /* buffer for stat() info */
#endif

	dir = opendir(path);

	/* opendir failed... file likely doesn't exist or isn't a directory */
	if(NULL == dir) {
		if (!quiet)
			perror(path);
		return;
	}

	while((ent = readdir(dir))) {
		if (strlen(path) + 1 + strlen(ent->d_name) > PATH_MAX) {
			if (!quiet)
				fprintf(stdout, "path too long (%ld) %s%c%s", (strlen(path) + 1 + strlen(ent->d_name)), path, PATH_SEPARATOR, ent->d_name);
			return;
		}

		if (!hidden) {
			#ifdef ON_WINDOWS
			sprintf(subpath, "%s%c%s", path, PATH_SEPARATOR, ent->d_name);
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
		sprintf(subpath, "%s%c%s", path, PATH_SEPARATOR, ent->d_name);
		if(lstat(subpath, &statbuf)) {
			if (!quiet)
				perror(subpath);
			return;
		}

		if(S_ISDIR(statbuf.st_mode)) {
		#endif
			/* Skip "." and ".." directory entries... they are not
			 * "real" directories */
			if(0 == strcmp("..", ent->d_name) || 0 == strcmp(".", ent->d_name))
				continue;

			// if we have d_type, we don't have subpath yet
			#if ( defined ( _DIRENT_HAVE_D_TYPE ))
			sprintf(subpath, "%s%c%s", path, PATH_SEPARATOR, ent->d_name);
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
