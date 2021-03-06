/*
 * This file is part of the coreboot project.
 *
 * Copyright 2010 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <arch/exception.h>
#include <bootblock_common.h>
#include <console/console.h>
#include <delay.h>
#include <program_loading.h>
#include <symbols.h>
#include <timestamp.h>

DECLARE_OPTIONAL_REGION(timestamp);

__attribute__((weak)) void bootblock_mainboard_early_init(void) { /* no-op */ }
__attribute__((weak)) void bootblock_soc_init(void) { /* do nothing */ }
__attribute__((weak)) void bootblock_mainboard_init(void) { /* do nothing */ }

void main(void)
{
	init_timer();

	/* Initialize timestamps if we have TIMESTAMP region in memlayout.ld. */
	if (IS_ENABLED(CONFIG_COLLECT_TIMESTAMPS) && _timestamp_size > 0)
		timestamp_init(timestamp_get());

	bootblock_mainboard_early_init();

	if (IS_ENABLED(CONFIG_BOOTBLOCK_CONSOLE)) {
		console_init();
		exception_init();
	}

	bootblock_soc_init();
	bootblock_mainboard_init();

	run_romstage();
}
