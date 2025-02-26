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

struct adaptivemmd_effects_opts {
        const struct adaptived_cause *cse;
};

static void free_opts(struct adaptivemmd_effects_opts * const opts)
{
	if (!opts)
		return;

	free(opts);
}

int adaptivemmd_effects_init(struct adaptived_effect * const eff, struct json_object *args_obj,
               const struct adaptived_cause * const cse)
{
	struct adaptivemmd_effects_opts *opts;
	struct adaptived_cause *xcse = (struct adaptived_cause *)cse;
	int ret = -EINVAL;

	opts = malloc(sizeof(struct adaptivemmd_effects_opts));
	if (!opts) {
		ret = -ENOMEM;
		goto error;
	}

	memset(opts, 0, sizeof(struct adaptivemmd_effects_opts));

	while (xcse) {
		if (strcmp(xcse->name, "adaptivemmd_causes") == 0) {
			break;
		}
		xcse = xcse->next;
	}
	if (xcse == NULL) {
		adaptived_err("adaptivemmd_effects_init: can't find matching cause: adaptivemmd_causes.\n");
		ret = -EINVAL;
		goto error;
	}
	if (strcmp(xcse->name, "adaptivemmd_causes") != 0) {
		adaptived_err("This effect (adaptivemmd_effects) is tightly coupled with the "
			      "adaptivemmd_causes cause.  Provided cause: %s is unsupported\n",
			      xcse->name);
		ret = -EINVAL;
		goto error;
	}
	opts->cse = xcse;

        /* we have successfully setup the logger effect */
        eff->data = (void *)opts;

	return 0;

error:
	adaptived_err("adaptivemmd_effects_init: FAIL, ret=%d\n", ret);
	free_opts(opts);
	return ret;
}

int run_adaptivemm_effects(struct adaptivemmd_opts * const opts)
{
	int ret = 0;
	bool trigger_handled = false;

	if (!opts->trigger_type)
		return 0;
	if (opts->trigger_type & MEMORY_PRESSURE_TRIGGER) {
		adaptived_info("run_adaptivemm_effects: MEMORY_PRESSURE_TRIGGER\n");
		trigger_handled = true;
	}
	if (opts->trigger_type & SUDDEN_MEMORY_LEAK_TRIGGER) {
		adaptived_info("run_adaptivemm_effects: SUDDEN_MEMORY_LEAK_TRIGGER\n");
		trigger_handled = true;
	}
	if (opts->trigger_type & BACKGROUND_MEMORY_LEAK_TRIGGER) {
		adaptived_info("run_adaptivemm_effects: BACKGROUND_MEMORY_LEAK_TRIGGER\n");
		trigger_handled = true;
	}
	if (opts->trigger_type & SLOW_MEMORY_LEAK_TRIGGER) {
		adaptived_info("run_adaptivemm_effects: SLOW_MEMORY_LEAK_TRIGGER\n");
		trigger_handled = true;
	}

	if (!trigger_handled) {
		adaptived_err("%s: effects called, but no handled.\n", __func__);
		ret = -EINVAL;
		goto error;
	}
	return ret;

error:
	return ret;
}

int adaptivemmd_effects_main(struct adaptived_effect * const eff)
{
	struct adaptivemmd_effects_opts *eff_opts = (struct adaptivemmd_effects_opts *)eff->data;
	struct adaptivemmd_opts *opts = (struct adaptivemmd_opts *)eff_opts->cse->data;
	int ret;

	if (!opts) {
		adaptived_err("adaptivemmd_effects_main: eff->data (opts) is NULL\n");
		return -EINVAL;
	}

	ret = run_adaptivemm_effects((struct adaptivemmd_opts * const)opts);
        if (ret < 0)
                goto error;

	adaptived_dbg("%s: OK: ret=%d\n", __func__, ret);
	return ret;

error:
	adaptived_dbg("%s: FAIL: ret=%d\n", __func__, ret);

	return ret;
}

void adaptivemmd_effects_exit(struct adaptived_effect * const eff)
{
	struct adaptivemmd_effects_opts *opts = (struct adaptivemmd_effects_opts *)eff->data;

	free_opts(opts);
}
