/* ptpusb.c
 *
 * USB-based PTP implementation
 * Based on ptpcam.c
 * Copyright (C) 2001-2003 Mariusz Woloszyn <emsi@ipartners.pl>
 * Parts Copyright (C) 2003 - 2004 Nikolai Kopanygin <superkolik@mail.ru>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <unistd.h>
#include <libptp2/ptp.h>
#include "ptpusb.h"

/*
    One global variable. Set it to 1 to receive more debugging info.
*/ 
short ptp_usb_verbose=0;

static short
ptp_read_func (unsigned char *bytes, unsigned int size, void *data)
{
	int result;
	PTP_USB *ptp_usb=(PTP_USB *)data;

	result=usb_bulk_read(ptp_usb->handle, ptp_usb->inep, bytes, size,3000);
	if (result==0)
	result=usb_bulk_read(ptp_usb->handle, ptp_usb->inep, bytes, size,3000);
	if (result >= 0)
		return (PTP_RC_OK);
	else
	{
		if (ptp_usb_verbose) perror("usb_bulk_read");
		return PTP_ERROR_IO;
	}
}

static short
ptp_write_func (unsigned char *bytes, unsigned int size, void *data)
{
	int result;
	PTP_USB *ptp_usb=(PTP_USB *)data;

	result=usb_bulk_write(ptp_usb->handle,ptp_usb->outep,bytes,size,3000);
	if (result >= 0)
		return (PTP_RC_OK);
	else
	{
		if (ptp_usb_verbose) perror("usb_bulk_write");
		return PTP_ERROR_IO;
	}
}

/* Added by Nikolai Kopanygin */
static short
ptp_check_int_func(unsigned char *bytes, unsigned int size, void *data)
{
	int count=100; /* try 100 times */
    int result=0;
	PTP_USB *ptp_usb=(PTP_USB *)data;
	while((result==0) && (count!=0)) {
        result=usb_bulk_read(ptp_usb->handle, ptp_usb->intep, bytes, size,3000);
        count--;
    }
	if (result >= 0) {

        do {
            bytes+=result;
            size-=result;
            if (size>0)
                result=usb_bulk_read(ptp_usb->handle, ptp_usb->intep, bytes, size,3000);
        } while ((result>0) && (size>0));

		return (PTP_RC_OK);
	} else {
		if (ptp_usb_verbose) perror("usb_bulk_read");
		return PTP_ERROR_IO;
	}
}

static short
ptp_check_int_fast_func(unsigned char *bytes, unsigned int size, void *data)
{
	int result;
	PTP_USB *ptp_usb=(PTP_USB *)data;

	result=usb_bulk_read(ptp_usb->handle, ptp_usb->intep, bytes, size,3000);
	if (result==0)
	result=usb_bulk_read(ptp_usb->handle, ptp_usb->intep, bytes, size,3000);
	if (result >= 0) {

        do {
            bytes+=result;
            size-=result;
            if (size>0)
                result=usb_bulk_read(ptp_usb->handle, ptp_usb->intep, bytes, size,3000);
        } while ((result>0) && (size>0));

		return (PTP_RC_OK);
	} else {
		if (ptp_usb_verbose) perror("usb_bulk_read");
		return PTP_ERROR_IO;
	}
}

void
debug (void *data, const char *format, va_list args)
{
	if (ptp_usb_verbose<2) return;
	vfprintf (stderr, format, args);
	fprintf (stderr,"\n");
	fflush(stderr);
}

void
error (void *data, const char *format, va_list args)
{
//	if (!ptp_usb_verbose) return;
	vfprintf (stderr, format, args);
	fprintf (stderr,"\n");
	fflush(stderr);
}

void
init_ptp_usb (PTPParams* params, PTP_USB* ptp_usb, struct usb_device* dev)
{
	usb_dev_handle *device_handle;

	params->write_func=ptp_write_func;
	params->read_func=ptp_read_func;
    params->check_int_func=ptp_check_int_func;
    params->check_int_fast_func=ptp_check_int_fast_func;
	params->error_func=error;
	params->debug_func=debug;
	params->sendreq_func=ptp_usb_sendreq;
	params->senddata_func=ptp_usb_senddata;
	params->getresp_func=ptp_usb_getresp;
	params->getdata_func=ptp_usb_getdata;
	params->data=ptp_usb;
	params->transaction_id=0;
	params->byteorder = PTP_DL_LE;

	if ((device_handle=usb_open(dev))){
		if (!device_handle) {
			perror("usb_open()");
			exit(0);
		}
		ptp_usb->handle=device_handle;
		usb_claim_interface(device_handle,
			dev->config->interface->altsetting->bInterfaceNumber);
	}
}

struct usb_bus*
init_usb()
{
	usb_init();
	usb_find_busses();
	usb_find_devices();
	return (usb_get_busses());
}

/*
   find_device() returns the pointer to a usb_device structure matching
   given busn, devicen numbers. If any or both of arguments are 0 then the
   first matching PTP device structure is returned. 
*/
struct usb_device*
find_device (int busn, int devn, short force)
{
	struct usb_bus *bus;
	struct usb_device *dev;

	bus=init_usb();
	for (; bus; bus = bus->next)
	for (dev = bus->devices; dev; dev = dev->next)
	if ((dev->config->interface->altsetting->bInterfaceClass==
		USB_CLASS_PTP)||force)
	if (dev->descriptor.bDeviceClass!=USB_CLASS_HUB)
	{
		int curbusn, curdevn;

		curbusn=strtol(bus->dirname,NULL,10);
		curdevn=strtol(dev->filename,NULL,10);

		if (devn==0) {
			if (busn==0) return dev;
			if (curbusn==busn) return dev;
		} else {
			if ((busn==0)&&(curdevn==devn)) return dev;
			if ((curbusn==busn)&&(curdevn==devn)) return dev;
		}
	}
	return NULL;
}

void
find_endpoints(struct usb_device *dev, int* inep, int* outep, int* intep)
{
	int i,n;
	struct usb_endpoint_descriptor *ep;

	ep = dev->config->interface->altsetting->endpoint;
	n=dev->config->interface->altsetting->bNumEndpoints;

	for (i=0;i<n;i++) {
	if (ep[i].bmAttributes==USB_ENDPOINT_TYPE_BULK)	{
		if ((ep[i].bEndpointAddress&USB_ENDPOINT_DIR_MASK)==
			USB_ENDPOINT_DIR_MASK)
			*inep=ep[i].bEndpointAddress;
		if ((ep[i].bEndpointAddress&USB_ENDPOINT_DIR_MASK)==0)
			*outep=ep[i].bEndpointAddress;
		} else if (ep[i].bmAttributes==USB_ENDPOINT_TYPE_INTERRUPT){
			if ((ep[i].bEndpointAddress&USB_ENDPOINT_DIR_MASK)==
				USB_ENDPOINT_DIR_MASK)
				*intep=ep[i].bEndpointAddress;
		}
	}
}

uint16_t
set_property (PTPParams* params,
		uint16_t property, char* value, uint16_t datatype)
{
	void* val=NULL;

	switch(datatype) {
	case PTP_DTC_INT8:
		val=malloc(sizeof(int8_t));
		*(int8_t*)val=(int8_t)strtol(value,NULL,0);
		break;
	case PTP_DTC_UINT8:
		val=malloc(sizeof(uint8_t));
		*(uint8_t*)val=(uint8_t)strtol(value,NULL,0);
		break;
	case PTP_DTC_INT16:
		val=malloc(sizeof(int16_t));
		*(int16_t*)val=(int16_t)strtol(value,NULL,0);
		break;
	case PTP_DTC_UINT16:
		val=malloc(sizeof(uint16_t));
		*(uint16_t*)val=(uint16_t)strtol(value,NULL,0);
		break;
	case PTP_DTC_INT32:
		val=malloc(sizeof(int32_t));
		*(int32_t*)val=(int32_t)strtol(value,NULL,0);
		break;
	case PTP_DTC_UINT32:
		val=malloc(sizeof(uint32_t));
		*(uint32_t*)val=(uint32_t)strtol(value,NULL,0);
		break;
	case PTP_DTC_STR:
		val=(void *)value;
	}
	return(ptp_setdevicepropvalue(params, property, val, datatype));
	free(val);
	return 0;
}

int
usb_get_endpoint_status(PTP_USB* ptp_usb, int ep, uint16_t* status)
{
	 return (usb_control_msg(ptp_usb->handle,
		USB_DP_DTH|USB_RECIP_ENDPOINT, USB_REQ_GET_STATUS,
		USB_FEATURE_HALT, ep, (char *)status, 2, 3000));
}

int
usb_clear_stall_feature(PTP_USB* ptp_usb, int ep)
{

	return (usb_control_msg(ptp_usb->handle,
		USB_RECIP_ENDPOINT, USB_REQ_CLEAR_FEATURE, USB_FEATURE_HALT,
		ep, NULL, 0, 3000));
}

int
usb_ptp_get_device_status(PTP_USB* ptp_usb, uint16_t* devstatus)
{
	return (usb_control_msg(ptp_usb->handle,
		USB_DP_DTH|USB_TYPE_CLASS|USB_RECIP_INTERFACE,
		USB_REQ_GET_DEVICE_STATUS, 0, 0,
		(char *)devstatus, 4, 3000));
}

int
usb_ptp_device_reset(PTP_USB* ptp_usb)
{
	return (usb_control_msg(ptp_usb->handle,
		USB_TYPE_CLASS|USB_RECIP_INTERFACE,
		USB_REQ_DEVICE_RESET, 0, 0, NULL, 0, 3000));
}

