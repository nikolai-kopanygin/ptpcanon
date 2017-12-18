/* ptpusb.h
 *
 * USB-based PTP implementation
 * Based on ptpcam.c
 * Copyright (C) 2001-2003 Mariusz Woloszyn <emsi@ipartners.pl>
 *
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

#ifndef PTPUSB_H
#define PTPUSB_H

#if defined(__cplusplus)
extern "C"
{
#endif	/* __cplusplus */

#include <libptp2/ptp.h>

/* USB interface class */
#ifndef USB_CLASS_PTP
#define USB_CLASS_PTP		6
#endif

/* USB control message data phase direction */
#ifndef USB_DP_HTD
#define USB_DP_HTD		(0x00 << 7)	// host to device
#endif
#ifndef USB_DP_DTH
#define USB_DP_DTH		(0x01 << 7)	// device to host
#endif

/* PTP class specific requests */
#ifndef USB_REQ_DEVICE_RESET
#define USB_REQ_DEVICE_RESET		0x66
#endif
#ifndef USB_REQ_GET_DEVICE_STATUS
#define USB_REQ_GET_DEVICE_STATUS	0x67
#endif

/* USB Feature selector HALT */
#ifndef USB_FEATURE_HALT
#define USB_FEATURE_HALT	0x00
#endif

/* error reporting macro */
#define ERROR(error) fprintf(stderr,"ERROR: "error);				

/* printing value type */
#define PTPCAM_PRINT_HEX	00
#define PTPCAM_PRINT_DEC	01

/* property value printing macros */
#define PRINT_PROPVAL_DEC(value)	\
		print_propval(dpd.DataType, value,			\
		PTPCAM_PRINT_DEC)

#define PRINT_PROPVAL_HEX(value)					\
		print_propval(dpd.DataType, value,			\
		PTPCAM_PRINT_HEX)

typedef struct _PTP_USB PTP_USB;
struct _PTP_USB {
	usb_dev_handle* handle;
	int inep;
	int outep;
	int intep;
};


struct usb_bus* init_usb(void);
void debug (void *data, const char *format, va_list args);
void error (void *data, const char *format, va_list args);
void init_ptp_usb (PTPParams* params, PTP_USB* ptp_usb, struct usb_device* dev);

/*
   find_device() returns the pointer to a usb_device structure matching
   given busn, devicen numbers. If any or both of arguments are 0 then the
   first matching PTP device structure is returned. 
*/
struct usb_device* find_device (int busn, int devicen, short force);
void find_endpoints(struct usb_device *dev, int* inep, int* outep, int* intep);
uint16_t set_property (PTPParams* params, uint16_t property, char* value, uint16_t datatype);
int usb_get_endpoint_status(PTP_USB* ptp_usb, int ep, uint16_t* status);
int usb_clear_stall_feature(PTP_USB* ptp_usb, int ep);
int usb_ptp_get_device_status(PTP_USB* ptp_usb, uint16_t* devstatus);
int usb_ptp_device_reset(PTP_USB* ptp_usb);

/*
    One global variable. Set it to 1 to receive more debugging info.
*/ 
extern short ptp_usb_verbose;



#if defined(__cplusplus)
}
#endif	/* __cplusplus */

#endif
