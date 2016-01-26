/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 The ChromiumOS Authors.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <types.h>
#include <pc80/mc146818rtc.h>
#include "vbnv.h"
#include "vbnv_layout.h"

void read_vbnv_cmos(uint8_t *vbnv_copy)
{
	int i;

	for (i = 0; i < CONFIG_VBNV_SIZE; i++)
		vbnv_copy[i] = cmos_read(CONFIG_VBNV_OFFSET + 14 + i);
}

void save_vbnv_cmos(const uint8_t *vbnv_copy)
{
	int i;

	for (i = 0; i < CONFIG_VBNV_SIZE; i++)
		cmos_write(vbnv_copy[i], CONFIG_VBNV_OFFSET + 14 + i);
}
