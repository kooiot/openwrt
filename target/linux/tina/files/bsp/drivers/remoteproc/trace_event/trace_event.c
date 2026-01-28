/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 *the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "include/trace_event_parser_cfg.h"

#ifdef TRACE_EVENT_PARSER_ON_LINUX_KERNEL
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <asm/cacheflush.h>

#define printf _printk
#else
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <awlog.h>
#include <stdlib.h>
#include <sunxi_hal_common.h>
#include <hal_mem.h>
#include <hal_thread.h>
#include <console.h>
#include <hal_atomic.h>
#endif

#include "include/trace_event.h"

#ifndef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
#define hal_malloc(x) NULL
#define hal_free(x)

#define hal_enter_critical() 0
#define hal_exit_critical(x)

/* Task states returned by eTaskGetState. */
typedef enum {
	eRunning = 0,     /* A task is querying the state of itself, so must be running. */
	eReady,           /* The task being queried is in a read or pending ready list. */
	eBlocked,         /* The task being queried is in the Blocked state. */
	eSuspended,       /* The task being queried is in the Suspended state, or is in the Blocked state with an infinite time out. */
	eDeleted,         /* The task being queried has been deleted, but its TCB has not yet been freed. */
	eInvalid          /* Used as an 'invalid state' value. */
} eTaskState;
#endif

//#define AW_TRACE_EVENT_DEBUG

#ifdef AW_TRACE_EVENT_DEBUG
#define TE_DEBUG_LOG(fmt, ...) printf("[%s:%d] "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define TE_DEBUG_LOG(fmt, ...)
#endif

#define TE_INFO_LOG(fmt, ...) printf("[%s:%d] "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)

#define ev_err(fmt, ...)	printf("[EVENT]" fmt, ##__VA_ARGS__)



#define events_get_prio(tcb) \
		({ \
			unsigned int prio; \
			if (ev_get_irq_nest()) \
				prio = uxTaskPriorityGetFromISR(tcb); \
			else \
				prio = uxTaskPriorityGet(tcb); \
			prio; \
		})

#define events_get_stat(tcb) \
		({ \
			eTaskState stat; \
			if (ev_get_irq_nest()) \
				stat = eRunning; \
			else \
				stat = eTaskGetState(tcb); \
			stat; \
		})

#define get_val(ev, mask, shift) \
		({ \
			unsigned long val; \
			val = (ev)->stat & (mask); \
			val >>= (shift); \
			val; \
		 })

#define get_arg_cnt(ev)			get_val(ev, ARGCNT_MASK, ARGCNT_SHIFT)

#define get_arg_sz(ev)			get_val(ev, ARGSZ_MASK, ARGSZ_SHIFT)

#define get_subsys(ev)			get_val(ev, SUBSYS_MASK, SUBSYS_SHIFT)

#define get_irq_nest(ev)		get_val(ev, NEST_MASK, NEST_SHIFT)

#define get_irqoff(ev)			get_val(ev, IRQOFF_MASK, IRQOFF_SHIFT)

#define get_event_type(ev)		get_val(ev, EV_TYPE_MASK, EV_TYPE_SHIFT)

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
#define event_base_sz(is_64bit_pointer) ((sizeof(os_event_t)) / sizeof(uint32_t))
#else
static inline uint32_t event_base_sz(int is_64bit_pointer)
{
	if (is_64bit_pointer)
		return (sizeof(os_event_64_t)) / sizeof(uint32_t);
	else
		return (sizeof(os_event_32_t)) / sizeof(uint32_t);
}
#endif

#define event_arg_sz(ev)		(get_arg_sz(ev))

#define get_task_pid(ev)		((ev)->pid)

#define get_task_tcb(ev)		pid2tcb((ev)->pid)

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
#define get_event_name(ev)		((ev)->name)
#define get_event_time(ev)		((ev)->time)
#define get_subsys_str(subsys_class_arr, ev)		(subsys_class_arr[get_subsys(ev)].name)
#else
#define get_event_name(ev)		((ev)->name_addr)
#define get_event_time(ev)		((ev)->time)
#define get_subsys_str(subsys_class_arr, ev)		(subsys_class_arr[get_subsys(ev)].name_addr)
#endif

#ifndef NATIVE_PLATFORM_CLEAN_DCACHE_REALTIME
#define WRITER_FLUSH_CACHE_SIZE 64
#endif

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM

#ifdef CONFIG_EVENTS_WRITER_DEFAULT_OVERWRITE_DIRECTLY
#define DEFAULT_TRACE_EVENT_WRITER_WORK_MODE WRITER_WORK_MODE_OVERWRITE_DIRECTLY
#else
#define DEFAULT_TRACE_EVENT_WRITER_WORK_MODE WRITER_WORK_MODE_DISCARD_NEW_DATA
#endif

#define MAX_EVENT_ARGS		(CONFIG_MAX_EVENT_ARGS)
#define MAX_BUFFER_SIZE		(CONFIG_MAX_EVENT_BUFFER_SIZE * 1024)
#define MAX_BUFFER_UNITS	(MAX_BUFFER_SIZE / (sizeof(uint32_t)))

int event_tracing;

#define MAX_TASK_NAME_CNT			CONFIG_MAX_TASK_NAME_CNT
#define MAX_TASK_NAME_LEN			CONFIG_MAX_TASK_NAME_LEN
struct pid_map {
	void *tcb;
	unsigned int prio;
	eTaskState stat;
	char name[MAX_TASK_NAME_LEN];
};

CACHE_LINE_ALIGN_ATTR static struct pid_map g_pid_maps[MAX_TASK_NAME_CNT] = { 0 };

#define GLOBAL_WRITER_INFO_SIZE CACHE_LINE_SIZE
#define GLOBAL_READER_CONTEXT_INFO_SIZE 128

CACHE_LINE_ALIGN_ATTR static uint8_t g_writer_info_mem[GLOBAL_WRITER_INFO_SIZE] = \
	{ DEFAULT_TRACE_EVENT_WRITER_WORK_MODE, 0, 0, 0 };

CACHE_LINE_ALIGN_ATTR static uint32_t g_events_buffer[MAX_BUFFER_UNITS];

__attribute__((__aligned__(GLOBAL_READER_CONTEXT_INFO_SIZE))) \
static uint8_t g_reader_context_info_mem[GLOBAL_READER_CONTEXT_INFO_SIZE];

const aw_trace_event_t g_aw_trace_event_obj = {
	.cfg = {
		.is_64bit_pointer = 0,
		.events_buf_len = MAX_BUFFER_SIZE,
		.max_task_name_cnt = MAX_TASK_NAME_CNT,
		.pid_map_type_size = sizeof(struct pid_map),
		.os_event_type_size = sizeof(os_event_t),
	},

	.addr32 = {
		.writer_info_addr = (uint32_t)&g_writer_info_mem,
		.reader_ctx_info_addr = (uint32_t)&g_reader_context_info_mem,
		.events_buf_addr = (uint32_t)&g_events_buffer,
		.subsys_class_addr = (uint32_t)&g_subsys_class,
		.pid_map_arr_addr = (uint32_t)&g_pid_maps,
	},

	.writer_info = (writer_info_t *)g_writer_info_mem,
	.events_buf = g_events_buffer,
	.reader_ctx_info = (reader_context_info_t *)g_reader_context_info_mem,
};

static aw_trace_event_parser_t g_aw_trace_event_parser = {
	.addr_map_func = NULL,
	.priv = NULL,
};

static uint8_t g_event_data_buf[256];
static reader_info_t g_reader_info = {
	.trace_event_obj = &g_aw_trace_event_obj,
	.single_event_pr_desc = {
		.print_buf = g_event_data_buf,
		.print_buf_len = sizeof(g_event_data_buf),
		.printf = NULL,
	},
};

static int tcb2pid(void *tcb)
{
	int i;

	for (i = 0; i < MAX_TASK_NAME_CNT; i++) {
		if (g_pid_maps[i].tcb == tcb)
			break;
		if (g_pid_maps[i].tcb == NULL) {
			const char *name = hal_thread_get_name(tcb);
			g_pid_maps[i].tcb = tcb;
			events_memcpy(g_pid_maps[i].name, name, MAX_TASK_NAME_LEN);
			g_pid_maps[i].name[MAX_TASK_NAME_LEN - 1] = '\0';
			g_pid_maps[i].prio = events_get_prio(tcb);
			g_pid_maps[i].stat = events_get_stat(tcb);

#ifdef NATIVE_PLATFORM_CLEAN_DCACHE_REALTIME
			unsigned long start_addr, end_addr, clean_size;
			start_addr = (unsigned long)&g_pid_maps[i];
			end_addr = start_addr + sizeof(g_pid_maps[i]) - 1;
			clean_size = CACHE_LINE_ALIGN_UP(end_addr)- CACHE_LINE_ALIGN_DOWN(start_addr);
			hal_dcache_clean(CACHE_LINE_ALIGN_DOWN(start_addr), clean_size);
#endif
			break;
		}
	}

	return i;
}

static inline void *pid2tcb(int pid)
{
	return g_pid_maps[pid].tcb;
}

static inline const char *get_task_name(int pid)
{
	return g_pid_maps[pid].name;
}

static inline unsigned int get_task_prio(int pid)
{
	return g_pid_maps[pid].prio;
}

static char get_tcb_state(int pid)
{
	eTaskState stat = g_pid_maps[pid].stat;
	switch (stat) {
	case eRunning:
		return 'R';
	case eReady:
		return 'R';
	case eBlocked:
		return 'S';
	case eSuspended:
		return 'D';
	case eDeleted:
		return 'D';
	default:
		return 'X';
	}
}
#else

#define MAX_TASK_NAME_LEN 0
typedef struct pid_map_32 {
	//void *tcb;
	uint32_t tcb_addr;
	unsigned int prio;
	eTaskState stat;
	char name[MAX_TASK_NAME_LEN];
} pid_map_32_t;

typedef struct pid_map_64 {
	//void *tcb;
	uint64_t tcb_addr;
	unsigned int prio;
	eTaskState stat;
	char name[MAX_TASK_NAME_LEN];
} pid_map_64_t;

static inline const char *get_task_name(int is_64bit_pointer, void *pid_maps_arr, uint32_t pid_map_type_size, int pid)
{
	pid_map_32_t *pid_map_32;
	pid_map_64_t *pid_map_64;

	pid_map_32 = (pid_map_32_t *)(pid_maps_arr + (pid_map_type_size * pid));

	if (is_64bit_pointer) {
		pid_map_64 = (pid_map_64_t *)pid_map_32;
		return pid_map_64->name;
	} else {
		return pid_map_32->name;
	}
}

static inline unsigned int get_task_prio(int is_64bit_pointer, void *pid_maps_arr, uint32_t pid_map_type_size, int pid)
{
	pid_map_32_t *pid_map_32;
	pid_map_64_t *pid_map_64;

	pid_map_32 = (pid_map_32_t *)(pid_maps_arr + (pid_map_type_size * pid));

	if (is_64bit_pointer) {
		pid_map_64 = (pid_map_64_t *)pid_map_32;
		return pid_map_64->prio;
	} else {
		return pid_map_32->prio;
	}
}

static char get_tcb_state(int is_64bit_pointer, void *pid_maps_arr, uint32_t pid_map_type_size, int pid)
{
	pid_map_32_t *pid_map_32;
	pid_map_64_t *pid_map_64;
	eTaskState stat;

	pid_map_32 = (pid_map_32_t *)(pid_maps_arr + (pid_map_type_size * pid));

	if (is_64bit_pointer) {
		pid_map_64 = (pid_map_64_t *)pid_map_32;
		stat =  pid_map_64->stat;
	} else {
		stat =  pid_map_32->stat;
	}

	switch (stat) {
	case eRunning:
		return 'R';
	case eReady:
		return 'R';
	case eBlocked:
		return 'S';
	case eSuspended:
		return 'D';
	case eDeleted:
		return 'D';
	default:
		return 'X';
	}
}
#endif

static void hexdump_event(int is_64bit_pointer, os_event_t *ev, user_print_func pr, void *user_priv_data)
{
	int i;
	uint32_t len;
	uint32_t *p = (uint32_t *)ev;

	len = event_base_sz(is_64bit_pointer) + event_arg_sz(ev);

	pr(user_priv_data, "hexdump event: %p, len=%u", p, len);

	p -= 16;
	len += 16;
	for (i = 0; i < len; i++) {
		uint32_t val = p[i];
		if ((i & 0x3) == 0)
			pr(user_priv_data, "\r\n0x%08lx: ", (long)(p + i));
		pr(user_priv_data, "0x%08lx ", (long)val);
	}
	pr(user_priv_data, "\r\n");
}

static int get_name_cnt(const char *from)
{
	// name formats: event_name:argN_type:argN_name:....:
	const char *p = from;
	int i = 0;

	while (*p != '\0') {
		if (*p == ':')
			i++;
		p++;
	}

	return i;
}

static int get_name_from_names(const char *from, int n, char *to, int max)
{
	// name formats: event_name:argN_type:argN_name:....:
	const char *p = from;
	const char *pp = from;
	int i = 0;

	if (!from) {
		ev_err("BUG: from is NULL, n=%d, to=%p, max=%d\r\n", n, to, max);
		return -EINVAL;
	}

	while (*pp != '\0') {
		if (*pp == ':') {
			if (i == n) {
				int len = (((unsigned long)pp) - ((unsigned long)p));
				memcpy(to, p, len > max ? max : len);
				to[len] = '\0';
				return 0;
			}
			p = pp + 1;
			i++;
		}
		pp++;
	}

	if (i == n) {
		int len = (((unsigned long)pp) - ((unsigned long)p));
		memcpy(to, p, len > max ? max : len);
		to[len] = '\0';
		return 0;
	}

	/* no found */
	return 1;
}

static void *get_arg_from_args(TRACE_EVENT_ARG_UNIT *args, int *ofs, int type)
{
	void *retp = &args[*ofs];
	switch (type) {
	case ARG_PRINT_U:
		*ofs += _count_arg_type_sz(ARG_PRINT_U, *(unsigned int *)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_X:
		*ofs += _count_arg_type_sz(ARG_PRINT_X, *(unsigned int *)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_D:
		*ofs += _count_arg_type_sz(ARG_PRINT_D, *(unsigned int *)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_C:
		*ofs += _count_arg_type_sz(ARG_PRINT_C, *(unsigned int *)(retp)) / sizeof(uint32_t);
		break;

	case ARG_PRINT_LD:
		*ofs += _count_arg_type_sz(ARG_PRINT_LD, *(unsigned long)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_LU:
		*ofs += _count_arg_type_sz(ARG_PRINT_LU, *(unsigned long)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_LX:
		*ofs += _count_arg_type_sz(ARG_PRINT_LX, *(unsigned long)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_P:
		*ofs += _count_arg_type_sz(ARG_PRINT_P, *(unsigned long)(retp)) / sizeof(uint32_t);
		break;
	case ARG_PRINT_STR:
		*ofs += _count_arg_type_sz(ARG_PRINT_STR, (const char *)(retp)) / sizeof(uint32_t);
		break;
	default:
		printf("Undown type %d ofs=%d ", type, *ofs);
		break;
	}

	return retp;
}

static void trace_event_dump_arg(user_print_func pr, void *user_priv_data, int type, const char *name, void *p, bool is_counter)
{
	char c = '=';
	if (is_counter)
		c = '|';

	//pr(user_priv_data, "val addr:%p\r\n", p);
	switch (type) {
	case ARG_PRINT_U: {
		unsigned int val = *(unsigned int *)p;
		pr(user_priv_data, "%s%c%u ", name, c, val);
	}
	break;
	case ARG_PRINT_LU: {
		unsigned long val = *(unsigned long *)p;
		pr(user_priv_data, "%s%c%lu ", name, c, val);
		break;
	}
	break;
	case ARG_PRINT_X: {
		unsigned int val = *(unsigned int *)p;
		pr(user_priv_data, "%s%c0x%x ", name, c, val);
	}
	break;
	case ARG_PRINT_LX: {
		unsigned long val = *(unsigned long *)p;
		pr(user_priv_data, "%s%c0x%08lx ", name, c, val);
	}
	break;
	case ARG_PRINT_D: {
		int val = *(int *)p;
		pr(user_priv_data, "%s%c%d ", name, c, val);
	}
	break;
	case ARG_PRINT_LD: {
		long val = *(long *)p;
		pr(user_priv_data, "%s%c%ld ", name, c, val);
	}
	break;
	case ARG_PRINT_C: {
		char val = *(char *)p;
		pr(user_priv_data, "%s%c%c ", name, c, (char)val);
	}
	break;
	case ARG_PRINT_STR: {
		const char *val = (const char *)p;
		if (strcmp(name, "func"))
			pr(user_priv_data, "%s%c%s ", name, c, val);
		else
			pr(user_priv_data, "%s ", val);
	}
	break;
	case ARG_PRINT_P:
		pr(user_priv_data, "%s%c%p ", name, c, p);
		break;
	default:
		pr(user_priv_data, "Undown type %d %s=%lx ", type, name, *(unsigned long *)p);
		break;
	}
}

static int trace_event_dump_one(const aw_trace_event_parser_t *parser,
								const aw_trace_event_t *trace_event_obj,
								os_event_t *ev, user_print_func pr, void *user_priv_data)
{
	static int last_pid = -1;
	const char *name;
	const char *subsys_str;
	int subsys;
	uint64_t time = get_event_time(ev);
	unsigned long pid;
	char name_buf[128];
	const int cpu = 0;  /* only support signal core */
	int arg_ofs = 0, ofs = 0;
	int i = 0;

	const char *task_name;
	unsigned int task_prio;
	char task_state;

	const aw_trace_event_cfg_t *cfg = &trace_event_obj->cfg;
	int is_64bit_pointer = cfg->is_64bit_pointer;

#ifndef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
	uint32_t pid_map_type_size;
	uint64_t subsys_name_addr;
	void *pid_map_arr;
	subsys_entry_32_t *subsys_class_32_arr;
	subsys_entry_64_t *subsys_class_64_arr;
	parser_addr_map_t addr_map_func = parser->addr_map_func;
#endif

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
	name = get_event_name(ev);
#else
	name = addr_map_func(parser, get_event_name(ev), 1);
#endif

	if (get_subsys(ev) > EV_NUM_SUBSYS) {
		pr(user_priv_data, "Invalid %s event subsys idex:%d, MAX:%d\r\n", name, get_subsys(ev), EV_NUM_SUBSYS);
		return -EINVAL;
	}

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
	subsys_str = get_subsys_str(g_subsys_class, ev);
#else
	if (is_64bit_pointer) {
		subsys_class_64_arr = (subsys_entry_64_t *)trace_event_obj->subsys_class_arr;
		subsys_name_addr = get_subsys_str(subsys_class_64_arr, ev);
	} else {
		subsys_class_32_arr = (subsys_entry_32_t *)trace_event_obj->subsys_class_arr;
		subsys_name_addr = get_subsys_str(subsys_class_32_arr, ev);
	}

	subsys_str = addr_map_func(parser, subsys_name_addr, 1);
#endif

	subsys = get_subsys(ev);
	pid = get_task_pid(ev);

	if (get_name_from_names(name, 0, name_buf, sizeof(name_buf)) != 0) {
		pr(user_priv_data, "can't get func name from event %p, %s, n=0\r\n", ev, name ? name : "NULL");
		hexdump_event(is_64bit_pointer, ev, pr, user_priv_data);
		return -EINVAL;
	}

	/* the first is func_name, one arg <-> 2 arg: type:name */
	if (get_name_cnt(name) != (get_arg_cnt(ev) * 2 + 1)) {
		pr(user_priv_data, "[%s]: Invalid ArgCnt:%d, Name Cnt:%d\r\n", name, get_arg_cnt(ev),
		   get_name_cnt(name));
		return -EINVAL;
	}

	if (subsys == EV_SCHE && name_buf[0] == 'o') {
		last_pid = pid;
		return 0;
	}

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
	task_name = get_task_name(pid);
#else
	pid_map_arr = trace_event_obj->pid_map_arr;
	pid_map_type_size = cfg->pid_map_type_size;
	task_name = get_task_name(is_64bit_pointer, pid_map_arr, pid_map_type_size, pid);
#endif

	pr(user_priv_data, "%16s-%lu  [%d] ", task_name, pid, cpu);
	pr(user_priv_data, "%c%d ", get_irqoff(ev) ? 'd':'.', get_irq_nest(ev));

#ifdef TRACE_EVENT_PARSER_ON_LINUX_KERNEL
	pr(user_priv_data, "%08ld:", time);
#else
#ifdef CONFIG_EVENTS_PRINT_TIME_FMT_LONG
	pr(user_priv_data, "%08ld:", time);
#endif
#ifdef CONFIG_EVENTS_PRINT_TIME_FMT_FLOAT
	pr(user_priv_data, "%0.6f:", (float)time / 1000000000.0F);
#endif
#ifdef CONFIG_EVENTS_PRINT_TIME_FMT_DOUBLE
	pr(user_priv_data, "%0.6lf:", (double)time / 1000000000.0);
#endif
#endif

	switch (get_subsys(ev)) {
	case EV_SCHE:
		if (name_buf[0] == 'i') {
			if (last_pid != -1) {
#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
				task_name = get_task_name(last_pid);
				task_prio = get_task_prio(last_pid);
#else
				task_name = get_task_name(is_64bit_pointer, pid_map_arr, pid_map_type_size, last_pid);
				task_prio = get_task_prio(is_64bit_pointer, pid_map_arr, pid_map_type_size, last_pid);
#endif
				pr(user_priv_data, " sched_switch prev: comm=%s pid=%lu prio=%u",
				   task_name, last_pid, task_prio);

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
				task_name = get_task_name(pid);
				task_prio = get_task_prio(pid);
				task_state = get_tcb_state(last_pid);
#else
				task_name = get_task_name(is_64bit_pointer, pid_map_arr, pid_map_type_size, pid);
				task_prio = get_task_prio(is_64bit_pointer, pid_map_arr, pid_map_type_size, pid);
				task_state = get_tcb_state(is_64bit_pointer, pid_map_arr, pid_map_type_size, last_pid);
#endif
				pr(user_priv_data, " state=%c ==> next: comm=%s pid=%lu prio=%u\r\n",
				   task_state, task_name, pid, task_prio);
			}
		}
		return 0;
	default:
		pr(user_priv_data, " tracing_mark_write:");
		break;
	}

	switch (get_event_type(ev)) {
	case EV_TYPE_BEGIN:
		pr(user_priv_data, " B|%lu|", pid);
		pr(user_priv_data, "%s: ", subsys_str);
		if (name_buf[0] != '\0')
			pr(user_priv_data, "%s ", name_buf);
		break;
	case EV_TYPE_END:
		pr(user_priv_data, " E|%lu|", pid);
		pr(user_priv_data, "%s: ", subsys_str);
		if (name_buf[0] != '\0')
			pr(user_priv_data, "%s ", name_buf);
		break;
	case EV_TYPE_MARK:
		pr(user_priv_data, " I|%lu|", pid);
		pr(user_priv_data, "%s: ", subsys_str);
		if (name_buf[0] != '\0')
			pr(user_priv_data, "%s ", name_buf);
		break;
	case EV_TYPE_CNT:
		pr(user_priv_data, " C|%u|", ev->args[0]);
		ofs = 1;
		break;
	default:
		break;
	}

	//pr(user_priv_data, "sys=%s ", subsys);
	//pr(user_priv_data, "ev full name: %s\r\n", ev->name);

	for (i = 0; i < get_arg_cnt(ev); i++) {
		int type;
		void *p;

		if (get_name_from_names(name, i * 2 + 1, name_buf, sizeof(name_buf)) != 0) {
			pr(user_priv_data, "can't get arg%d type from %s\r\n", i, name ? name : "NULL");
			hexdump_event(is_64bit_pointer, ev, pr, user_priv_data);
			return -EINVAL;
		}
		type = (int)(name_buf[0] - '0');
		//printf("type:%d\r\n", type);
		if (get_name_from_names(name, i * 2 + 2, name_buf, sizeof(name_buf)) != 0) {
			pr(user_priv_data, "can't get arg%d name from %s\r\n", i, name ? name : "NULL");
			hexdump_event(is_64bit_pointer, ev, pr, user_priv_data);
			return -EINVAL;
		}
		//printf("name:%s\r\n", name_buf);
		//printf("arg_ofs:%d\r\n", arg_ofs);
		p = get_arg_from_args(ev->args, &arg_ofs, type);

#ifndef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
		if (type == ARG_PRINT_P || type == ARG_PRINT_STR)
			p = addr_map_func(parser, (uint64_t)(uintptr_t)p, 1);
#endif

		if (i < ofs)
			continue;
		trace_event_dump_arg(pr, user_priv_data, type, name_buf, p, get_event_type(ev) == EV_TYPE_CNT);
	}
	pr(user_priv_data, "\r\n");

	return 0;
}

static inline int is_first_read(const reader_info_t *reader_info, writer_work_mode_t w_work_mode)
{
	if (reader_info->is_read_all)
		return 1;

	if (w_work_mode == WRITER_WORK_MODE_DISCARD_NEW_DATA) {
		if (!reader_info->read_pos && !reader_info->read_loop_cnt) {
			return 1;
		}
	} else {
		if (!reader_info->read_pos && !reader_info->last_event_pos
			&& !reader_info->last_event_timestamp) {
			//the first read when only read then new data
			return 1;
		}
	}

	return 0;
}

static int find_read_pos(const reader_info_t *reader_info, const writer_info_t *writer_info,
						 const uint32_t *events_buf, uint32_t max_buf_units,
						 uint32_t *current_read_pos, int *is_overwrite)
{
	uint32_t target_read_pos, read_pos, last_event_pos;
	uint32_t write_pos, write_loop_cnt;
	uint64_t last_event_timestamp;
	os_event_t *ev = NULL;
	writer_work_mode_t w_work_mode;

	write_pos = writer_info->write_pos;
	write_loop_cnt = writer_info->write_loop_cnt;

	if (reader_info->is_read_all) {
		//read all data
		if (write_loop_cnt == 0) {
			*current_read_pos = 0;
			return 0;
		}

		target_read_pos = write_pos;
		goto find_magic;
	}

	//The reader want only read the new data
	w_work_mode = writer_info->work_mode;
	read_pos = reader_info->read_pos;
	if (w_work_mode == WRITER_WORK_MODE_DISCARD_NEW_DATA) {
		if ((write_loop_cnt == reader_info->read_loop_cnt) && (read_pos == write_pos)) {
			//no new data
			return NEW_TRACE_EVENT_NOT_EXIST;
		}

		*current_read_pos = read_pos;
		return 0;
	}


	//overwrite directly mode
	last_event_pos = reader_info->last_event_pos;
	last_event_timestamp = reader_info->last_event_timestamp;

	if (!read_pos && !last_event_pos && !last_event_timestamp) {
		//the first read when only read then new data
		if (write_loop_cnt == 0) {
			*current_read_pos = 0;
			return 0;
		}

		target_read_pos = write_pos;
		goto find_magic;
	}

	ev = (os_event_t *)&events_buf[last_event_pos];
	if ((ev->magic == EVENT_MAGIC) && (get_event_time(ev) == last_event_timestamp)) {
		if (read_pos == write_pos) {
			//no new data
			return NEW_TRACE_EVENT_NOT_EXIST;
		}

		*current_read_pos = read_pos;
		return 0;
	}

	target_read_pos = write_pos;
	*is_overwrite = 1;

find_magic:
#define MAX_EVENT_UNIT_SIZE 256
	while (target_read_pos < (target_read_pos + MAX_EVENT_UNIT_SIZE)) {

		if (events_buf[target_read_pos] == EVENT_MAGIC)
			break;

		target_read_pos++;

		if (target_read_pos >= max_buf_units) {
			target_read_pos = 0;
			break;
		}
	}

	if (target_read_pos == (target_read_pos + MAX_EVENT_UNIT_SIZE))
		target_read_pos = 0;

	*current_read_pos = target_read_pos;
	return 0;
}

//static int is_event_has_been_flush_cache(const aw_trace_event_t *trace_event_obj,
static int is_event_has_been_flush_cache(int is_64bit_pointer,
		uint32_t current_event_pos, const uint32_t *events_buf,
		uint32_t write_loop_cnt, uint32_t read_loop_cnt,
		uint32_t next_flush_pos, uint32_t flush_cache_size)
{
//#ifndef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
#if 1
	uint32_t event_start_offset, event_end_offset, tmp_offset, next_flush_offset;
	os_event_t *ev;

	if (write_loop_cnt > read_loop_cnt)
		return 1;

	if (current_event_pos >= next_flush_pos) {
		return 0;
	}

	event_start_offset = current_event_pos * EVENTS_BUF_UINT_SIZE;
	next_flush_offset = next_flush_pos * EVENTS_BUF_UINT_SIZE;
	tmp_offset = event_start_offset + offsetof(os_event_t, stat);
	if (tmp_offset >= next_flush_offset) {
		return 0;
	}

	ev = (os_event_t *)&events_buf[current_event_pos];
	event_end_offset = event_start_offset + event_base_sz(is_64bit_pointer) + event_arg_sz(ev) - 1;

	if (event_end_offset >= next_flush_offset) {
		return 0;
	}
#endif

	return 1;
}

static int get_events_buf_info(const aw_trace_event_parser_t *parser,
							   const aw_trace_event_t *trace_event_obj,
							   const uint32_t **events_buf, uint32_t *events_buf_len,
							   const writer_info_t **w_info)
{
	*w_info = trace_event_obj->writer_info;
	*events_buf = trace_event_obj->events_buf;
#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
	*events_buf_len = MAX_BUFFER_SIZE;
#else
	*events_buf_len = trace_event_obj->cfg.events_buf_len;
#endif
	return 0;
}

static int default_user_print_func(void *priv, const char *fmt, ...)
{
	va_list args;

	int ret;
	uint8_t *current_buf;
	uint32_t buf_len;
	user_print_desc_t *pr_desc;

	pr_desc = priv;

	if (!pr_desc) {
		printf("the priv data of user print is NULL!\n");
		return -1;
	}

	TE_DEBUG_LOG("print buf: %px, len: %u, w_index: %u, printf_func: %px",
				 pr_desc->print_buf, pr_desc->print_buf_len, pr_desc->print_buf_write_index,
				 pr_desc->printf);

	if (pr_desc->printf) {
		current_buf = pr_desc->print_buf;
		buf_len = pr_desc->print_buf_len;

		va_start(args, fmt);
		ret = vsnprintf((char *)current_buf, buf_len, fmt, args);
		va_end(args);

		if (ret >= buf_len) {
			ev_err("tmp print buf is too small! real: %u, need: %d",
				   buf_len, ret);
		}

		ret = pr_desc->printf("%s", current_buf);

		pr_desc->print_buf_write_index += ret;
		TE_DEBUG_LOG("printf_func: %px, printf: %px, fmt: '%s', ret: %d",
					 pr_desc->printf, printf, fmt, ret);
	} else {
		uint32_t buf_total_len, buf_write_index;

		buf_total_len = pr_desc->print_buf_len;
		buf_write_index = pr_desc->print_buf_write_index;

		if (buf_write_index >= buf_total_len)
			return -1;

		current_buf = &pr_desc->print_buf[buf_write_index];
		buf_len = buf_total_len - buf_write_index;

		TE_DEBUG_LOG("current_buf: %px, buf_len: %u, fmt: '%s'",
					 current_buf, buf_len, fmt);

		va_start(args, fmt);
		ret = vsnprintf((char *)current_buf, buf_len, fmt, args);
		va_end(args);

		TE_DEBUG_LOG("vsnprintf ret: %d", ret);

		if (ret < 0) {
			return ret;
		}

		if (ret >= buf_len) {
			/* string is truncated, remove the truncated string */
			*current_buf  = '\0';

			/* using the BUF_SPACE_NOT_ENOUGH as ret code to indicate this situation. */
			ret = BUF_SPACE_NOT_ENOUGH;
			return ret;
		}

		buf_write_index += ret;
		pr_desc->print_buf_write_index = buf_write_index;
	}

	return ret;
}

static inline int check_input_para(const aw_trace_event_parser_t *parser,
						reader_info_t *reader_info)
{
	TE_DEBUG_LOG("r_info: %px, read_all: %d, read_to_buf: %d, panic: %d, obj: %px, user_pr_func: %px, user_pr_priv: %px",
				 reader_info, reader_info->is_read_all, reader_info->is_read_to_buf, reader_info->is_panic_when_read,
				 reader_info->trace_event_obj, reader_info->user_pr_func, reader_info->user_pr_priv_data);

	TE_DEBUG_LOG("print_buf: %px, buf_len: %d, index: %d, printf: %px",
				 reader_info->single_event_pr_desc.print_buf,
				 reader_info->single_event_pr_desc.print_buf_len,
				 reader_info->single_event_pr_desc.print_buf_write_index,
				 reader_info->single_event_pr_desc.printf);

	if (!parser || !reader_info || !reader_info->trace_event_obj
		|| !reader_info->user_pr_func
		|| !reader_info->single_event_pr_desc.print_buf
		|| !reader_info->single_event_pr_desc.print_buf_len) {
		return -EINVAL;
	}

	TE_DEBUG_LOG("reader info: rpos=%u loop=%u last_event_pos=%u last_event_ts=%llu",
		reader_info->read_pos, reader_info->read_loop_cnt,
		reader_info->last_event_pos, reader_info->last_event_timestamp);

#ifndef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
	if (!parser->addr_map_func)
		return -EINVAL;
#endif
	return 0;
}

static inline void load_reader_ctx_info_from_shared_mem(reader_info_t *reader_info)
{
	const aw_trace_event_t *te_obj;
	te_obj = reader_info->trace_event_obj;

//#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
//#endif
	reader_info->read_pos = te_obj->reader_ctx_info->read_pos;
	reader_info->read_loop_cnt = te_obj->reader_ctx_info->read_loop_cnt;
}

static inline void update_reader_info(reader_info_t *reader_info,
	uint32_t current_rpos, uint32_t current_read_loop_cnt,
	uint32_t last_event_pos, uint64_t last_event_timestamp)
{
	if (reader_info->is_read_all) {
		return;
	}

	TE_DEBUG_LOG("old reader info: rpos=%u loop=%u last_event_pos=%u last_event_ts=%llu",
		reader_info->read_pos, reader_info->read_loop_cnt,
		reader_info->last_event_pos, reader_info->last_event_timestamp);

	reader_info->read_pos = current_rpos;
	reader_info->read_loop_cnt = current_read_loop_cnt;

	reader_info->last_event_pos = last_event_pos;
	reader_info->last_event_timestamp = last_event_timestamp;

	TE_DEBUG_LOG("new reader info: rpos=%u loop=%u last_event_pos=%u last_event_ts=%llu",
		reader_info->read_pos, reader_info->read_loop_cnt,
		reader_info->last_event_pos, reader_info->last_event_timestamp);

}

static inline void sync_reader_ctx_info_to_shared_mem(reader_info_t *reader_info)
{
	const aw_trace_event_t *te_obj;
	te_obj = reader_info->trace_event_obj;

	te_obj->reader_ctx_info->read_pos = reader_info->read_pos;
	te_obj->reader_ctx_info->read_loop_cnt = reader_info->read_loop_cnt;

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
	//hal_dcache_clean();
#endif
}

int aw_trace_event_read(const aw_trace_event_parser_t *parser,
						reader_info_t *reader_info)
{
	int ret = 0, is_panic_when_read, is_read_to_buf, is_read_all, is_overwrite = 0, no_new_event;
	__attribute__((__unused__)) unsigned long flags;

	const uint32_t *orig_events_buf;
	const uint32_t *events_buf = NULL;
	const writer_info_t *w_info_ptr;
	writer_info_t current_w_info;

	uint64_t last_event_timestamp;
	uint32_t current_rpos = 0, read_loop_cnt = 0, last_event_pos;
	uint32_t current_wpos, write_loop_cnt, events_buf_len, max_buf_units;

	os_event_t *ev = NULL;
	const aw_trace_event_t *trace_event_obj;
	user_print_func pr = NULL;
	void *user_priv_data;
	user_print_desc_t *pr_desc;
	writer_work_mode_t w_work_mode;

	ret = check_input_para(parser, reader_info);
	if (ret)
		return -1;

	trace_event_obj = reader_info->trace_event_obj;
	ret = get_events_buf_info(parser, trace_event_obj, &orig_events_buf, &events_buf_len, &w_info_ptr);
	if (ret) {
		printf("get events buf info failed, ret: %d\n", ret);
		return ret;
	}

	is_panic_when_read = reader_info->is_panic_when_read;

#ifdef TRACE_EVENT_PARSER_ON_LINUX_KERNEL
	events_buf = orig_events_buf;
#else
	if (is_panic_when_read) {
		events_buf = orig_events_buf;
	} else {
		events_buf = hal_malloc(events_buf_len);
		flags = hal_enter_critical();
		if (!events_buf) { /* out of memory? */
			events_buf = orig_events_buf;
		} else {
			memcpy((uint32_t *)events_buf, orig_events_buf, events_buf_len);
			hal_exit_critical(flags);
		}
	}
#endif

	current_w_info = *w_info_ptr;
	w_work_mode = current_w_info.work_mode;
	current_wpos = current_w_info.write_pos;
	write_loop_cnt = current_w_info.write_loop_cnt;

	max_buf_units = events_buf_len / sizeof(uint32_t);

	TE_DEBUG_LOG("parser: %px, obj: %px, is_panic: %d",
				 parser, trace_event_obj, is_panic_when_read);

	TE_DEBUG_LOG("orig_events_buf: %px, events_buf(using): %px, events_buf_len: %u, max_buf_units: %u",
				 orig_events_buf, events_buf, events_buf_len, max_buf_units);

	TE_DEBUG_LOG("current_wpos: %u, write_loop_cnt: %u, next_flush_pos: %u",
		current_wpos, write_loop_cnt, current_w_info.next_flush_pos);

	if (w_work_mode == WRITER_WORK_MODE_DISCARD_NEW_DATA)
		load_reader_ctx_info_from_shared_mem(reader_info);

	no_new_event = 0;
	current_rpos = NEW_TRACE_EVENT_NOT_EXIST;
	ret = find_read_pos(reader_info, &current_w_info, events_buf, max_buf_units,
						&current_rpos, &is_overwrite);
	if (ret) {
		no_new_event = 1;
	}

	is_read_all = reader_info->is_read_all;
	is_read_to_buf = reader_info->is_read_to_buf;
	pr = reader_info->user_pr_func;
	user_priv_data = reader_info->user_pr_priv_data;
	read_loop_cnt = reader_info->read_loop_cnt;

	if (is_panic_when_read) {
		pr(user_priv_data, "write_pos = %u\r\n", current_wpos);
		pr(user_priv_data, "read_pos = %u\r\n", current_rpos);
	}

	if (no_new_event
		|| ((current_rpos < max_buf_units) && (events_buf[current_rpos] != EVENT_MAGIC))) {
		if (!is_read_to_buf) {
			if (is_read_all)
				pr(user_priv_data, "Not Any Events\r\n");
			else
				pr(user_priv_data, "Not Any New Events\r\n");
		}

#ifdef AW_TRACE_EVENT_DEBUG
		uint32_t tmp_magic = 0;
		if (current_rpos != NEW_TRACE_EVENT_NOT_EXIST)
			tmp_magic = events_buf[current_rpos];

		printf("Not events, magic: 0x%08x, target read pos: %u, reader: pos=%u, loop=%u, writer: pos=%u, loop=%u\n",
			tmp_magic, current_rpos, reader_info->read_pos, read_loop_cnt, current_wpos, write_loop_cnt);
#endif
		ret = 0;
		goto exit_without_update_reader_info;
	}

	//check rpos
	if (current_rpos >= max_buf_units) {
		pr(user_priv_data, "Error! read_pos(%u) is greater than max buf units(%u)\r\n",
			current_rpos, max_buf_units);
		ret = 0;
		goto exit_without_update_reader_info;
	}

	if (w_work_mode == WRITER_WORK_MODE_OVERWRITE_DIRECTLY) {
		if (is_overwrite) {
			if (write_loop_cnt <= read_loop_cnt) {
				pr(user_priv_data, "abnormal read loop cnt! reader: pos=%u, loop=%u, "
					"writer: pos=%u, loop=%u\n",
					reader_info->read_pos, read_loop_cnt, current_wpos, write_loop_cnt);

				reader_info->read_pos = 0;
				reader_info->read_loop_cnt = 0;
				reader_info->last_event_pos = 0;
				reader_info->last_event_timestamp = 0;
				goto exit_without_update_reader_info;
			}

			pr(user_priv_data, "trace event buffer overrite! size: %u word\n",
				current_wpos + (max_buf_units - reader_info->read_pos)
				+ (write_loop_cnt - read_loop_cnt - 1) * max_buf_units);

			pr(user_priv_data, "max_units: %u, target read pos: %u, reader: pos=%u, loop=%u, writer: pos=%u, loop=%u\n",
				max_buf_units, current_rpos,
				reader_info->read_pos, read_loop_cnt, current_wpos, write_loop_cnt);

			read_loop_cnt = write_loop_cnt;
		}
	}

	TE_DEBUG_LOG("init read_pos: %u", current_rpos);

	if ((w_work_mode == WRITER_WORK_MODE_DISCARD_NEW_DATA)
		&& !is_event_has_been_flush_cache(trace_event_obj->cfg.is_64bit_pointer,
		current_rpos, events_buf, write_loop_cnt, read_loop_cnt,
		current_w_info.next_flush_pos,
		WRITER_FLUSH_CACHE_SIZE)) {
		ret = 0;

#ifdef AW_TRACE_EVENT_DEBUG
		printf("not flush cache, target read pos: %u, reader: pos=%u, loop=%u, writer: pos=%u, loop=%u\n",
			current_rpos, reader_info->read_pos, read_loop_cnt, current_wpos, write_loop_cnt);
#endif
		goto exit_without_update_reader_info;
	}

	if (is_first_read(reader_info, w_work_mode)) {
		pr(user_priv_data, "# tracer: nop\r\n#\r\n");
		pr(user_priv_data, "#                        _---=> irqs-off (.: irq enable, d:irq disabled)\r\n");
		pr(user_priv_data, "#                       /  _--=> preempt-depth\r\n");
		pr(user_priv_data, "#                       | /\r\n");
		pr(user_priv_data, "#       TASK-PID   CPU# || TIMESTAMP\r\n");
		pr(user_priv_data, "#          | |       |  ||     |\r\n");
	}

	pr_desc = &reader_info->single_event_pr_desc;
	while (1) {
		TE_DEBUG_LOG("current read_pos: %u", current_rpos);
		last_event_pos = current_rpos;
		ev = (os_event_t *)(&events_buf[current_rpos]);
		last_event_timestamp = get_event_time(ev);

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
		TE_DEBUG_LOG("ev: %p, ev->name, addr: %p, value: %p",
					 ev, &ev->name, ev->name);
#endif

		if (is_read_to_buf) {
			pr_desc->print_buf_write_index = 0;
			trace_event_dump_one(parser, trace_event_obj, ev, default_user_print_func, pr_desc);
			if (pr_desc->print_buf_write_index) {
				ret = pr(user_priv_data, "%s", pr_desc->print_buf);
				if (ret < 0) {
					ev = (os_event_t *)(&events_buf[last_event_pos]);
					if (ret == BUF_SPACE_NOT_ENOUGH) {
						ret = 0;
					}
					goto exit;
				}
				ret = 0;
			}

		} else {
			trace_event_dump_one(parser, trace_event_obj, ev, pr, user_priv_data);
		}

		/* find next event */
		current_rpos += event_base_sz(trace_event_obj->cfg.is_64bit_pointer) + event_arg_sz(ev);

		if (current_rpos == current_wpos)
			break;

		if ((w_work_mode == WRITER_WORK_MODE_DISCARD_NEW_DATA)
			&& !is_event_has_been_flush_cache(trace_event_obj->cfg.is_64bit_pointer,
			current_rpos, events_buf, write_loop_cnt, read_loop_cnt,
			current_w_info.next_flush_pos,
			WRITER_FLUSH_CACHE_SIZE)) {
			ret = 0;
			break;
		}

		if (events_buf[current_rpos] != EVENT_MAGIC) {
			TE_DEBUG_LOG("read_pos: %u, current_wpos: %u, data: 0x%08x",
						 current_rpos, current_wpos, events_buf[current_rpos]);

			if (events_buf[current_rpos] == ~(EVENT_MAGIC)) {
				/* is the unused, skip it */
				current_rpos = 0;
				read_loop_cnt++;
				if (w_work_mode == WRITER_WORK_MODE_DISCARD_NEW_DATA) {
					ret = 0;
					break;
				}

				if (events_buf[current_rpos] == EVENT_MAGIC)
					continue;
			}

			ret = pr(user_priv_data, "Invalid Event Format, idx:%zu, data: 0x%08x\r\n",
					current_rpos * sizeof(uint32_t), events_buf[current_rpos]);
			if (ret < 0)
				printf("Invalid Event Format, idx:%zu, data: 0x%08x\r\n",
					   current_rpos * sizeof(uint32_t), events_buf[current_rpos]);

			ret = -1;
			goto exit;
		}
	}

exit:
	update_reader_info(reader_info, current_rpos, read_loop_cnt, last_event_pos, last_event_timestamp);
	if (w_work_mode == WRITER_WORK_MODE_DISCARD_NEW_DATA)
		sync_reader_ctx_info_to_shared_mem(reader_info);

exit_without_update_reader_info:
	if (!is_panic_when_read) {
		if (events_buf == orig_events_buf)
			hal_exit_critical(flags);
		else
			hal_free((uint32_t *)events_buf);
	}

	return ret;
}

int aw_trace_event_read_to_buf(aw_trace_event_parser_t *parser,
							   reader_info_t *reader_info, void *buf, uint32_t buf_len)
{
	int ret;
	user_print_desc_t print_desc;

	if (!reader_info || !buf || !buf_len) {
		return -EINVAL;
	}

	memset(&print_desc, 0, sizeof(print_desc));
	print_desc.print_buf = buf;
	print_desc.print_buf_len = buf_len;

	reader_info->user_pr_func = default_user_print_func;
	reader_info->user_pr_priv_data = &print_desc;

	reader_info->is_read_to_buf = 1;
	ret = aw_trace_event_read(parser, reader_info);
	reader_info->read_data_len = print_desc.print_buf_write_index;

	return ret;
}

int aw_trace_event_read_all_to_buf(aw_trace_event_parser_t *parser,
								   const aw_trace_event_t *trace_event_obj, int is_panic_read, void *buf, uint32_t buf_len)
{
	reader_info_t *r_info;
#if defined(TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM)
	r_info = &g_reader_info;
#elif defined(TRACE_EVENT_PARSER_ON_LINUX_KERNEL)
	r_info = &parser->reader_info;
#else
#error "please provide a global reader_info_t objdect!"
#endif

	r_info->is_read_all = 1;
	r_info->is_panic_when_read = is_panic_read;
	r_info->trace_event_obj = trace_event_obj;
	return aw_trace_event_read_to_buf(parser, r_info, buf, buf_len);
}

int aw_trace_event_dump(aw_trace_event_parser_t *parser,
						reader_info_t *reader_info, event_printf_t event_pr_func)
{
	int ret;
	user_print_desc_t print_desc;

	if (!reader_info || !event_pr_func) {
		return -EINVAL;
	}

	memset(&print_desc, 0, sizeof(print_desc));
	print_desc.printf = event_pr_func;

#ifdef TRACE_EVENT_PARSER_ON_LINUX_KERNEL
	print_desc.print_buf = parser->kernel_buf;
	print_desc.print_buf_len = sizeof(parser->kernel_buf);
#else
	/* using the single event print buf when don't read to buf */
	print_desc.print_buf = reader_info->single_event_pr_desc.print_buf;
	print_desc.print_buf_len = reader_info->single_event_pr_desc.print_buf_len;
#endif

	reader_info->user_pr_func = default_user_print_func;
	reader_info->user_pr_priv_data = &print_desc;

#ifdef TRACE_EVENT_PARSER_ON_LINUX_KERNEL
	reader_info->is_read_to_buf = 1;
#else
	reader_info->is_read_to_buf = 0;
#endif

	ret = aw_trace_event_read(parser, reader_info);
	reader_info->read_data_len = print_desc.print_buf_write_index;

	return ret;

}

int aw_trace_event_dump_all(aw_trace_event_parser_t *parser,
							const aw_trace_event_t *trace_event_obj, int is_panic_dump, event_printf_t event_pr_func)
{
	reader_info_t *r_info;
#if defined(TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM)
	r_info = &g_reader_info;
#elif defined(TRACE_EVENT_PARSER_ON_LINUX_KERNEL)
	r_info = &parser->reader_info;
#else
#error "please provide a global reader_info_t objdect!"
#endif

	r_info->is_read_all = 1;
	r_info->is_panic_when_read = is_panic_dump;
	r_info->trace_event_obj = trace_event_obj;
	return aw_trace_event_dump(parser, r_info, event_pr_func);
}

int aw_trace_event_parser_init(aw_trace_event_parser_t *parser, parser_addr_map_t map_func, void *priv)
{
	if (!parser)
		return -EINVAL;

#ifndef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
	if (!map_func)
		return -EINVAL;

	parser->addr_map_func = map_func;
	parser->priv = priv;
#endif

#ifdef TRACE_EVENT_PARSER_ON_LINUX_KERNEL
	memset(&parser->reader_info, 0, sizeof(parser->reader_info));
	memset(&parser->trace_event, 0, sizeof(parser->trace_event));

	parser->reader_info.single_event_pr_desc.print_buf = parser->event_data_buf;
	parser->reader_info.single_event_pr_desc.print_buf_len = sizeof(parser->event_data_buf);
	parser->reader_info.single_event_pr_desc.printf = NULL;
#endif

	TE_DEBUG_LOG("init aw trace event parser(%px), addr_map: %px, priv: %px",
				 parser, parser->addr_map_func, parser->priv);
	return 0;
}

int aw_trace_event_obj_init(aw_trace_event_t *obj, const aw_trace_event_parser_t *parser, const aw_trace_event_t *native_obj)
{
#ifndef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
	int is_64bit_pointer;
	void *tmp;
	uint64_t events_buf_addr, writer_info_addr, r_ctx_info_addr, subsys_class_arr_addr, pid_map_arr_addr;
	const aw_trace_event_cfg_t *cfg;
	writer_info_t *writer_info;
	reader_context_info_t *r_ctx_info;
	parser_addr_map_t addr_map_func;
#endif

	if (!obj || !parser || !native_obj)
		return -EINVAL;

#ifndef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
	if (!parser->addr_map_func)
		return -EINVAL;

	/* only copy data before events_buf, because the size of pointer is
	 * perhaps different between native platform and non native platform. */
	memcpy(obj, native_obj, offsetof(aw_trace_event_t, events_buf));

	cfg = &obj->cfg;
	is_64bit_pointer = obj->cfg.is_64bit_pointer;

	addr_map_func = parser->addr_map_func;
	if (is_64bit_pointer) {
		events_buf_addr = obj->addr64.events_buf_addr;
		writer_info_addr = obj->addr64.writer_info_addr;
		r_ctx_info_addr = obj->addr64.reader_ctx_info_addr;
		subsys_class_arr_addr = obj->addr64.subsys_class_addr;
		pid_map_arr_addr = obj->addr64.pid_map_arr_addr;
	} else {
		events_buf_addr = obj->addr32.events_buf_addr;
		writer_info_addr = obj->addr32.writer_info_addr;
		r_ctx_info_addr = obj->addr32.reader_ctx_info_addr;
		subsys_class_arr_addr = obj->addr32.subsys_class_addr;
		pid_map_arr_addr = obj->addr32.pid_map_arr_addr;
	}

#if 0
	printf("native obj: %px\n", native_obj);
	aw_trace_event_info_dump(native_obj);
	printf("not native obj:\n");
	aw_trace_event_info_dump(obj);
#endif

	tmp = addr_map_func(parser, events_buf_addr, 1);
	if (!tmp)
		return -1;

	obj->events_buf = tmp;

	writer_info = addr_map_func(parser, writer_info_addr, 1);
	if (!writer_info)
		return -2;

	obj->writer_info = writer_info;

	r_ctx_info = addr_map_func(parser, r_ctx_info_addr, 1);
	if (!r_ctx_info)
		return -3;

	obj->reader_ctx_info = r_ctx_info;

	tmp = addr_map_func(parser, subsys_class_arr_addr, 1);
	if (!tmp)
		return -1;

	obj->subsys_class_arr = tmp;

	tmp = addr_map_func(parser, pid_map_arr_addr, 1);
	if (!tmp)
		return -1;

	obj->pid_map_arr = tmp;
#endif

	//TE_DEBUG_LOG("init aw trace event object(%px), addr_map: %px, priv: %px",
	//			 obj, parser->addr_map_func, parser->priv);

#if 0
	aw_trace_event_info_dump(obj);
#endif
	return 0;
}

int aw_trace_event_info_dump(const aw_trace_event_t *obj)
{
	const aw_trace_event_cfg_t *cfg = &obj->cfg;
	const aw_trace_event_addr_info_32_t *addr32;
	const aw_trace_event_addr_info_64_t *addr64;

	printf("64bit pointer: %d\n", cfg->is_64bit_pointer);
	printf("events_buf_len: %u, max_task_name_cnt: %u\n", cfg->events_buf_len, cfg->max_task_name_cnt);
	printf("type size: pid_map=%u, os_event=%u\n", cfg->pid_map_type_size, cfg->os_event_type_size);

	if (cfg->is_64bit_pointer) {
		addr64 = &obj->addr64;
		printf("events_buf_addr: 0x%016llx\n", addr64->events_buf_addr);
		printf("writer_info_addr: 0x%016llx\n", addr64->writer_info_addr);
		printf("subsys_class_addr: 0x%016llx\n", addr64->subsys_class_addr);
		printf("pid_map_arr_addr: 0x%016llx\n", addr64->pid_map_arr_addr);
	} else {
		addr32 = &obj->addr32;
		printf("events_buf_addr: 0x%08x\n", addr32->events_buf_addr);
		printf("writer_info_addr: 0x%08x\n", addr32->writer_info_addr);
		printf("subsys_class_addr: 0x%08x\n", addr32->subsys_class_addr);
		printf("pid_map_arr_addr: 0x%08x\n", addr32->pid_map_arr_addr);
	}

#ifdef TRACE_EVENT_PARSER_ON_LINUX_KERNEL
	printf("events_buf: %px\n", obj->events_buf);
	printf("writer_info: %px\n", obj->writer_info);
	printf("reader_ctx_info: %px\n", obj->reader_ctx_info);
	printf("subsys_class_arr: %px\n", obj->subsys_class_arr);
	printf("pid_map_arr: %px\n", obj->pid_map_arr);
#else
	printf("events_buf: %p\n", obj->events_buf);
	printf("writer_info: %p\n", obj->writer_info);
	printf("reader_ctx_info: %p\n", obj->reader_ctx_info);
	printf("subsys_class_arr: %p\n", obj->subsys_class_arr);
	printf("pid_map_arr: %p\n", obj->pid_map_arr);
#endif
	return 0;
}


#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM

int dump_trace_event_info(int argc, char **argv)
{
	const writer_info_t *w_info;
	const reader_info_t *r_info;
	const reader_context_info_t *r_ctx_info;

	w_info = g_aw_trace_event_obj.writer_info;
	r_info = &g_reader_info;
	r_ctx_info = g_aw_trace_event_obj.reader_ctx_info;

	hal_dcache_invalidate((unsigned long)r_ctx_info, GLOBAL_READER_CONTEXT_INFO_SIZE);

	printf("reader info: pos=%u, loop=%u, last_ev_pos=%u, last_ev_ts=%llu\n",
		r_info->read_pos, r_info->read_loop_cnt,
		r_info->last_event_pos, r_info->last_event_timestamp);

	printf("global reader context info: pos=%u, loop=%u\n",
		r_ctx_info->read_pos, r_ctx_info->read_loop_cnt);

	printf("global writer: work_mode=%d, pos=%u, loop=%u, "
		"is_discarding=%u, discard_ev_cnt=%u, discard_ev_size=%u Words\n",
		w_info->work_mode, w_info->write_pos, w_info->write_loop_cnt,
		w_info->is_discarding, w_info->discard_event_cnt, w_info->discard_data_size);

	printf("flush cache info: next_flush_pos=%u, next_flush_offset=%u\n",
		w_info->next_flush_pos, w_info->next_flush_pos * EVENTS_BUF_UINT_SIZE);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(dump_trace_event_info, te_ctx_info, debug aw trace event);

#ifndef NATIVE_PLATFORM_CLEAN_DCACHE_REALTIME
#include <osal/hal_time.h>
#define DEFAULT_WAIT_TIME_TO_CLEAN_DCACHE 500
void trace_event_clean_dcache(void)
{
	const aw_trace_event_t *trace_event_obj;
	trace_event_obj = &g_aw_trace_event_obj;

	//pid_maps
	unsigned long start_addr, end_addr, clean_size;
	start_addr = (unsigned long)g_pid_maps;
	end_addr = start_addr + sizeof(g_pid_maps) - 1;
	clean_size = CACHE_LINE_ALIGN_UP(end_addr)- CACHE_LINE_ALIGN_DOWN(start_addr);
	start_addr = CACHE_LINE_ALIGN_DOWN(start_addr);
	TE_INFO_LOG("pid maps: start=0x%08lx, size=0x%lx(%lu)", start_addr, clean_size, clean_size);
	hal_dcache_clean(start_addr, clean_size);

	//trace event write pos
	start_addr = (unsigned long)trace_event_obj->writer_info;
	end_addr = start_addr + sizeof(*trace_event_obj->writer_info) - 1;
	clean_size = CACHE_LINE_ALIGN_UP(end_addr)- CACHE_LINE_ALIGN_DOWN(start_addr);
	start_addr = CACHE_LINE_ALIGN_DOWN(start_addr);
	TE_INFO_LOG("writer info: start=0x%08lx, size=0x%lx(%lu)", start_addr, clean_size, clean_size);
	hal_dcache_clean(start_addr, clean_size);

	//trace event buffer
	start_addr = (unsigned long)trace_event_obj->events_buf;
	end_addr = start_addr + MAX_BUFFER_SIZE - 1;
	clean_size = CACHE_LINE_ALIGN_UP(end_addr)- CACHE_LINE_ALIGN_DOWN(start_addr);
	start_addr = CACHE_LINE_ALIGN_DOWN(start_addr);
	TE_INFO_LOG("events buf: start=0x%08lx, size=0x%lx(%lu)", start_addr, clean_size, clean_size);
	hal_dcache_clean(start_addr, clean_size);

	/* wait 500ms to confirm */
	TE_INFO_LOG("wait %ums to confirm cache write back!", DEFAULT_WAIT_TIME_TO_CLEAN_DCACHE);
	hal_msleep(DEFAULT_WAIT_TIME_TO_CLEAN_DCACHE);
	TE_INFO_LOG("trace event clean dcache success!");
}

void aw_trace_event_clean_dcache_conditional(const aw_trace_event_t *te_obj, const os_event_t *event, unsigned long arg_size, int is_warp_around)
{
	unsigned long events_buf_addr, event_end_offset, flush_start_addr, flush_size;
	writer_info_t *w_info;
	uint32_t next_flush_offset;

	w_info = te_obj->writer_info;

	if (w_info->work_mode == WRITER_WORK_MODE_OVERWRITE_DIRECTLY)
		return;

	events_buf_addr = (unsigned long)te_obj->events_buf;
	next_flush_offset = w_info->next_flush_pos * EVENTS_BUF_UINT_SIZE;
	flush_start_addr = events_buf_addr + next_flush_offset;

	if (is_warp_around) {
		/* If warp around occur, the tail space is writed certainly, so need to flush cache */
		if (next_flush_offset >= te_obj->cfg.events_buf_len) {
			printf("Error when warp around! next_flush_offset: %u, events_buf_len: %u",
				next_flush_offset, te_obj->cfg.events_buf_len);
			return;
		}
		flush_size = te_obj->cfg.events_buf_len - next_flush_offset;
		hal_dcache_clean(CACHE_LINE_ALIGN_DOWN(flush_start_addr), CACHE_LINE_ALIGN_UP(flush_size));
		w_info->next_flush_pos = 0;
	} else {
		if (!event)
			return;

		event_end_offset = (unsigned long)event + event_byte_size(arg_size) - 1 - events_buf_addr;
		if (event_end_offset < (next_flush_offset + WRITER_FLUSH_CACHE_SIZE - 1)) {
			return;
		}

		flush_size = WRITER_FLUSH_CACHE_SIZE;
		hal_dcache_clean(CACHE_LINE_ALIGN_DOWN(flush_start_addr), flush_size);
		w_info->next_flush_pos += flush_size / EVENTS_BUF_UINT_SIZE;
		next_flush_offset += flush_size;
		if (next_flush_offset >= te_obj->cfg.events_buf_len) {
			printf("Error! next_flush_offset: %u, events_buf_len: %u",
				next_flush_offset, te_obj->cfg.events_buf_len);
			w_info->next_flush_pos = 0;
		}
	}

	TE_DEBUG_LOG("clean_size: %lu, next_flush_pos: %u", flush_size, w_info->next_flush_pos);
	hal_dcache_clean((unsigned long)w_info, CACHE_LINE_SIZE);
	hal_dcache_clean(CACHE_LINE_ALIGN_DOWN((unsigned long)g_pid_maps), CACHE_LINE_ALIGN_UP(sizeof(g_pid_maps)));
}
void trace_event_clean_dcache_conditional(const os_event_t *event, unsigned long arg_size, int is_warp_around)
{
	aw_trace_event_clean_dcache_conditional(&g_aw_trace_event_obj, event, arg_size, is_warp_around);
}
#endif

int trace_event_info(void)
{
	const aw_trace_event_cfg_t *cfg;
	const aw_trace_event_addr_info_32_t *addr_info;
	const writer_info_t *writer_info;

	cfg = &g_aw_trace_event_obj.cfg;
	addr_info = &g_aw_trace_event_obj.addr32;
	writer_info = g_aw_trace_event_obj.writer_info;

	TE_INFO_LOG("cfg: 64bit_pointer=%u, events_buf_len=0x%x(%u), "
		"max_task_name_cnt=%u, pid_map_type_size=%u, os_event_type_size=%u",
		cfg->is_64bit_pointer, cfg->events_buf_len, cfg->events_buf_len,
		cfg->max_task_name_cnt, cfg->pid_map_type_size, cfg->os_event_type_size);
	TE_INFO_LOG("addr: w_info=0x%08x, buf=0x%08x, subsys=0x%08x, pid_map=0x%08x",
		addr_info->writer_info_addr, addr_info->events_buf_addr,
		addr_info->subsys_class_addr, addr_info->pid_map_arr_addr);

	TE_INFO_LOG("writer: pos=%u, loop=%u", writer_info->write_pos, writer_info->write_loop_cnt);

	return 0;
}

static void trace_event_clear_buffer(void)
{
	unsigned long flags;
	uint32_t *events_buffer;
	const aw_trace_event_t *trace_event_obj;
	writer_info_t *w_info;

	trace_event_obj = &g_aw_trace_event_obj;
	events_buffer = trace_event_obj->events_buf;
	w_info = trace_event_obj->writer_info;

	flags = events_buffer_lock();

	memset(events_buffer, 0, MAX_BUFFER_SIZE);
	hal_dcache_clean((unsigned long)events_buffer, CACHE_LINE_ALIGN_UP(MAX_BUFFER_SIZE));

	memset(g_pid_maps, 0, sizeof(g_pid_maps));

	w_info->write_pos = 0;
	w_info->write_loop_cnt = 0;
	hal_dcache_clean((unsigned long)w_info, CACHE_LINE_SIZE);

	events_buffer_unlock(flags);
}

static inline int writer_handle_warp_around(writer_info_t *w_info, uint32_t *events_buf)
{
	events_buf[w_info->write_pos] = ~(EVENT_MAGIC);
#ifdef NATIVE_PLATFORM_CLEAN_DCACHE_REALTIME
	hal_dcache_clean(CACHE_LINE_ALIGN_DOWN((unsigned long)&events_buf[w_info->write_pos]), CACHE_LINE_SIZE);
#endif

	w_info->write_pos = 0;
	w_info->write_loop_cnt++;
	return 0;
}

static inline int writer_enter_discard_state(writer_info_t *w_info, uint32_t discard_ev_size)
{
	w_info->discard_event_cnt++;
	w_info->discard_data_size += discard_ev_size;
	w_info->is_discarding = 1;
	return 0;
}

static inline int writer_exit_discard_state(writer_info_t *w_info)
{
	if (w_info->is_discarding) {
		printf("Writer discard info: event_cnt=%u, data_size=%u\n",
			w_info->discard_event_cnt, w_info->discard_data_size);
		w_info->discard_event_cnt = 0;
		w_info->discard_data_size = 0;
		w_info->is_discarding = 0;
	}
	return 0;
}

static void sync_writer_info_to_shared_mem(const aw_trace_event_t *trace_event_obj, const writer_info_t *new_w_info)
{
	writer_info_t *w_info;
	w_info = trace_event_obj->writer_info;

	//FIXME: perhaps need hwspinlock to protect
	*w_info = *new_w_info;

#ifdef NATIVE_PLATFORM_CLEAN_DCACHE_REALTIME
	hal_dcache_clean((unsigned long)w_info, GLOBAL_WRITER_INFO_SIZE);
#endif
}

static inline os_event_t *aw_trace_event_get_ev_space(const aw_trace_event_t *trace_event_obj,
	int arg_sz, int *is_warp_around)
{
	os_event_t *event;
	writer_info_t w_info;
	reader_context_info_t *r_ctx_info_ptr, r_ctx_info;
	unsigned long ev_unit_size;
	int is_need_warp_around = 0;
	uint32_t *events_buf;

	w_info = *trace_event_obj->writer_info;
	events_buf = trace_event_obj->events_buf;
	//w_info = trace_event_obj->writer_info;

	ev_unit_size = event_unit_size(arg_sz);

	if ((MAX_BUFFER_UNITS - w_info.write_pos) <= ev_unit_size) {
		is_need_warp_around = 1;
		writer_handle_warp_around(&w_info, events_buf);
	}

	if (w_info.work_mode == WRITER_WORK_MODE_DISCARD_NEW_DATA) {
		r_ctx_info_ptr = trace_event_obj->reader_ctx_info;
		hal_dcache_invalidate((unsigned long)r_ctx_info_ptr, GLOBAL_READER_CONTEXT_INFO_SIZE);

		//FIXME: perhaps need hwspinlock to protect
		r_ctx_info = *r_ctx_info_ptr;

		if (is_need_warp_around) {
			if (ev_unit_size > r_ctx_info.read_pos) {
				/* There is no enough space to save the event after warp around,
				 * so the writer need to enter discard state. */
				writer_enter_discard_state(&w_info, ev_unit_size);
				event = NULL;
				goto sync_to_mem;
			} else {
				writer_exit_discard_state(&w_info);
			}
		} else {
			if ((w_info.write_loop_cnt > r_ctx_info.read_loop_cnt)
				&& ((w_info.write_pos + ev_unit_size) > r_ctx_info.read_pos)) {
				writer_enter_discard_state(&w_info, ev_unit_size);
				event = NULL;
				goto sync_to_mem;
			} else {
				writer_exit_discard_state(&w_info);
			}
		}
	}

	event = (os_event_t *)(&events_buf[w_info.write_pos]);
	w_info.write_pos += ev_unit_size;

sync_to_mem:
	sync_writer_info_to_shared_mem(trace_event_obj, &w_info);

	if (is_warp_around)
		*is_warp_around = is_need_warp_around;

	return event;
}

static inline os_event_t *aw_trace_event_init_ev(const aw_trace_event_t *trace_event_obj,
	unsigned long arg_sz, int *is_warp_around)
{
	os_event_t *event;

	event = aw_trace_event_get_ev_space(trace_event_obj, arg_sz, is_warp_around);
	if (event) {
		event->magic = EVENT_MAGIC;
		event->time =  ev_get_time();
		event->pid = tcb2pid(ev_get_tcb());
	}

	return event;
}

os_event_t *trace_init_event(unsigned long arg_sz, int *is_warp_around)
{
	return aw_trace_event_init_ev(&g_aw_trace_event_obj, arg_sz, is_warp_around);
}

int trace_event_dump(event_printf_t pr)
{
	return aw_trace_event_dump_all(&g_aw_trace_event_parser, &g_aw_trace_event_obj, 0, pr);
}

int trace_event_panic_dump(event_printf_t pr)
{
	return aw_trace_event_dump_all(&g_aw_trace_event_parser, &g_aw_trace_event_obj, 1, pr);
}

void trace_event_dump_all_events(event_printf_t pr)
{
	int i;
	struct subsys_entry *ev;

	for (i = 0; i < EV_NUM_SUBSYS; i++) {
		ev = &g_subsys_class[i];
		if (!ev->buildin)
			continue;
		pr("\t%-30s [%s]\r\n", ev->name, ev->enable ? "enable" : "disable");
	}
}

void trace_event_set_sys(const char *sys, bool action)
{
	int i;
	unsigned long flags;

	if (!sys) {
		flags = events_buffer_lock();
		for (i = 0; i < EV_NUM_SUBSYS; i++) {
			if (!g_subsys_class[i].buildin)
				continue;
			if (action)
				g_subsys_class[i].enable = 1;
			else
				g_subsys_class[i].enable = 0;
		}
		events_buffer_unlock(flags);
		return;
	}

	for (i = 0; i < EV_NUM_SUBSYS; i++) {
		if (!g_subsys_class[i].buildin)
			continue;
		if (strcmp(sys, g_subsys_class[i].name))
			continue;
		flags = events_buffer_lock();
		if (action)
			g_subsys_class[i].enable = 1;
		else
			g_subsys_class[i].enable = 0;
		events_buffer_unlock(flags);
		return;
	}
	printf("Can't find %s subsys\r\n", sys);
}

static void event_print_help(void)
{
	printf("useage:\r\n");
	printf("  trace_events events        :dump all support subsys\r\n");
	printf("  trace_events dump          :dump events buffer\r\n");
	printf("  trace_events ctrl name 0/1 :ctrl wether enable sys event\r\n");
	printf("  trace_events disable       :disable all sys event\r\n");
	printf("  trace_events enable        :enable all sys event\r\n");
	printf("  trace_events clear         :clear event buffer\r\n");
}

int cmd_events(int argc, char **argv)
{
	if (argc < 2) {
		event_print_help();
		return 0;
	}

	if (argc == 2) {
		if (!strcmp(argv[1], "dump"))
			trace_event_dump(printf);
		else if (!strcmp(argv[1], "events"))
			trace_event_dump_all_events(printf);
		else if (!strcmp(argv[1], "clear"))
			trace_event_clear_buffer();
		if (!strcmp(argv[1], "disable")) {
			trace_event_set_sys(NULL, false);
#ifndef NATIVE_PLATFORM_CLEAN_DCACHE_REALTIME
			if (g_aw_trace_event_obj.writer_info->work_mode == WRITER_WORK_MODE_OVERWRITE_DIRECTLY)
				trace_event_clean_dcache();
#endif
		}

		if (!strcmp(argv[1], "enable"))
			trace_event_set_sys(NULL, true);

#ifndef NATIVE_PLATFORM_CLEAN_DCACHE_REALTIME
		if (!strcmp(argv[1], "cache"))
			trace_event_clean_dcache();
#endif

		if (!strcmp(argv[1], "info"))
			trace_event_info();

		return 0;
	} else if (argc == 4) {
		if (!strcmp(argv[1], "ctrl")) {
			trace_event_set_sys(argv[2], argv[3][0] == '1' ? true : false);
		}
		return 0;
	}
	event_print_help();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_events, trace_events, Ctrl trace event);

#ifdef AW_TRACE_EVENT_DEBUG
int dump_new_trace_event(int argc, char **argv)
{
	int ret;

	g_reader_info.is_read_all = 0;
	g_reader_info.is_panic_when_read = 1;
	ret = aw_trace_event_dump(&g_aw_trace_event_parser, &g_reader_info, printf);
	if (ret)
		printf("aw_trace_event_dump failed, ret: %d\n", ret);

	printf("read_data_len: %u\n", g_reader_info.read_data_len);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(dump_new_trace_event, d, dump new trace event);

int dump_all_trace_event(int argc, char **argv)
{
	int ret;

	ret = aw_trace_event_dump_all(&g_aw_trace_event_parser, &g_aw_trace_event_obj, 0, printf);
	if (ret)
		printf("aw_trace_event_dump_all failed, ret: %d\n", ret);

	printf("read_data_len: %u\n", g_reader_info.read_data_len);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(dump_all_trace_event, da, dump all trace event);

static uint8_t g_tmp_test_buf[8196];
int dump_new_trace_event_to_buf(int argc, char **argv)
{
	int ret;

	g_reader_info.is_read_all = 0;
	g_reader_info.is_panic_when_read = 0;
	ret = aw_trace_event_read_to_buf(&g_aw_trace_event_parser, &g_reader_info, g_tmp_test_buf, sizeof(g_tmp_test_buf));
	if (ret)
		printf("aw_trace_event_dump_to_buf failed, ret: %d\n", ret);

	printf("g_tmp_test_buf(%px): '%s'\n", g_tmp_test_buf, g_tmp_test_buf);

	printf("g_event_data_buf(%px): '%s'\n", g_event_data_buf, g_event_data_buf);

	g_tmp_test_buf[0] = '\0';
	g_event_data_buf[0] = '\0';

	printf("read_data_len: %u\n", g_reader_info.read_data_len);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(dump_new_trace_event_to_buf, d2b, dump new trace event ot buf);

int dump_all_trace_event_to_buf(int argc, char **argv)
{
	int ret;

	ret = aw_trace_event_read_all_to_buf(&g_aw_trace_event_parser, &g_aw_trace_event_obj, 0, g_tmp_test_buf, sizeof(g_tmp_test_buf));
	if (ret)
		printf("aw_trace_event_dump_to_buf failed, ret: %d\n", ret);

	printf("g_tmp_test_buf(%px): '%s'\n", g_tmp_test_buf, g_tmp_test_buf);

	printf("g_event_data_buf(%px): '%s'\n", g_event_data_buf, g_event_data_buf);

	g_tmp_test_buf[0] = '\0';
	g_event_data_buf[0] = '\0';

	printf("read_data_len: %u\n", g_reader_info.read_data_len);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(dump_all_trace_event_to_buf, da2b, dump all trace event to buf);

int dump_struct_info(int argc, char **argv)
{
	TE_DEBUG_LOG("sizeof(aw_trace_event_t): 0x%x", sizeof(aw_trace_event_t));
	TE_DEBUG_LOG("sizeof(aw_trace_event_cfg_t): 0x%x", sizeof(aw_trace_event_cfg_t));
	TE_DEBUG_LOG("sizeof(aw_trace_event_info_32_t): 0x%x", sizeof(aw_trace_event_addr_info_32_t));
	TE_DEBUG_LOG("sizeof(aw_trace_event_info_64_t): 0x%x", sizeof(aw_trace_event_addr_info_64_t));
	TE_DEBUG_LOG("sizeof(os_event_t): %zu\n", sizeof(os_event_t));

	TE_DEBUG_LOG("aw_trace_event_t member offset:");
	TE_DEBUG_LOG("cfg: 0x%x", offsetof(aw_trace_event_t, cfg));
	TE_DEBUG_LOG("addr32: 0x%x", offsetof(aw_trace_event_t, addr32));
	TE_DEBUG_LOG("addr64: 0x%x", offsetof(aw_trace_event_t, addr64));
	TE_DEBUG_LOG("writer_info: 0x%x", offsetof(aw_trace_event_t, writer_info));
	TE_DEBUG_LOG("events_buf: 0x%x", offsetof(aw_trace_event_t, events_buf));

	uint32_t *write_pos;
	write_pos = &g_aw_trace_event_obj.writer_info->write_pos;

	TE_DEBUG_LOG("wpos: %u", *write_pos);
	TE_DEBUG_LOG("events buf, size: %u, units: %u", MAX_BUFFER_SIZE, MAX_BUFFER_UNITS);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(dump_struct_info, debug_te, debug aw trace event);
#endif

void __attribute__((no_instrument_function))
__cyg_profile_func_enter(void *this_func, void *call_site)
{
	trace_event_begin(EV_USR0, "", ARG_PTR_RENAME(func, (unsigned long)(this_func)));
}

void __attribute__((no_instrument_function))
__cyg_profile_func_exit(void *this_func, void *call_site)
{
	trace_event_end(EV_USR0, "", ARG_PTR_RENAME(func, (unsigned long)(this_func)));
}
#endif

