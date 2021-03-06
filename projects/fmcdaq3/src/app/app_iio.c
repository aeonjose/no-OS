/***************************************************************************//**
 *   @file   app_iio.c
 *   @brief  Application IIO setup.
 *   @author Antoniu Miclaus (antoniu.miclaus@analog.com)
********************************************************************************
 * Copyright 2020(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include "error.h"
#include "uart.h"
#include "uart_extra.h"
#include "iio_app.h"
#include "parameters.h"
#include "app_iio.h"
#ifndef PLATFORM_MB
#include "irq.h"
#include "irq_extra.h"
#endif

/******************************************************************************/
/************************ Variables Definitions *******************************/
/******************************************************************************/
static struct uart_desc *uart_desc;
#ifndef PLATFORM_MB
static struct irq_ctrl_desc *irq_desc;
#endif

/******************************************************************************/
/************************** Functions Implementation **************************/
/******************************************************************************/

/**
 * iio_server_write() - Write data to UART.
 * @buf - Data to be written.
 * @len - Number of bytes to be written.
 * @Return: SUCCESS in case of success, FAILURE otherwise.
 */
static ssize_t iio_server_write(const char *buf, size_t len)
{
	return uart_write(uart_desc, (const uint8_t *)buf, len);
}

/**
 * iio_server_read() - Read data from UART.
 * @buf - Storing data location.
 * @len - Number of bytes to be read.
 * @Return: SUCCESS in case of success, FAILURE otherwise.
 */
static ssize_t iio_server_read(char *buf, size_t len)
{
	return uart_read(uart_desc, (uint8_t *)buf, len);
}

/**
 * @brief Application IIO setup.
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t iio_server_init(struct iio_axi_adc_init_param *adc_init,
			struct iio_axi_dac_init_param *dac_init)
{
	struct xil_uart_init_param xil_uart_init_par = {
#ifdef PLATFORM_MB
		.type = UART_PL,
#else
		.type = UART_PS,
		.irq_id = UART_IRQ_ID,
#endif
	};
	struct uart_init_param uart_init_par = {
		.baud_rate = 921600,
		.device_id = UART_DEVICE_ID,
		.extra = &xil_uart_init_par,
	};
	struct iio_server_ops uart_iio_server_ops = {
		.read = iio_server_read,
		.write = iio_server_write,
	};
	struct iio_app_init_param iio_app_init_par = {
		.iio_server_ops = &uart_iio_server_ops,
	};
	struct iio_app_desc *iio_app_desc;
	struct iio_axi_adc_desc *iio_axi_adc_desc;
	struct iio_axi_dac_desc *iio_axi_dac_desc;
	int32_t status;

#ifndef PLATFORM_MB
	struct xil_irq_init_param xil_irq_init_param = {
		.type = IRQ_PS,
	};

	struct irq_init_param irq_init_param = {
		.irq_ctrl_id = INTC_DEVICE_ID,
		.extra = &xil_irq_init_param,
	};

	status = irq_ctrl_init(&irq_desc, &irq_init_param);
	if (IS_ERR_VALUE(status))
		return FAILURE;

	xil_uart_init_par.irq_desc = irq_desc;
#endif

	status = uart_init(&uart_desc, &uart_init_par);
	if (IS_ERR_VALUE(status))
		return FAILURE;

#ifndef PLATFORM_MB
	status = irq_global_enable(irq_desc);
	if (IS_ERR_VALUE(status))
		return FAILURE;
#endif

	status = iio_app_init(&iio_app_desc, &iio_app_init_par);
	if (IS_ERR_VALUE(status))
		return FAILURE;

	status = iio_axi_adc_init(&iio_axi_adc_desc, adc_init);
	if (IS_ERR_VALUE(status))
		return FAILURE;

	status = iio_axi_dac_init(&iio_axi_dac_desc, dac_init);
	if (IS_ERR_VALUE(status))
		return FAILURE;

	return iio_app(iio_app_desc);
}
