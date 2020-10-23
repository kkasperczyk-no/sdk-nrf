/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <logging/log.h>
#include <mbedtls/pkcs5.h>

LOG_MODULE_REGISTER(cli_sample, CONFIG_OT_COMMAND_LINE_INTERFACE_LOG_LEVEL);

#define WELLCOME_TEXT \
	"\n\r"\
	"\n\r"\
	"OpenThread Command Line Interface is now running.\n\r" \
	"Use the 'ot' keyword to invoke OpenThread commands e.g. " \
	"'ot thread start.'\n\r" \
	"For the full commands list refer to the OpenThread CLI " \
	"documentation at:\n\r" \
	"https://github.com/openthread/openthread/blob/master/src/cli/README.md\n\r"


void main(void)
{
	LOG_INF(WELLCOME_TEXT);

    const mbedtls_md_info_t * md_info;
    mbedtls_md_context_t md_ctxt;
    int use_hmac = 1;

    bool free_md_ctxt = false;

    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    mbedtls_md_init(&md_ctxt);
    free_md_ctxt = true;

    mbedtls_md_setup(&md_ctxt, md_info, use_hmac);

	static const char * salt = "abcdefghijklmnopqrstuvwyz";
	const uint8_t saltLength = 25;
	uint32_t password = 1234;
	static uint8_t out[2][40];
	const uint8_t keyLength = 80;
	const uint16_t iterationsNumber = 4096;

	uint32_t startTime = k_cycle_get_32();	
    mbedtls_pkcs5_pbkdf2_hmac(&md_ctxt, (uint8_t*)(&password), sizeof(password), salt, saltLength,
                                       iterationsNumber, keyLength, &out[0][0]);
	uint32_t stopTime = k_cycle_get_32();

	LOG_INF("mbedtls_pkcs5_pbkdf2_hmac calculation time: %d ms", (stopTime-startTime)/(sys_clock_hw_cycles_per_sec()/1000));

    if (free_md_ctxt)
    {
        mbedtls_md_free(&md_ctxt);
    }
}
