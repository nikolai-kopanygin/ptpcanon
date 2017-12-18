/* ptpcanon.c
 *
 * Based on ptpcam.c
 * Copyright (C) 2001-2003 Mariusz Woloszyn <emsi@ipartners.pl>
 *
 * Canon related parts
 * Copyright (C) 2003 Nikolai Kopanygin <superkolik@mail.ru>
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
#include <getopt.h>
#include <usb.h>
#include <unistd.h>

#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (GETTEXT_PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

#include "ptpcanon.h"


#define MAXNAMELEN 256

void
usage()
{
	printf("USAGE: ptpcanon [OPTION]\n\n");
}

void
help()
{
	printf("USAGE: ptpcanon [OPTION]\n\n");
	printf("Options:\n"
	"  -h, --help                   Print this help message\n"
	"  -B, --bus=BUS-NUMBER         USB bus number\n"
	"  -D, --dev=DEV-NUMBER         USB assigned device number\n"
	"  -r, --reset                  Reset the device\n"
	"  -l, --list-devices           List all PTP devices\n"
	"  -p, --list-properties        List all PTP device properties\n"
	"                               "
				"(e.g. focus mode, focus distance, etc.)\n"
	"  -s, --show-property=NUMBER   Display property details "
					"(or set its value,\n"
	"                               if used in conjunction with --val)\n"
	"  --set-property=NUMBER        Set property value (--val required)\n"
	"  --val=VALUE                  Property value\n"
	"  -f, --force                  Talk to non PTP devices\n"
	"  -v, --verbose                Be verbosive (print more debug)\n"
    "  -c, --canon-test             Test a Canon PTP camera\n"
    "  -t, --take-shot              Take a photo and save it to disk as noname.jpg by default (see also --name)\n"
    "  -N, --name FILENAME          Save the taken photo under FILENAME\n"
    "  -F, --flash FLASH            Use this flash value with --take-shot\n",
    "  -Z, --zoom ZOOM              Use this zoom value with --take-shot\n",
    "  -b, --on-button              Capture on the Shutter button press, use with --take-shot\n",
	"\n");
}


void
list_devices(short force)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	int found=0;


	bus=init_usb();
  	for (; bus; bus = bus->next)
    	for (dev = bus->devices; dev; dev = dev->next) {
		/* if it's a PTP device try to talk to it */
		if ((dev->config->interface->altsetting->bInterfaceClass==
			USB_CLASS_PTP)||force)
		if (dev->descriptor.bDeviceClass!=USB_CLASS_HUB)
		{
			int n;
			struct usb_endpoint_descriptor *ep;
			PTPParams params;
			PTP_USB ptp_usb;
			//int inep=0, outep=0, intep=0;
			PTPDeviceInfo deviceinfo;

			if (!found){
				printf("Listing devices...\n");
				printf("bus/dev\tvendorID/prodID\tdevice model\n");
				found=1;
			}
			ep = dev->config->interface->altsetting->endpoint;
			n=dev->config->interface->altsetting->bNumEndpoints;
			/* find endpoints */
/*			for (i=0;i<n;i++) {
				if (ep[i].bmAttributes==2) {
					if ((ep[i].bEndpointAddress&0x80)==0x80)
						inep=ep[i].bEndpointAddress;
					if ((ep[i].bEndpointAddress&0x80)==0)
						outep=ep[i].bEndpointAddress;
				} else if (ep[i].bmAttributes==3) {
					if ((ep[i].bEndpointAddress&0x80)==0x80)
						intep=ep[i].bEndpointAddress;
				}
			}
			ptp_usb.inep=inep;
			ptp_usb.outep=outep;
			ptp_usb.intep=intep;
*/
			find_endpoints(dev,&ptp_usb.inep,&ptp_usb.outep,
				&ptp_usb.intep);
			init_ptp_usb(&params, &ptp_usb, dev);

			CC(ptp_opensession (&params,1),
				"Could not open session!\n"
				"Try to reset the camera.\n");
			CC(ptp_getdeviceinfo (&params, &deviceinfo),
				"Could not get device info!\n");

      			printf("%s/%s\t0x%04X/0x%04X\t%s\n",
				bus->dirname, dev->filename,
				dev->descriptor.idVendor,
				dev->descriptor.idProduct, deviceinfo.Model);

			CC(ptp_closesession(&params),
				"Could not close session!\n");
			usb_release_interface(ptp_usb.handle,
			dev->config->interface->altsetting->bInterfaceNumber);
		}
	}
	if (!found) printf("\nFound no PTP devices\n");
	printf("\n");
}

const char*
get_property_description(PTPParams* params, uint16_t dpc)
{
	int i;
	// Device Property descriptions
	struct {
		uint16_t dpc;
		const char *txt;
	} ptp_device_properties[] = {
		{PTP_DPC_Undefined,		N_("PTP Undefined Property")},
		{PTP_DPC_BatteryLevel,		N_("Battery Level")},
		{PTP_DPC_FunctionalMode,	N_("Functional Mode")},
		{PTP_DPC_ImageSize,		N_("Image Size")},
		{PTP_DPC_CompressionSetting,	N_("Compression Setting")},
		{PTP_DPC_WhiteBalance,		N_("White Balance")},
		{PTP_DPC_RGBGain,		N_("RGB Gain")},
		{PTP_DPC_FNumber,		N_("F-Number")},
		{PTP_DPC_FocalLength,		N_("Focal Length")},
		{PTP_DPC_FocusDistance,		N_("Focus Distance")},
		{PTP_DPC_FocusMode,		N_("Focus Mode")},
		{PTP_DPC_ExposureMeteringMode,	N_("Exposure Metering Mode")},
		{PTP_DPC_FlashMode,		N_("Flash Mode")},
		{PTP_DPC_ExposureTime,		N_("Exposure Time")},
		{PTP_DPC_ExposureProgramMode,	N_("Exposure Program Mode")},
		{PTP_DPC_ExposureIndex,
					N_("Exposure Index (film speed ISO)")},
		{PTP_DPC_ExposureBiasCompensation,
					N_("Exposure Bias Compensation")},
		{PTP_DPC_DateTime,		N_("Date Time")},
		{PTP_DPC_CaptureDelay,		N_("Pre-Capture Delay")},
		{PTP_DPC_StillCaptureMode,	N_("Still Capture Mode")},
		{PTP_DPC_Contrast,		N_("Contrast")},
		{PTP_DPC_Sharpness,		N_("Sharpness")},
		{PTP_DPC_DigitalZoom,		N_("Digital Zoom")},
		{PTP_DPC_EffectMode,		N_("Effect Mode")},
		{PTP_DPC_BurstNumber,		N_("Burst Number")},
		{PTP_DPC_BurstInterval,		N_("Burst Interval")},
		{PTP_DPC_TimelapseNumber,	N_("Timelapse Number")},
		{PTP_DPC_TimelapseInterval,	N_("Timelapse Interval")},
		{PTP_DPC_FocusMeteringMode,	N_("Focus Metering Mode")},
		{PTP_DPC_UploadURL,		N_("Upload URL")},
		{PTP_DPC_Artist,		N_("Artist")},
		{PTP_DPC_CopyrightInfo,		N_("Copyright Info")},
		{0,NULL}
	};
	struct {
		uint16_t dpc;
		const char *txt;
	} ptp_device_properties_EK[] = {
		{PTP_DPC_EK_ColorTemperature,	N_("EK: Color Temperature")},
		{PTP_DPC_EK_DateTimeStampFormat,
					N_("EK: Date Time Stamp Format")},
		{PTP_DPC_EK_BeepMode,		N_("EK: Beep Mode")},
		{PTP_DPC_EK_VideoOut,		N_("EK: Video Out")},
		{PTP_DPC_EK_PowerSaving,	N_("EK: Power Saving")},
		{PTP_DPC_EK_UI_Language,	N_("EK: UI Language")},
		{0,NULL}
	};

	struct {
		uint16_t dpc;
		const char *txt;
	} ptp_device_properties_CANON[] = {
		{PTP_DPC_CANON_BeepMode,	N_("CANON: Beep Mode")},
		{PTP_DPC_CANON_UnixTime,	N_("CANON: Time measured in"
						" secondssince 01-01-1970")},
		{PTP_DPC_CANON_FlashMemory,	N_("CANON: Flash Card Capacity")},
		{PTP_DPC_CANON_CameraModel,	N_("CANON: Camera Model")},

		{0,NULL}
	};

	if (dpc|PTP_DPC_EXTENSION_MASK==PTP_DPC_EXTENSION)
	switch (params->deviceinfo.VendorExtensionID) {
		case PTP_VENDOR_EASTMAN_KODAK:
		for (i=0; ptp_device_properties_EK[i].txt!=NULL; i++)
			if (ptp_device_properties_EK[i].dpc==dpc)
				return (ptp_device_properties_EK[i].txt);
		break;

		case PTP_VENDOR_CANON:
		for (i=0; ptp_device_properties_CANON[i].txt!=NULL; i++)
			if (ptp_device_properties_CANON[i].dpc==dpc)
				return (ptp_device_properties_CANON[i].txt);
		break;
	}
	for (i=0; ptp_device_properties[i].txt!=NULL; i++)
		if (ptp_device_properties[i].dpc==dpc)
			return (ptp_device_properties[i].txt);

	return NULL;
}

void
list_properties (int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	const char* propdesc;
	int i;

	printf("Listing properties...\n");
#ifdef DEBUG
	printf("dev %i\tbus %i\n",devn,busn);
#endif
	dev=find_device(busn,devn,force);
	if (dev==NULL) {
		fprintf(stderr,"could not find any device matching given "
		"bus/dev numbers\n");
		exit(-1);
	}
	find_endpoints(dev,&ptp_usb.inep,&ptp_usb.outep,&ptp_usb.intep);

	init_ptp_usb(&params, &ptp_usb, dev);
	CR(ptp_opensession (&params,1),
		"Could not open session!\n");
	CR(ptp_getdeviceinfo (&params, &params.deviceinfo),
		"Could not get device info\n");
	printf("Querying: %s\n",params.deviceinfo.Model);
	for (i=0; i<params.deviceinfo.DevicePropertiesSupported_len;i++){
		propdesc=get_property_description(&params,
			params.deviceinfo.DevicePropertiesSupported[i]);
		if (propdesc!=NULL) 
			printf("0x%04x : %s\n",params.deviceinfo.
				DevicePropertiesSupported[i], propdesc);
		else
			printf("0x%04x : 0x%04x\n",params.deviceinfo.
				DevicePropertiesSupported[i],
				params.deviceinfo.
					DevicePropertiesSupported[i]);
	}
	CR(ptp_closesession(&params), "Could not close session!\n");
	usb_release_interface(ptp_usb.handle,
		dev->config->interface->altsetting->bInterfaceNumber);
}

short
print_propval (uint16_t datatype, void* value, short hex)
{
	switch (datatype) {
		case PTP_DTC_INT8:
			printf("%hhi",*(char*)value);
			return 0;
		case PTP_DTC_UINT8:
			printf("%hhu",*(unsigned char*)value);
			return 0;
		case PTP_DTC_INT16:
			printf("%hi",*(int16_t*)value);
			return 0;
		case PTP_DTC_UINT16:
			if (hex==PTPCAM_PRINT_HEX)
				printf("0x%04hX (%hi)",*(uint16_t*)value,
					*(uint16_t*)value);
			else
				printf("%hi",*(uint16_t*)value);
			return 0;
		case PTP_DTC_INT32:
			printf("%i",*(int32_t*)value);
			return 0;
		case PTP_DTC_UINT32:
			if (hex==PTPCAM_PRINT_HEX)
				printf("0x%08X (%i)",*(uint32_t*)value,
					*(uint32_t*)value);
			else
				printf("%i",*(uint32_t*)value);
			return 0;
		case PTP_DTC_STR:
			printf("\"%s\"",(char *)value);
	}
	return -1;
}


void
getset_property (int busn,int devn,uint16_t property,char* value,short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	PTPDevicePropDesc dpd;
	const char* propdesc;

#ifdef DEBUG
	printf("dev %i\tbus %i\n",devn,busn);
#endif
	dev=find_device(busn,devn,force);
	if (dev==NULL) {
		fprintf(stderr,"could not find any device matching given "
		"bus/dev numbers\n");
		exit(-1);
	}
	find_endpoints(dev,&ptp_usb.inep,&ptp_usb.outep,&ptp_usb.intep);

	init_ptp_usb(&params, &ptp_usb, dev);
	CR(ptp_opensession (&params,1),
		"Could not open session!\nTry to reset the camera.\n");
	CR(ptp_getdeviceinfo (&params, &params.deviceinfo),
		"Could not get device info\nTry to reset the camera.\n");
	propdesc=get_property_description(&params,property);
	printf("Camera: %s",params.deviceinfo.Model);
	if ((devn!=0)||(busn!=0)) 
		printf(" (bus %i, dev %i)\n",busn,devn);
	else
		printf("\n");
	if (!ptp_property_issupported(&params, property))
	{
		fprintf(stderr,"The device does not support this property!\n");
		CR(ptp_closesession(&params), "Could not close session!\n"
			"Try to reset the camera.\n");
		return;
	}
	printf("Property '%s'\n",propdesc);
	memset(&dpd,0,sizeof(dpd));
	CR(ptp_getdevicepropdesc(&params,property,&dpd),
		"Could not get device property description!\n"
		"Try to reset the camera.\n");
	printf ("Data type is 0x%04x\n",dpd.DataType);
	printf ("Current value is ");
	if (dpd.FormFlag==PTP_DPFF_Enumeration)
		PRINT_PROPVAL_DEC(dpd.CurrentValue);
	else 
		PRINT_PROPVAL_HEX(dpd.CurrentValue);
	printf("\n");
	printf ("Factory default value is ");
	if (dpd.FormFlag==PTP_DPFF_Enumeration)
		PRINT_PROPVAL_DEC(dpd.FactoryDefaultValue);
	else 
		PRINT_PROPVAL_HEX(dpd.FactoryDefaultValue);
	printf("\n");
	printf("The property is ");
	if (dpd.GetSet==PTP_DPGS_Get)
		printf ("read only");
	else
		printf ("settable");
	switch (dpd.FormFlag) {
	case PTP_DPFF_Enumeration:
		printf (", enumerated. Allowed values are:\n");
		{
			int i;
			for(i=0;i<dpd.FORM.Enum.NumberOfValues;i++){
				PRINT_PROPVAL_HEX(
				dpd.FORM.Enum.SupportedValue[i]);
				printf("\n");
			}
		}
		break;
	case PTP_DPFF_Range:
		printf (", within range:\n");
		PRINT_PROPVAL_DEC(dpd.FORM.Range.MinimumValue);
		printf(" - ");
		PRINT_PROPVAL_DEC(dpd.FORM.Range.MaximumValue);
		printf("; step size: ");
		PRINT_PROPVAL_DEC(dpd.FORM.Range.StepSize);
		printf("\n");
		break;
	case PTP_DPFF_None:
		printf(".\n");
	}
	if (value) {
		printf("Setting property value to '%s'\n",value);
		CRE(set_property(&params, property, value, dpd.DataType));
	}
	ptp_free_devicepropdesc(&dpd);
	CR(ptp_closesession(&params), "Could not close session!\n"
	"Try to reset the camera.\n");
	usb_release_interface(ptp_usb.handle,
		dev->config->interface->altsetting->bInterfaceNumber);
}

/* Added by Nikolai Kopanygin */
/*
    0. Sets up a connection with the camera.
    1. Turns the shooting mode on.
    2. Turns the viewfinder on.
    3. Sets the minimum zoom.
    4. Takes 3 viewfinder shots and saves them in the current directory.
    5. Turns the viewfinder off.
    6. Takes a photo an saves it in the current directory.
    7. Increases zoom.
    8. Repeats steps 3 - 6.
    9. Turns the shooting mode off.
    

*/

void
test_canon (int busn,int devn,int force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	PTPDevicePropDesc dpd;
    PTPUSBEventContainer event;
    PTPContainer evc;
    int isevent;
    uint32_t entnum;
    int i,k;
    
    FILE *f;
    uint16_t zoom;
    uint16_t val16;
    uint32_t dummy;
    uint32_t val32;
    uint16_t *pval16;
    uint32_t *pval32;
    char *str;

    CR(connect_camera(busn, devn, force, &ptp_usb, &params, &dev),
        "Could not connect camera!\n");
    CR(enter_shooting_mode(&params),
        "Could not enter shooting mode!\n");

    for (zoom=0; zoom<3; zoom++) {
        uint32_t handle;
        char name[17];
        char *image;
        uint32_t size;
        uint32_t dummy;
        uint16_t res;
        uint32_t bsize=5000;
        uint32_t nblocks; 
        uint32_t tail;  /* remainder */
        uint32_t pos;
        uint8_t flash;
	
        /* setting flash off */
	flash=0;
        printf("Setting Flash mode to %d\n",flash);
        CRE(ptp_setdevicepropvalue(&params, PTP_DPC_CANON_FlashMode, &flash, PTP_DTC_UINT8));
	
        /* setting zoom */
		printf("Setting Zoom value to %d\n",zoom);
        CRE(res = ptp_setdevicepropvalue(&params, PTP_DPC_CANON_Zoom, &zoom, PTP_DTC_UINT16));
        if (res==PTP_RC_OK) {
            uint16_t *props=NULL;
            uint32_t propnum;
            uint16_t *pval16;
            int i;
            
            printf("Let's see what properties changed.\n");
            CRE(res=ptp_canon_getchanges(&params, &props, &propnum));
            if (res==PTP_RC_OK) {
                for (i=0;(i<propnum)&&(i<20) ;i++) {
                    printf("changed 0x%4X\n",props[i]);
                }
                if (propnum>20) 
                    printf("........\n");
            }
            free(props);
            pval16=NULL;
            CRE(res=ptp_getdevicepropvalue(&params, PTP_DPC_CANON_Zoom, (void **)&pval16, PTP_DTC_UINT16));
            if (res==PTP_RC_OK)
                printf("Zoom is actually set to %d\n",*pval16);
            free(pval16); 
        }

        /* turn viewfinder on */
        printf("Turning viewfinder on\n");
        CRE(res = ptp_canon_viewfinderon (&params));
        if (res==PTP_RC_OK) {
            int i;
            /* take 3 VF shots */
            for (i=0; i<3; i++) {
                image=NULL;
                CRE(res=ptp_canon_getviewfinderimage (&params, &image, &size));
                if (res==PTP_RC_OK) {
                    sprintf(name,"canon_vf_%d_%d.jpg",zoom,i);
                    f = fopen(name, "w");
                    if (f) {
                        fwrite(image,1,size,f);
                        fclose(f);
                        printf("VF shot %s written\n",name);
                    }
                }
                free(image);
                usleep(500000); 
            }

            /* turn viewfinder off */        
            printf("Turning viewfinder off\n");
            CRE(ptp_canon_viewfinderoff (&params));
        }
        
        

        /* take a "real" shot */ 
        val16=3;
		CRE(ptp_setdevicepropvalue(&params, PTP_DPC_CANON_D029, &val16, PTP_DTC_UINT16));
        printf("Initiating capture...\n");
        CRE(res=ptp_canon_initiatecaptureinmemory (&params));
        if (res!=PTP_RC_OK)
            continue;
        
        /* Catch event */
        if (PTP_RC_OK==(val16=ptp_usb_event_wait (&params, &evc))) {
            if (evc.Code==PTP_EC_CaptureComplete)
                printf("Event: capture complete. \n");
            else
                printf("Event: 0x%X\n", evc.Code);
            
        } else
            printf("No event yet, we'll try later.\n");

        printf("Checking events in stack.\n");
        for (i=0;i<100;i++) {
            CRE(res=ptp_canon_checkevent(&params,&event,&isevent));
            if (res!=PTP_RC_OK)
                continue;
            if (isevent  && 
                (event.type==PTP_USB_CONTAINER_EVENT) &&
                (event.code==PTP_EC_CANON_RequestObjectTransfer) ) {
                    handle=event.param1;
                    printf("PTP_EC_CANON_RequestObjectTransfer, object handle=0x%X. \n",event.param1);
                    printf("evdata: L=0x%X, T=0x%X, C=0x%X, trans_id=0x%X, p1=0x%X, p2=0x%X, p3=0x%X\n",
                        event.length,event.type,event.code,event.trans_id,
                        event.param1, event.param2, event.param3);
                    int j;
                    for (j=0;j<2;j++) {
                        CRE(res=ptp_canon_checkevent(&params,&event,&isevent));
                        if ((res==PTP_RC_OK) && isevent)
                            printf("evdata: L=0x%X, T=0x%X, C=0x%X, trans_id=0x%X, p1=0x%X, p2=0x%X, p3=0x%X\n",
                                event.length,event.type,event.code,event.trans_id,
                                event.param1, event.param2, event.param3);
                    }
                    break;
                }
        }
        /* Catch event, attempt  2 */
        if (val16!=PTP_RC_OK) {
            if (PTP_RC_OK==ptp_usb_event_wait (&params, &evc)) {
                if (evc.Code==PTP_EC_CaptureComplete)
                    printf("Event: capture complete. \n");
                else
                    printf("Event: 0x%X\n", evc.Code);
            } else
                printf("No expected capture complete event\n");
        }
        
        if (i==100)
            fprintf(stderr,"ERROR: Capture timed out!\n");        
        else
            download_object(&params, handle, name);
    } /* next zoom */

    printf("Test passed\n");
    CR(exit_shooting_mode(&params),
        "Could not exit shooting mode!\n");
    CR(disconnect_camera(&ptp_usb, &params, dev),
        "Disconnecting failed!\n");
}

uint16_t connect_camera(int busn, int devn, uint16_t force, PTP_USB *ptp_usb, PTPParams *params, struct usb_device **dev) 
{
#ifdef DEBUG
	printf("dev %i\tbus %i\n",devn,busn);
#endif
	*dev=find_device(busn,devn,force);
	if (*dev==NULL) {
		fprintf(stderr,"could not find any device matching given "
		"bus/dev numbers\n");
		exit(-1);
	}
	find_endpoints(*dev,&ptp_usb->inep,&ptp_usb->outep,&ptp_usb->intep);
	init_ptp_usb(params, ptp_usb, *dev);
    printf("Opening session\n");
	CR2(ptp_opensession (params,1),
		"Could not open session!\nTry to reset the camera.\n");
    return PTP_RC_OK;
}

uint16_t disconnect_camera(PTP_USB *ptp_usb, PTPParams *params, struct usb_device *dev)
{
	CR2(ptp_closesession(params), "Could not close session!\n"
	"Try to reset the camera.\n");
	usb_release_interface(ptp_usb->handle,
		dev->config->interface->altsetting->bInterfaceNumber);
    return PTP_RC_OK;
}

uint16_t enter_shooting_mode(PTPParams *params)
{
    PTPStorageIDs sids;
    PTPStorageInfo sinfo;
    PTPUSBEventContainer event;
    PTPContainer evc;
    int isevent;
    int i,k;
    
    uint16_t val16;
    uint16_t *pval16;
    uint32_t *pval32;

    sids.Storage=NULL;
    sinfo.StorageDescription=NULL;
    sinfo.VolumeLabel=NULL;

    printf("Getting storage IDs\n");
    free(sids.Storage);
    CR2(ptp_getstorageids (params, &sids),
        "Could not get storage IDs.\n");

    printf("Magic code begins.\n");
    CRE2(ptp_getdevicepropvalue(params, 0xd045, (void **)&pval16, PTP_DTC_UINT16));
    printf("prop 0xd045 value is 0x%4X\n",*pval16);
    free(pval16);
    val16=1;
    CRE2(ptp_setdevicepropvalue(params, 0xd045, &val16, PTP_DTC_UINT16));
    CRE2(ptp_getdevicepropvalue(params, 0xd02e, (void **)&pval32, PTP_DTC_UINT32));
    printf("prop 0xd02e value is 0x%4X\n",*pval32);
    free(pval32);
    CRE2(ptp_getdevicepropvalue(params, 0xd02f, (void **)&pval32, PTP_DTC_UINT32));
    printf("prop 0xd02f value is 0x%4X\n",*pval32);
    free(pval32);
	CR2(ptp_getdeviceinfo (params, &params->deviceinfo),
		"Could not get device info\nTry to reset the camera.\n");
	CR2(ptp_getdeviceinfo (params, &params->deviceinfo),
		"Could not get device info\nTry to reset the camera.\n");
    CRE2(ptp_getdevicepropvalue(params, 0xd02e, (void **)&pval32, PTP_DTC_UINT32));
    printf("prop 0xd02e value is 0x%4X\n",*pval32);
    free(pval32);
    CRE2(ptp_getdevicepropvalue(params, 0xd02f, (void **)&pval32, PTP_DTC_UINT32));
    printf("prop 0xd02f value is 0x%4X\n",*pval32);
    free(pval32);
	CR2(ptp_getdeviceinfo (params, &params->deviceinfo),
		"Could not get device info\nTry to reset the camera.\n");
    CRE2(ptp_getdevicepropvalue(params, 0xd045, (void **)&pval16, PTP_DTC_UINT16));
    printf("prop 0xd045 value is 0x%4X\n",*pval16);
    free(pval16);
    printf("Magic code ends.\n");
        
    printf("Setting prop. 0xd045 to 4\n");
    val16=4;
    CR2(ptp_setdevicepropvalue(params, PTP_DPC_CANON_D045, &val16, PTP_DTC_UINT16),
        "Could not set prop. 0xd045 to 4.\n");
    
    
    printf("Getting storage IDs\n");
    free(sids.Storage);
    CR2(ptp_getstorageids (params, &sids),
        "Could not get storage IDs.\n");
    printf("sids.n=%d\n",sids.n);


    for (k=0; k<sids.n; k++) {
        printf("Getting Storage 0x%X description.\n",sids.Storage[k]);
        free(sinfo.StorageDescription);
        free(sinfo.VolumeLabel);
        CR2(ptp_getstorageinfo (params, sids.Storage[k],&sinfo),
            "Could not get storage info.\n");
        printf("Volume label is '%s'\n",sinfo.VolumeLabel);    
    }
    free(sids.Storage);

    printf("Entering shooting mode\n");
    CR2(ptp_canon_startshootingmode (params),
        "Could not start shooting mode.\n");

    /* Catch event */
    if (PTP_RC_OK==(val16=ptp_usb_event_wait (params, &evc))) {
        if (evc.Code==PTP_EC_StorageInfoChanged)
            printf("Event: entering  shooting mode. \n");
        else 
            printf("Event: 0x%X\n", evc.Code);
    } else {
        printf("No event yet, we'll try later.\n");
    }
    
    /* Emptying event stack */
    for (i=0;i<2;i++) {
        CRE2(ptp_canon_checkevent(params,&event,&isevent));
        if (isevent)
            printf("evdata: L=0x%X, T=0x%X, C=0x%X, trans_id=0x%X, p1=0x%X, p2=0x%X, p3=0x%X\n",
                event.length,event.type,event.code,event.trans_id,
                event.param1, event.param2, event.param3);
    }
    /* Catch event, attempt  2 */
    if (val16!=PTP_RC_OK) {
        if (PTP_RC_OK==ptp_usb_event_wait (params, &evc)) {
            if (evc.Code==PTP_EC_StorageInfoChanged)
                printf("Event: entering  shooting mode.\n");
            else
                printf("Event: 0x%X\n", evc.Code);
        } else
            printf("No expected mode change event.\n");
    }
    return PTP_RC_OK;
}

uint16_t exit_shooting_mode(PTPParams *params) 
{
    PTPUSBEventContainer event;
    PTPContainer evc;
    int isevent;
    int i;
    
    uint16_t val16;
    uint16_t *pval16;
    uint32_t *pval32;

    CR2(ptp_canon_endshootingmode (params),
        "Could not exit shooting mode!\n");
    /* Catch event */
    if (PTP_RC_OK==(val16=ptp_usb_event_wait (params, &evc))) {
        if (evc.Code==PTP_EC_StorageInfoChanged)
            printf("Event: exiting shooting mode. \n");
        else 
            printf("Event: 0x%X\n", evc.Code);
    } else
        printf("No event yet, we'll try later.\n");

    /* Emptying event stack */
    for (i=0;i<2;i++) {
        CRE2(ptp_canon_checkevent(params,&event,&isevent));
        if (isevent)
            printf("evdata: L=0x%X, T=0x%X, C=0x%X, trans_id=0x%X, p1=0x%X, p2=0x%X, p3=0x%X\n",
                event.length,event.type,event.code,event.trans_id,
                event.param1, event.param2, event.param3);
    }
    /* Catch event, attempt  2 */
    if (val16!=PTP_RC_OK) {
        if (PTP_RC_OK==ptp_usb_event_wait (params, &evc)) {
            if (evc.Code==PTP_EC_StorageInfoChanged)
                printf("Event: exiting  shooting mode.\n");
            else
                printf("Event: 0x%X\n", evc.Code);
        } else
            printf("No expected mode change event.\n");
    }
    return PTP_RC_OK;
}


uint16_t download_object(PTPParams *params, uint32_t handle, char *filename)
{
    FILE *f;
    char *image;
    uint32_t dummy;
    uint32_t size;
    uint16_t res;
    uint32_t bsize=5000;
    uint32_t nblocks; 
    uint32_t tail;  /* remainder */
    uint32_t pos;
    int i;
    
    CRE2(res=ptp_canon_getobjectsize (params, handle, 0, &size, &dummy));
    if (res!=PTP_RC_OK)
        return res;
    if (size==0) {
        fprintf(stderr, "Empty object\n");
        return res;
    }
    printf("Downloading %s...\n", filename);
    f = fopen(filename,"w");
    if (!f) {
        fprintf(stderr, "Cannot open file %s\n", filename);
        return res;
    }
    nblocks=size/bsize; /* full blocks */
    tail=size%bsize;
    for(i=0; i<nblocks; i++) {
        int readnum;
        pos = i==0?1:((tail==0)&&(i==nblocks-1))?3:2;
        image=NULL;
        CRE2(res=ptp_canon_getpartialobject (params, handle, i*bsize, bsize, pos, &image, &readnum));
        if (res==PTP_RC_OK) {
            if (readnum!=bsize)
                printf("Image block read error\n");
            fwrite(image,1,readnum,f);
        }
        free(image);
    }
    /* the last block */
    if (tail!=0) {
        uint32_t readnum;
        pos=nblocks==0?1:3;
        image=NULL;
        CRE2(res=ptp_canon_getpartialobject (params, handle, nblocks*bsize, tail, pos, &image, &readnum));
        if (res==PTP_RC_OK) {
            if (readnum!=tail)
                printf("Image remainder read error\n");
            fwrite(image,1,readnum,f);
        }
        free(image);
    }
    fclose(f);
    printf("Image %s saved\n", filename);
    return PTP_RC_OK;
}


void take_shot (int busn, int devn,uint16_t force, char* filename, uint8_t flash, uint16_t zoom, int on_button)
{
    PTP_USB ptp_usb;
    struct usb_device *dev;
    PTPParams params;
    PTPUSBEventContainer event;
    PTPContainer evc;
    int isevent;
    uint32_t entnum;
    int i;
    uint16_t res;
    uint16_t val16;
    uint32_t val32;
    uint16_t *pval16;
    uint32_t *pval32;
    uint32_t handle;

    CR(connect_camera(busn, devn, force, &ptp_usb, &params, &dev),
        "Could not connect camera!\n");
    CR(enter_shooting_mode(&params),
        "Could not enter shooting mode!\n");

    if (on_button) while(1) {
        CRE(ptp_canon_checkevent(&params,&event,&isevent));
        if (isevent && event.code==0xc00b) {
                printf("Event: Shot pressed. \n");
                break;
        } else if (isevent && event.length>0) {
            printf("evdata: L=0x%X, T=0x%X, C=0x%X, trans_id=0x%X, p1=0x%X, p2=0x%X, p3=0x%X\n",
                event.length,event.type,event.code,event.trans_id,
                event.param1, event.param2, event.param3);
        }
        usleep(100000);
    }

    printf("Setting Flash mode to %d\n",flash);
    CRE(ptp_setdevicepropvalue(&params, PTP_DPC_CANON_FlashMode, &flash, PTP_DTC_UINT8));
    printf("Setting Zoom value to %d\n",zoom);
    CRE(ptp_setdevicepropvalue(&params, PTP_DPC_CANON_Zoom, &zoom, PTP_DTC_UINT16));

    /* take a "real" shot */ 
    val16=3;
	CRE(ptp_setdevicepropvalue(&params, PTP_DPC_CANON_D029, &val16, PTP_DTC_UINT16));
    printf("Initiating capture...\n");
    CRE(res=ptp_canon_initiatecaptureinmemory (&params));
    if (res==PTP_RC_OK) {
        /* Catch event */
        if (PTP_RC_OK==(val16=ptp_usb_event_wait (&params, &evc))) {
            if (evc.Code==PTP_EC_CaptureComplete)
                printf("Event: capture complete. \n");
            else
                printf("Event: 0x%X\n", evc.Code);
            
        } else
            printf("No event yet, we'll try later.\n");

        printf("Checking events in stack.\n");
        for (i=0;i<100;i++) {
            CRE(res=ptp_canon_checkevent(&params,&event,&isevent));
            if (res!=PTP_RC_OK)
                continue;
            if (isevent  && 
                (event.type==PTP_USB_CONTAINER_EVENT) &&
                (event.code==PTP_EC_CANON_RequestObjectTransfer) ) {
                    handle=event.param1;
                    printf("PTP_EC_CANON_RequestObjectTransfer, object handle=0x%X. \n",event.param1);
                    printf("evdata: L=0x%X, T=0x%X, C=0x%X, trans_id=0x%X, p1=0x%X, p2=0x%X, p3=0x%X\n",
                        event.length,event.type,event.code,event.trans_id,
                        event.param1, event.param2, event.param3);
                    int j;
                    for (j=0;j<2;j++) {
                        CRE(res=ptp_canon_checkevent(&params,&event,&isevent));
                        if ((res==PTP_RC_OK) && isevent)
                            printf("evdata: L=0x%X, T=0x%X, C=0x%X, trans_id=0x%X, p1=0x%X, p2=0x%X, p3=0x%X\n",
                                event.length,event.type,event.code,event.trans_id,
                                event.param1, event.param2, event.param3);
                    }
                    break;
                }
        }
        /* Catch event, attempt  2 */
        if (val16!=PTP_RC_OK) {
            if (PTP_RC_OK==ptp_usb_event_wait (&params, &evc)) {
                if (evc.Code==PTP_EC_CaptureComplete)
                    printf("Event: capture complete. \n");
                else
                    printf("Event: 0x%X\n", evc.Code);
            } else
                printf("No expected capture complete event\n");
        }
        
        if (i==100)
            fprintf(stderr,"ERROR: Capture timed out!\n");        
        else
            download_object(&params, handle, filename);
    }
    
    CR(exit_shooting_mode(&params),
        "Could not exit shooting mode!\n");
    CR(disconnect_camera(&ptp_usb, &params, dev),
        "Disconnecting failed!\n");
}

void
reset_device (int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	uint16_t status;
	uint16_t devstatus[2] = {0,0};
	int ret;

#ifdef DEBUG
	printf("dev %i\tbus %i\n",devn,busn);
#endif
	dev=find_device(busn,devn,force);
	if (dev==NULL) {
		fprintf(stderr,"could not find any device matching given "
		"bus/dev numbers\n");
		exit(-1);
	}
	find_endpoints(dev,&ptp_usb.inep,&ptp_usb.outep,&ptp_usb.intep);

	init_ptp_usb(&params, &ptp_usb, dev);

	// get device status (devices likes that regardless of its result)
	usb_ptp_get_device_status(&ptp_usb,devstatus);
	
	// check the in endpoint status
	ret = usb_get_endpoint_status(&ptp_usb,ptp_usb.inep,&status);
	if (ret<0) perror ("usb_get_endpoint_status()");
	// and clear the HALT condition if happend
	if (status) {
		printf("Resetting input pipe!\n");
		ret=usb_clear_stall_feature(&ptp_usb,ptp_usb.inep);
		if (ret<0)perror ("usb_clear_stall_feature()");
	}
	status=0;
	// check the out endpoint status
	ret = usb_get_endpoint_status(&ptp_usb,ptp_usb.outep,&status);
	if (ret<0) perror ("usb_get_endpoint_status()");
	// and clear the HALT condition if happend
	if (status) {
		printf("Resetting output pipe!\n");
		ret=usb_clear_stall_feature(&ptp_usb,ptp_usb.outep);
		if (ret<0)perror ("usb_clear_stall_feature()");
	}
	status=0;
	// check the interrupt endpoint status
	ret = usb_get_endpoint_status(&ptp_usb,ptp_usb.intep,&status);
	if (ret<0)perror ("usb_get_endpoint_status()");
	// and clear the HALT condition if happend
	if (status) {
		printf ("Resetting interrupt pipe!\n");
		ret=usb_clear_stall_feature(&ptp_usb,ptp_usb.intep);
		if (ret<0)perror ("usb_clear_stall_feature()");
	}

	// get device status (now there should be some results)
	ret = usb_ptp_get_device_status(&ptp_usb,devstatus);
	if (ret<0) 
		perror ("usb_ptp_get_device_status()");
	else	{
		if (devstatus[1]==PTP_RC_OK) 
			printf ("Device status OK\n");
		else
			printf ("Device status 0x%04x\n",devstatus[1]);
	}
	
	// finally reset the device (that clears prevoiusly opened sessions)
	ret = usb_ptp_device_reset(&ptp_usb);
	if (ret<0)perror ("usb_ptp_device_reset()");
	// get device status (devices likes that regardless of its result)
	usb_ptp_get_device_status(&ptp_usb,devstatus);

	usb_release_interface(ptp_usb.handle,
		dev->config->interface->altsetting->bInterfaceNumber);
}

/* main program  */

int
main(int argc, char ** argv)
{
	int busn=0,devn=0;
    
	int flash=0;
	int zoom=0;
    int on_button=0;
    
	int action=0;
	short force=0;
	uint16_t property=0;
	char* value=NULL;
    char filename[MAXNAMELEN];
	/* parse options */
	int option_index = 0,opt;
	static struct option loptions[] = {
		{"help",0,0,'h'},
		{"bus",1,0,'B'},
		{"dev",1,0,'D'},
		{"reset",0,0,'r'},
		{"list-devices",0,0,'l'},
		{"list-properties",0,0,'p'},
		{"show-property",1,0,'s'},
		{"set-property",1,0,'s'},
		{"val",1,0,0},
		{"force",0,0,'f'},
		{"verbose",2,0,'v'},
		{"canon-test",0,0,'c'},
		{"take-shot",0,0,'t'},
		{"name",1,0,'N'},
		{"flash",1,0,'F'},
		{"zoom",1,0,'Z'},
		{"on-button",0,0,'b'},
		{0,0,0,0}
	};
    
    strcpy(filename,"noname.jpg");
	
	while(1) {
		opt = getopt_long (argc, argv, "hlpfrctbs:D:N:F:B:Z:v::", loptions, &option_index);
		if (opt==-1) break;
	
		switch (opt) {
		case 0:
			if (!(strcmp("val",loptions[option_index].name)))
				value=strdup(optarg);
			break;
		case 'h':
			help();
			break;
		case 'B':
			busn=strtol(optarg,NULL,10);
			break;
		case 'D':
			devn=strtol(optarg,NULL,10);
			break;
		case 'f':
			force=~force;
			break;
		case 'v':
			if (optarg) 
				ptp_usb_verbose=strtol(optarg,NULL,10);
			else
				ptp_usb_verbose=1;
			printf("VERBOSE LEVEL  = %i\n",ptp_usb_verbose);
			break;
		case 'r':
			action=ACT_DEVICE_RESET;
			break;
		case 'l':
			action=ACT_LIST_DEVICES;
			break;
		case 'p':
			action=ACT_LIST_PROPERTIES;
			break;
		case 's':
			action=ACT_GETSET_PROPERTY;
			property=strtol(optarg,NULL,16);
			break;
		case 'c':
			action=ACT_CANON_TEST;
			break;
		case 't':
			action=ACT_TAKE_SHOT;
			break;
		case 'N':
			strncpy(filename,optarg,MAXNAMELEN);
            filename[MAXNAMELEN-1]='\0';
			break;
		case 'F':
			flash=strtol(optarg,NULL,10);
			break;
		case 'Z':
			zoom=strtol(optarg,NULL,10);
			break;
		case 'b':
			on_button=1;
			break;
		case '?':
			break;
		default:
			fprintf(stderr,"getopt returned character code 0%o\n",
				opt);
			break;
		}
	}
	if (argc==1) {
		usage();
		return 0;
	}
	switch (action) {
		case ACT_DEVICE_RESET:
			reset_device(busn,devn,force);
			break;
		case ACT_LIST_DEVICES:
			list_devices(force);
			break;
		case ACT_LIST_PROPERTIES:
			list_properties(busn,devn,force);
			break;
		case ACT_GETSET_PROPERTY:
			getset_property(busn,devn,property,value,force);
			break;
		case ACT_CANON_TEST:
            test_canon (busn,devn,force);
			break;
		case ACT_TAKE_SHOT:
            take_shot (busn,devn,force,filename,flash,zoom,on_button);
			break;
	}

	return 0;
}
