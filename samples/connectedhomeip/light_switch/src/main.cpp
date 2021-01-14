/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "app_task.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(app);

int main()
{
	return GetAppTask().StartApp();
}
