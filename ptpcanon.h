/* ptpcanon.h
 *
 * Copyright (C) 2003 - 2004 Nikolai Kopanygin <superkolik@mail.ru>
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

#ifndef PTPCANON_H
#define PTPCANON_H

#include <libptp2/ptp.h>
#include "ptpusb.h"

/*
    Some macros
*/

/* Check value and Return on error */
#define CR(result,error) {						\
			if((result)!=PTP_RC_OK) {			\
				fprintf(stderr,"ERROR: "error);		\
				usb_release_interface(ptp_usb.handle,	\
		dev->config->interface->altsetting->bInterfaceNumber);	\
				return;					\
			}						\
}

/* Check value and Return result on error */
#define CR2(result,error) {						\
			if((result)!=PTP_RC_OK) {			\
				fprintf(stderr,"ERROR: "error);		\
				return result;					\
			}						\
}

/* Check value and Return -1 on error */
#define CRR(result) {							\
			if((result)!=PTP_RC_OK) {			\
				return -1;				\
			}						\
}

/* Check value and Report (PTP) Error if needed */
#define CRE(result) {							\
			uint16_t r;					\
			r=(result);					\
			if (r!=PTP_RC_OK)				\
				ptp_perror(&params,r);			\
}

/* Check value and Report (PTP) Error if needed */
/* 
    The same as above, but with params a pointer 
*/
#define CRE2(result) {							\
			uint16_t r;					\
			r=(result);					\
			if (r!=PTP_RC_OK)				\
				ptp_perror(params,r);			\
}

/* Check value and Continue on error */
#define CC(result,error) {						\
			if((result)!=PTP_RC_OK) {			\
				fprintf(stderr,"ERROR: "error);		\
				usb_release_interface(ptp_usb.handle,	\
		dev->config->interface->altsetting->bInterfaceNumber);	\
				continue;					\
			}						\
}
 
/* requested actions */
#define ACT_DEVICE_RESET	01
#define ACT_LIST_DEVICES	02
#define ACT_LIST_PROPERTIES	03
#define ACT_GETSET_PROPERTY	04
#define ACT_CANON_TEST      05
#define ACT_TAKE_SHOT       06

void usage(void);
void help(void);
void list_devices(short force);
const char* get_property_description(PTPParams* params, uint16_t dpc);
void list_properties (int dev, int bus, short force);
short print_propval (uint16_t datatype, void* value, short hex);
void getset_property (int busn,int devn,uint16_t property,char* value,short force);
void test_canon (int busn,int devn,int force);
void take_shot (int busn, int devn,uint16_t force, char* filename, uint8_t flash, uint16_t zoom, int on_button);
void reset_device (int busn, int devn, short force);

uint16_t connect_camera(int busn, int devn, uint16_t force, PTP_USB *ptp_usb, PTPParams *params, struct usb_device **dev);
uint16_t disconnect_camera(PTP_USB *ptp_usb, PTPParams *params, struct usb_device *dev);
uint16_t enter_shooting_mode(PTPParams *params);
uint16_t exit_shooting_mode(PTPParams *params);
uint16_t download_object(PTPParams *params, uint32_t handle, char *filename);

#endif
