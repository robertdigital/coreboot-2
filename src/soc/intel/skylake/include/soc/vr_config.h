/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
 * Copyright (C) 2015 Intel Corporation.
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

/* VR Settings for each domain */

#ifndef _SOC_VR_CONFIG_H_
#define _SOC_VR_CONFIG_H_

#include <fsp/soc_binding.h>

struct vr_config {

	/*
	 * The below settings will take effect when this is set to 1
	 * for that domain.
	 */
	int vr_config_enable;

	/* Power State X current cuttof in 1/4 Amp increments
	 * Range is 0-128A
	 */
	int psi1threshold;
	int psi2threshold;
	int psi3threshold;

	/* Enable power state 3/4 for different domains */
	int psi3enable;
	int psi4enable;

	/*
	 * Imon slope correction. Specified in 1/100 increment
	 * values. Range is 0-200. 125 = 1.25
	 */
	int imon_slope;

	/*
	 * Imon offset correction. Units 1/4, Range 0-255.
	 * Value of 100 = 100/4 = 25 offset.
	 */
	int imon_offset;

	/* VR Icc Max limit. 0-255A in 1/4 A units. 400 = 100A */
	int icc_max;

	/* VR Voltage Limit. Range is 0-7999mV */
	int voltage_limit;
};

#define VR_CFG_AMP(i) ((i) * 4)

/* VrConfig Settings for 5 domains
 * 0 = System Agent, 1 = IA Core, 2 = Ring,
 * 3 = GT unsliced,  4 = GT sliced
 */
enum vr_domain{
	VR_SYSTEM_AGENT,
	VR_IA_CORE,
	VR_RING,
	VR_GT_UNSLICED,
	VR_GT_SLICED,
	NUM_VR_DOMAINS
};

void fill_vr_domain_config(SILICON_INIT_UPD *params, int domain,
				const struct vr_config *cfg);

#endif
