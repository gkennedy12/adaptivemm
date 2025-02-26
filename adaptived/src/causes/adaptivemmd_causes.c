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
 * Cause to evaluate and respond to memory consumption trend
 *
 */

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <time.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <stdbool.h>
#include <signal.h>
#include <linux/kernel-page-flags.h>

#include <adaptived-utils.h>
#include <adaptived.h>

#include "adaptived-internal.h"
#include "defines.h"

// #include "adaptivemmd.h"
#include "../../../adaptivemm_opts.h"

#define VERSION         "9.9.9"

extern bool memory_pressure_check_enabled;
extern bool neg_dentry_check_enabled;
extern bool memleak_check_enabled;

extern int one_time_initializations();
extern int run_memory_checks();
extern void free_numa_arrays(struct adaptivemmd_opts * const opts);
extern int setup_numa_arrays(struct adaptivemmd_opts * const opts);
extern int shared_adaptivemmd_init();

// extern int iteration;
extern int verbose;
extern int debug_mode;
extern unsigned long maxgap;
extern int neg_dentry_pct;
extern int aggressiveness;
extern int periodicity;
extern int skip_dmazone;
extern char *log_file;
extern char *sample_path;

static void free_opts(struct adaptivemmd_opts * const opts)
{
	if (!opts)
		return;

	if (opts->capture_data_dir) {
		free(opts->capture_data_dir);
		opts->capture_data_dir = NULL;
	}
	if (opts->log_file) {
		free(opts->log_file);
		opts->log_file = NULL;
	}
	free_numa_arrays(opts);

	free(opts);
}

/*
 * run_adaptivemm_init() - Initialize settings that are set once at
 *	adaptivemmd startup
 *
 */
int run_adaptivemm_init(struct adaptivemmd_opts * const opts, int interval)
{
	int ret = 0;

        if (shared_adaptivemmd_init() < 0)
		goto error;

        /*
         * Update free page and hugepage counts before initialization
         */
	ret = one_time_initializations();
	adaptived_dbg("%s: one_time_initializations() ret=%d\n", __func__, ret);

	// iteration++;
	return ret;

error:
	adaptived_err("run_adaptivemm_init: FAIL, ret=%d\n", ret);
	return ret;
}

int adaptivemmd_causes_init(struct adaptived_cause * const cse, struct json_object *args_obj, int interval)
{
	struct adaptivemmd_opts *opts;
	int ret = -EINVAL;
	const char *file_str;

	opts = malloc(sizeof(struct adaptivemmd_opts));
	if (!opts) {
		ret = -ENOMEM;
		goto error;
	}

	memset(opts, 0, sizeof( struct adaptivemmd_opts));

	ret = setup_numa_arrays(opts);
	if (ret < 0)
		goto error;
        ret = adaptived_parse_bool(args_obj, "ENABLE_FREE_PAGE_MGMT", &memory_pressure_check_enabled);
        if (ret && ret != -ENOENT) {
		adaptived_err("ENABLE_FREE_PAGE_MGMT failed, ret=%d\n", ret);
                goto error;
        }
        ret = adaptived_parse_bool(args_obj, "ENABLE_NEG_DENTRY_MGMT", &neg_dentry_check_enabled);
        if (ret && ret != -ENOENT) {
		adaptived_err("ENABLE_NEG_DENTRY_MGMT failed, ret=%d\n", ret);
                goto error;
        }
        ret = adaptived_parse_bool(args_obj, "ENABLE_MEMLEAK_CHECK", &memleak_check_enabled);
        if (ret && ret != -ENOENT) {
		adaptived_err("ENABLE_MEMLEAK_CHECK failed, ret=%d\n", ret);
                goto error;
        }
	adaptived_dbg("%s: memory_pressure_check_enabled=%d, neg_dentry_check_enabled=%d, memleak_check_enabled=%d\n",
		__func__, memory_pressure_check_enabled, neg_dentry_check_enabled, memleak_check_enabled);

	if (!memory_pressure_check_enabled && !neg_dentry_check_enabled && !memleak_check_enabled) {
		adaptived_err("%s: Not enables set.\n", __func__);
		ret = -ENOENT;
                goto error;
	}
#if 0
        ret = adaptived_parse_int(args_obj, "mem_trigger_delta", &opts->mem_trigger_delta);
        if (ret == -ENOENT) {
                opts->mem_trigger_delta = MEM_TRIGGER_DELTA;
        } else if (ret) {
                goto error;
        }
	adaptived_dbg("Minimum %% change in meminfo numbers trigger: %d\n", opts->mem_trigger_delta);
        if (opts->mem_trigger_delta < 0 || opts->mem_trigger_delta > 100) {
		adaptived_err("mem_trigger_delta %d invalid.\n", opts->mem_trigger_delta);
                goto error;
	}

        ret = adaptived_parse_int(args_obj, "unacct_mem_grth_max", &opts->unacct_mem_grth_max);
        if (ret == -ENOENT) {
                opts->unacct_mem_grth_max = UNACCT_MEM_GRTH_MAX;
        } else if (ret) {
                goto error;
        }
	adaptived_dbg("Unaccounted memory growth max samples: %d\n", opts->unacct_mem_grth_max);
        if (opts->unacct_mem_grth_max < 0) {
		adaptived_err("unacct_mem_grth_max %d invalid.\n", opts->unacct_mem_grth_max);
                goto error;
	}
#endif
	ret = adaptived_parse_int(args_obj, "neg_dentry_pct", &neg_dentry_pct);
	if (ret == -ENOENT) {
		neg_dentry_pct = MAX_NEGDENTRY;
	} else if (ret) {
		goto error;
	}
	adaptived_dbg("%s: neg_dentry_pct = %d\n", __func__, neg_dentry_pct);

	ret = adaptived_parse_int(args_obj, "maxgap", (int *)&maxgap);
	if (ret == -ENOENT) {
		maxgap = 0;
	} else if (ret) {
		goto error;
	}
	adaptived_dbg("%s: maxgap = %d\n", __func__, maxgap);

        ret = adaptived_parse_int(args_obj, "debug_mode", &debug_mode);
        if (ret == -ENOENT) {
                debug_mode = 0;
        } else if (ret) {
                goto error;
        }
        adaptived_dbg("%s: debug_mode = %d\n", __func__, debug_mode);

        ret = adaptived_parse_int(args_obj, "verbose", &verbose);
        if (ret == -ENOENT) {
                verbose = 0;
        } else if (ret) {
                goto error;
        }
        adaptived_dbg("%s: verbose = %d\n", __func__, verbose);

	ret = adaptived_parse_string(args_obj, "log_file", &file_str);
	if (ret == -ENOENT) {
		opts->log_file = NULL;
		ret = 0;
	} else if (ret) {
		adaptived_err("Failed to parse log_file.\n");
		goto error;
	} else {
		if (access(file_str, F_OK) == 0) {
			adaptived_err("adaptivemmd_causes_init: log_file %s exists. Log file cannot exist to avoid overwrite.\n", file_str);
			ret = -EINVAL;
			goto error;
		}
		opts->log_file = malloc(sizeof(char) * strlen(file_str) + 1);
		if (!opts->log_file) {
			ret = -ENOMEM;
			goto error;
		} else {
			char* pathCopy = strdup(file_str);
			char* directory = dirname(pathCopy);

			if (access(directory, F_OK) != 0) {
				if (mkdir(directory, 0755) != 0 && errno != EEXIST) {
					adaptived_err("adaptivemmd_causes_init: mkdir() dir %s failed, errno=%d\n", file_str, errno);
					free(pathCopy);
					ret = -errno;
					goto error;
				}
			}
			free(pathCopy);

			strcpy(opts->log_file, file_str);
			opts->log_file[strlen(file_str)] = '\0';

			log_file = opts->log_file;
			adaptived_dbg("%s: log_file=%s\n", __func__, opts->log_file);
		}
	}
	ret = adaptived_parse_string(args_obj, "capture_data_dir", &file_str);
	if (ret == -ENOENT) {
		opts->capture_data_dir = NULL;
		ret = 0;
	} else if (ret) {
		adaptived_err("Failed to parse capture_data_dir.\n");
		goto error;
	} else {
		if (access(file_str, F_OK) != 0) {
			adaptived_err("adaptivemmd_causes_init: Can't find capture_data_dir %s\n", file_str);
			ret = -EINVAL;
			goto error;
		}
		opts->capture_data_dir = malloc(sizeof(char) * strlen(file_str) + 1);
		if (!opts->capture_data_dir) {
			ret = -ENOMEM;
			goto error;
		}

		strcpy(opts->capture_data_dir, file_str);
		opts->capture_data_dir[strlen(file_str)] = '\0';

		sample_path = opts->capture_data_dir;
		adaptived_dbg("adaptivemmd_causes_init: data will be sampled dir: %s\n", opts->capture_data_dir);
	}

	ret = adaptived_cause_set_data(cse, (void *)opts);
        if (ret)
                goto error;

	ret = run_adaptivemm_init(opts, interval);	/* must be done after adaptived_cause_set_data() */
        if (ret < 0)
                goto error;


	adaptived_dbg("%s: opts=%p, memory_pressure_check_enabled=%d, neg_dentry_check_enabled=%d, memleak_check_enabled=%d\n",
                __func__, opts, memory_pressure_check_enabled, neg_dentry_check_enabled, memleak_check_enabled);

	return ret;

error:
	adaptived_err("adaptivemmd_causes_init: FAIL, ret=%d\n", ret);
	free_opts(opts);
	return ret;
}

int adaptivemmd_causes_main(struct adaptived_cause * const cse, int time_since_last_run)
{
	struct adaptivemmd_opts *opts = (struct adaptivemmd_opts *)adaptived_cause_get_data(cse);
	int ret;

	ret = run_memory_checks();
	if (ret < 0) {
		adaptived_err("run_adaptivemm: run_memory_checks() failed.\n");
		goto error;
	}
	opts->trigger_type = ret;
	// iteration++;	/* needed for replay sample data */

	adaptived_dbg("%s: OK: ret=%d\n", __func__, ret);

	return ret;

error:
	adaptived_dbg("%s: FAIL: ret=%d\n", __func__, ret);

	return ret;
}

void adaptivemmd_causes_exit(struct adaptived_cause * const cse)
{
	struct adaptivemmd_opts *opts = (struct adaptivemmd_opts *)adaptived_cause_get_data(cse);

	free_opts(opts);
}
