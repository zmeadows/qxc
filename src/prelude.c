#include "prelude.h"

#include <errno.h>
#include <ftw.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool strs_are_equal(const char* strA, const char* strB) { return !strcmp(strA, strB); }

void strip_ext(char* fname)
{
    char* end = fname + strlen(fname);

    while (end > fname && *end != '.') {
	--end;
    }

    if (end > fname) {
	*end = '\0';
    }
}

// for make_tmp_dir below
static int remove_callback(const char* pathname,
			   __attribute__((unused)) const struct stat* sbuf,
			   __attribute__((unused)) int type,
			   __attribute__((unused)) struct FTW* ftwb)
{
    return remove(pathname);
}

const char* mk_tmp_dir(void)
{
    /* Create the temporary directory */
    char template[] = "/tmp/qxc.XXXXXX";

    const char* tmp_dirname = mkdtemp(template);

    if (tmp_dirname == NULL) {
	perror("mkdtemp failed: ");
	return NULL;
    }

    return tmp_dirname;
}

void rm_tmp_dir(const char* tmp_path)
{
    if (nftw(tmp_path, remove_callback, FOPEN_MAX, FTW_DEPTH | FTW_MOUNT | FTW_PHYS) ==
	-1) {
	perror("tempdir: error: ");
    }
}

size_t max(size_t a, size_t b) { return a > b ? a : b; }

void print_file(const char* filepath)
{
    FILE* fptr = fopen(filepath, "r");

    if (fptr == NULL) {
	fprintf(stderr, "Cannot open file \n");
    }

    // Read contents from file
    char c = (char)fgetc(fptr);
    while (c != EOF) {
	printf("%c", c);
	c = (char)fgetc(fptr);
    }

    printf("\n\n");

    fclose(fptr);
}
