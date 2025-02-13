/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
/**
 * Cause test for adaptivemmd memory leak background message.
 *
 */

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include <adaptived.h>

#include "ftests.h"

static const char * const sample_dir = "sample_data";

#define EXPECTED_RET -ETIME


static int remove_directory(const char *path)
{
    DIR *dir;
    struct dirent *entry;
    char full_path[1024];

    if (!(dir = opendir(path))) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat statbuf;
        if (stat(full_path, &statbuf) == -1) {
            perror("stat");
            closedir(dir);
            return -1;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // Recursively remove subdirectories
            if (remove_directory(full_path) == -1) {
                closedir(dir);
                return -1;
            }
        } else {
            // Remove files
            if (unlink(full_path) == -1) {
                perror("unlink");
                closedir(dir);
                return -1;
            }
        }
    }

    closedir(dir);

    // Remove the directory itself
    if (rmdir(path) == -1) {
        perror("rmdir");
        return -1;
    }

    return 0;
}

int validate_replay_log(char *expected_file, char *replay_file)
{
        FILE *fp0, *fp1;
        char *line0 = NULL;
        char *line1 = NULL;
        size_t len0 = 0;
        size_t len1 = 0;
        ssize_t read0, read1;
        int i = 0, ret = 0;

	adaptived_dbg("\nvalidate_replay_log: %s vs %s\n", expected_file, replay_file);
        fp0 = fopen(expected_file, "r");
        if (fp0 == NULL)
                return -EINVAL;
        fp1 = fopen(replay_file, "r");
        if (fp1 == NULL) {
		fclose(fp0);
                return -EINVAL;
	}
        while ((read0 = getline(&line0, &len0, fp0)) != -1) {
		read1 = getline(&line1, &len1, fp1);
		if (read1 < 0) {
			ret = -EINVAL;
			goto out;
		}
		if (strncmp(line0, "adaptivemmd ", strlen("adaptivemmd ")) == 0) {
			adaptived_dbg("sudo1008: skip version: %s", line0);
			continue;
		}
		if (strncmp(line0, "adaptivemm_fopen: can't find ", strlen("adaptivemm_fopen: can't find ")) == 0) {
			adaptived_dbg("sudo1008: skip line: %s", line0);
			continue;
		}
		if (strcmp(line0, line1) != 0) {
			adaptived_err("Mismatch!\n");
			line0[strcspn(line0, "\n")] = 0;
			line1[strcspn(line1, "\n")] = 0;
			adaptived_err("Should be: \"%s\"\n", line0);
			adaptived_err("       is: \"%s\"\n", line1);
			ret = -EINVAL;
			goto out;
		}
		i++;
        }
	if (!i) {
		adaptived_err("No line compares done!\n");
		ret = -EINVAL;
	}
out:
        fclose(fp1);
        fclose(fp0);

        return ret;
}

void remove_sample_data()
{
	char cmdline[FILENAME_MAX];

	(void)remove("replay.log");
	sprintf(cmdline, "%s/mem_leak_logs", sample_dir);
	adaptived_info("cmdline: %s\n", cmdline);
	remove_directory(cmdline);
}

int main(int argc, char *argv[])
{
	char config_path[FILENAME_MAX];
	char cmdline[FILENAME_MAX];
	struct adaptived_ctx *ctx;
	int ret = 0;

	snprintf(config_path, FILENAME_MAX - 1, "%s/1008-sudo-adaptivemmd_mem_leak.json", argv[1]);
	config_path[FILENAME_MAX - 1] = '\0';

	ctx = adaptived_init(config_path);
	if (!ctx)
		goto err;

	ret = adaptived_set_attr(ctx, ADAPTIVED_ATTR_MAX_LOOPS, 9);
	if (ret)
		goto err;
	ret = adaptived_set_attr(ctx, ADAPTIVED_ATTR_INTERVAL, 3000);
	if (ret)
		goto err;

	ret = adaptived_set_attr(ctx, ADAPTIVED_ATTR_SKIP_SLEEP, 1);
	if (ret)
		goto err;

	ret = adaptived_set_attr(ctx, ADAPTIVED_ATTR_LOG_LEVEL, LOG_DEBUG);
	if (ret)
		goto err;

	(void)remove("replay.log");

	sprintf(cmdline, "tar xf %s/%s/mem_leak_logs.tar.gz --directory %s",
		argv[1], sample_dir, sample_dir);
	adaptived_dbg("cmdline: %s\n", cmdline);
	ret = system(cmdline);
	if (ret)
		goto err;

	ret = adaptived_loop(ctx, true);
	if (ret != EXPECTED_RET)
		goto err;
	sprintf(cmdline, "%s/mem_leak_logs/adaptivemm.log", sample_dir);
	ret = validate_replay_log(cmdline, "./replay.log"); 
	if (ret)
		goto err;
	adaptived_info("sudo1008: OK\n");
	adaptived_release(&ctx);
	remove_sample_data();
	return AUTOMAKE_PASSED;

err:
	adaptived_info("sudo1008: FAIL, ret=%d\n", ret);
	if (ctx)
		adaptived_release(&ctx);
	remove_sample_data();
	return AUTOMAKE_HARD_ERROR;
}
