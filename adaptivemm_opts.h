/*
 *  * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
 *  * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *  *
 *  * This code is free software; you can redistribute it and/or modify it
 *  * under the terms of the GNU General Public License version 2 only, as
 *  * published by the Free Software Foundation.
 *  *
 *  * This code is distributed in the hope that it will be useful, but WITHOUT
 *  * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  * version 2 for more details (a copy is included in the LICENSE file that
 *  * accompanied this code).
 *  *
 *  * You should have received a copy of the GNU General Public License version
 *  * 2 along with this work; if not, write to the Free Software Foundation,
 *  * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *  *
 *  * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 *  * or visit www.oracle.com if you need additional information or have any
 *  * questions.
 *  */
#ifndef ADAPTIVEMM_OPTS_H
#define	ADAPTIVEMM_OPTS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * System files that provide information
 */
#define	BUDDYINFO		"/proc/buddyinfo"
#define ZONEINFO		"/proc/zoneinfo"
#define VMSTAT			"/proc/vmstat"
#define MEMINFO			"/proc/meminfo"
#define KPAGECOUNT		"/proc/kpagecount"
#define KPAGEFLAGS		"/proc/kpageflags"
#define MODULES			"/proc/modules"
#define HUGEPAGESINFO		"/sys/kernel/mm/hugepages"

/*
 * System files to control reclamation and compaction
 */
#define RESCALE_WMARK		"/proc/sys/vm/watermark_scale_factor"
#define	COMPACT_PATH_FORMAT	"/sys/devices/system/node/node%d/compact"

/*
 * System files to control negative dentries
 */
#define NEG_DENTRY_LIMIT	"/proc/sys/fs/negative-dentry-limit"

/*
 * Possible locations for configuration files
 */
#define CONFIG_FILE1		"/etc/sysconfig/adaptivemmd"
#define CONFIG_FILE2		"/etc/default/adaptivemmd"

#define MAX_NUMANODES	1024

#define MAX_VERBOSE	5
#define MAX_AGGRESSIVE	3
#define MAX_NEGDENTRY	100

/*
 * Number of consecutive samples showing growth in unaccounted memory
 * that will trigger memory leak warning
 */
#define UNACCT_MEM_GRTH_MAX	10

/* Minimum % change in meminfo numbers to trigger warnings */
#define MEM_TRIGGER_DELTA	10

struct adaptivemmd_opts {
        int max_numa_nodes;
        int *compaction_requested;
        unsigned long *last_bigpages;
        // struct lsq_struct page_lsq[MAX_NUMANODES][MAX_ORDER];
        struct lsq_struct **page_lsq;
        unsigned long *min_wmark;
        unsigned long *low_wmark;
        unsigned long *high_wmark;
        unsigned long *managed_pages;
	int trigger_type;
	char *log_file;
	char *capture_data_dir;
};

#define NO_TRIGGER			0
#define MEMORY_PRESSURE_TRIGGER		0x1
#define SUDDEN_MEMORY_LEAK_TRIGGER	0x2
#define BACKGROUND_MEMORY_LEAK_TRIGGER	0x4
#define SLOW_MEMORY_LEAK_TRIGGER	0x8

#ifdef __cplusplus
}
#endif

#endif /* ADAPTIVEMM_OPTS_H */
