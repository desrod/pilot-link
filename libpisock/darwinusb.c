/*
 * darwinusb.c: I/O support for Darwin (Mac OS X) USB
 *
 * Copyright (c) 2004, Florent Pillet.
 *
 * libpisock interface modeled after linuxusb.c by Jeff Dionne and 
 * Kenneth Albanowski
 * Some structures & defines extracted from Linux "visor.c",
 * which is Copyright (C) 1999 - 2003 Greg Kroah-Hartman (greg@kroah.com)
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */

#include <mach/mach.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

#include "pi-debug.h"
#include "pi-socket.h"
#include "pi-source.h"
#include "pi-usb.h"
#include "pi-error.h"

/*
 * Theory of operation
 *
 * Darwin IOKit is different from traditional unix i/o. It is much more
 * structured, and also more complex.
 *
 * One of the strengths of IOKit is IOUSBLib which allows talking to USB
 * devices directly from userland code, without the need to write a driver.
 * This is the way we do it here.
 *
 * Here is what we do:
 * - We start a separate thread which will handle the USB communications.  The
 *   main (controlling) thread exposes start, stop, poll, read and write
 *   functions. These function take care of controlling the USB thread.
 * - We register for "device added" notifications. These notifications are
 *   sent by the IOKit to registered clients when a new device shows up. We
 *   use a matching dictionary to restrict the devices we're interested in to
 *   USB devices (IOUSBDevice class)
 * - When we get notified that a new device showed up, we check the USB vendor
 *   ID and product ID and only accept those that are known to be Palm OS
 *   devices
 * - We then examine the device interfaces and select the pipes we're going to
 *   use for input and output [1].
 * - We register for notifications coming from the device. When the device
 *   goes away, our notification callback is called and we can cleanly close
 *   things.
 * - Once everything is initialized, we fire a first "read" from the read
 *   pipe.  Subsequent reads are fired directly from the previous read's
 *   completion routine [2].
 * - In case the thread or application is aborted, the IOKit will clean things
 *   up for us.
 * - The controlling thread can also call darwin_usb_stop_listening() which
 *   will close the USB connection and release all buffers.
 *
 *
 * [1] Usually, Palm OS devices expose 4 bulk pipes: 2 "in" and 2 "out". One
 *     of the issues we face is to guess which pipe to use to receive data,
 *     and which pipe to use to send data. Some devices reply to the extended
 *     connection information request, which provides information about the
 *     endpoint to use for synchronization, but some others don't. In this
 *     case we're pretty much on our one, trying to guess which pipe to use.
 *
 * [2] Reading is done asynchronously and in a chained way: we fire a read
 *     with ReadPipeAsync(). Once the read completes (or fails), our
 *     completion routine is called. As long as the read is not "aborted"
 *     (which means the device has been disconnected), we fire another read
 *     from the read_completion() function.
 *
 *     All read data fills a buffer which is independantly consumed by the
 *     main thread.  This way, we get the maximum read throughput by making
 *     sure that any data received from the device is fetched as soon as
 *     possible and made available to the main thread.
 *
 *     Writes, in the contrary, are synchronous for now. I can make them async
 *     as well, though I have not explored the implications for the libpisock
 *     code yet.  This could speed things up a bit as well, though.
 *
 */

/* Define this to make debug logs include USB debug info */
#undef DEBUG_USB

/*
 * These values are somewhat tricky.  Priming reads with a size of exactly one
 * USB packet works best (no timeouts).  Probably best to leave these as they are.
 */
#define MAX_AUTO_READ_SIZE	16384
#define AUTO_READ_SIZE		64


/* Hardware interface */
static IONotificationPortRef usb_notify_port;
static IOUSBInterfaceInterface **usb_interface;
static IOUSBDeviceInterface **usb_device;	/* kept for CLOSE_NOTIFICATION */
static io_iterator_t usb_device_added_iter;
static io_object_t usb_device_notification;	/* for device removal */

static int usb_opened = 0;
static int usb_in_pipe_ref = 0;				/* pipe for reads */
static int usb_out_pipe_ref = 0;			/* pipe for writes */


static int usb_auto_read_size = 0;			/* if != 0, prime reads to the input pipe to get data permanently */

/* these provide hints about the size of the next read */
static int usb_read_ahead_size = 0;			/* when waiting for big chunks of data, used as a hint to make bigger read requests */
static int usb_last_read_ahead_size;		/* also need this to properly compute the size of the next read */


static char usb_read_buffer[MAX_AUTO_READ_SIZE];

static pthread_mutex_t read_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t read_queue_data_avail_cond = PTHREAD_COND_INITIALIZER;
static char *read_queue = NULL;				/* stores completed reads, grows by 64k chunks */
static size_t read_queue_size = 0;
static size_t read_queue_used = 0;

static pthread_t usb_thread = 0;
static CFRunLoopRef usb_run_loop = 0;
static pthread_mutex_t usb_run_loop_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t usb_thread_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t usb_thread_ready_cond = PTHREAD_COND_INITIALIZER;

static IOReturn control_request (IOUSBDeviceInterface **dev, UInt8 requestType, UInt8 request, void *pData, UInt16 maxReplyLength);

static void device_added (void *refCon, io_iterator_t iterator);
static void device_notification (void *refCon, io_service_t service, natural_t messageType, void *messageArgument);
static void read_completion (void *refCon, IOReturn result, void *arg0);

static int accepts_device (unsigned short vendor, unsigned short product);
static IOReturn configure_device (IOUSBDeviceInterface **dev, unsigned short vendor, unsigned short product, UInt8 *inputPipeNumber, UInt8 *outputPipeNumber);
static IOReturn find_interfaces (IOUSBDeviceInterface **dev, unsigned short vendor, unsigned short product, UInt8 inputPipeNumber, UInt8 outputPipeNumber);
static int prime_read (void);

static IOReturn read_visor_connection_information (IOUSBDeviceInterface **dev);
static IOReturn read_generic_connection_information (IOUSBDeviceInterface **dev, UInt8 *inputPipeNumber, UInt8 *outputPipeNumber);


//
// USB control requests we send to the devices
// Got them from linux/drivers/usb/serial/visor.h
//
#define	GENERIC_REQUEST_BYTES_AVAILABLE			0x01
#define	GENERIC_CLOSE_NOTIFICATION				0x02
#define VISOR_GET_CONNECTION_INFORMATION		0x03
#define PALM_GET_EXT_CONNECTION_INFORMATION		0x04

//
// Structures defining the info a device returns
// Got them from linux/drivers/usb/serial/visor.h
//
typedef struct
{
	UInt16 num_ports;
	struct
	{
		UInt8 port_function_id;
		UInt8 port;
	} connections[2];
} visor_connection_info;

/* struct visor_connection_info.connection[x].port defines: */
#define VISOR_ENDPOINT_1		0x01
#define VISOR_ENDPOINT_2		0x02

/* struct visor_connection_info.connection[x].port_function_id defines: */
#define VISOR_FUNCTION_GENERIC				0x00
#define VISOR_FUNCTION_DEBUGGER				0x01
#define VISOR_FUNCTION_HOTSYNC				0x02
#define VISOR_FUNCTION_CONSOLE				0x03
#define VISOR_FUNCTION_REMOTE_FILE_SYS		0x04

typedef struct
{
	UInt8 num_ports;
	UInt8 endpoint_numbers_different;
	UInt16 reserved1;
 	struct
	{
		UInt32 port_function_id;
		UInt8 port;
		UInt8 endpoint_info;
		UInt16 reserved;
    } connections[2];
} palm_ext_connection_info;

//
// Some vendor and product codes we use
//
#define	VENDOR_SONY					0x054c
#define	VENDOR_HANDSPRING			0x082d
#define	VENDOR_PALMONE				0x0830
#define	VENDOR_TAPWAVE				0x12ef
#define PRODUCT_PALMCONNECT_USB		0x0080
#define	PRODUCT_HANDSPRING_VISOR	0x0100
#define PRODUCT_SONY_CLIE_3_5		0x0038

//
// This table helps us determine whether a connecting USB device is
// one we'd like to talk to.
//
static struct {
	unsigned short vendorID;
	unsigned short productID;
}
acceptedDevices[] = {
	/* Sony */
	{0x054c, 0x0038},	// Sony Palm OS 3.5 devices
	{0x054c, 0x0066},	// Sony T, S320, SJ series, and other Palm OS 4.0 devices
	{0x054c, 0x0095},	// Sony S360
	{0x054c, 0x000a},	// Sony NR and other Palm OS 4.1 devices
	{0x054c, 0x009a},	// Sony NR70V/U
	{0x054c, 0x00da},	// Sony NX
	{0x054c, 0x00e9},	// Sony NZ
	{0x054c, 0x0144},	// Sony UX
	{0x054c, 0x0169},	// Sony TJ

	/* AlphaSmart */
	{0x081e, 0xdf00},   // Dana?

	/* HANDSPRING (vendor 0x082d) */
	{0x082d, 0x0100},	// Visor, Treo 300
	{0x082d, 0x0200},	// Treo
	{0x082d, 0x0300},	// Treo 600

	/* PalmOne, Palm Inc */
	{0x0830, 0x0001},	// m500
	{0x0830, 0x0002},	// m505
	{0x0830, 0x0003},	// m515
	{0x0830, 0x0010},
	{0x0830, 0x0011},
	{0x0830, 0x0020},	// i705
	{0x0830, 0x0030},
	{0x0830, 0x0031},	// Tungsten|W
	{0x0830, 0x0040},	// m125
	{0x0830, 0x0050},	// m130
	{0x0830, 0x0051},
	{0x0830, 0x0052},
	{0x0830, 0x0053},
	{0x0830, 0x0060},	// Tungsten series, Zire 71
	{0x0830, 0x0061},	// Zire 31, 72
	{0x0830, 0x0062},
	{0x0830, 0x0063},
	{0x0830, 0x0070},	// Zire
	{0x0830, 0x0071},
	{0x0830, 0x0080},	// serial adapter
	{0x0830, 0x0099},
	{0x0830, 0x0100},

	/* GARMIN */
	{0x091e, 0x0004},	// IQUE 3600

	/* Kyocera */
	{0x0c88, 0x0021},	// 7135 Smartphone
	{0x0c88, 0xa226},	// 6035 Smartphone

	/* Tapwave */
	{0x12ef, 0x0100},	// Zodiac, Zodiac2

	/* ACEECA */
	{0x4766, 0x0001},	// MEZ1000

	/* Samsung */
	{0x04e8, 0x8001}	// I330
};


/***************************************************************************/
/*                                                                         */
/*                             INTERNAL ROUTINES                           */
/*                                                                         */
/***************************************************************************/

static int
start_listening(void)
{
	mach_port_t masterPort;
	CFMutableDictionaryRef matchingDict;
	CFRunLoopSourceRef runLoopSource;
	kern_return_t kr;

	usb_notify_port = NULL;
	usb_device_added_iter = NULL;
	usb_device_notification = NULL;
	usb_interface = NULL;
	usb_device = NULL;

	usb_in_pipe_ref = 0;
	usb_out_pipe_ref = 0;
	usb_auto_read_size = AUTO_READ_SIZE;
	usb_read_ahead_size = 0;
	usb_last_read_ahead_size = 0;
	usb_opened = 0;
	read_queue_size = 0;
	read_queue_used = 0;

	// first create a master_port for my task
	kr = IOMasterPort (MACH_PORT_NULL, &masterPort);
	if (kr || !masterPort)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: couldn't create a master IOKit Port(%08x)\n", kr));
		return PI_ERR_GENERIC_SYSTEM;
	}

	// Set up the matching criteria for the devices we're interested in
	// Interested in instances of class IOUSBDevice and its subclasses
	// Since we are supporting many USB devices, we just get notifications
	// for all USB devices and sort out the ones that we want later.
	matchingDict = IOServiceMatching (kIOUSBDeviceClassName);
	if (!matchingDict)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: can't create a USB matching dictionary\n"));
		mach_port_deallocate (mach_task_self(), masterPort);
		return PI_ERR_GENERIC_SYSTEM;
	}

	// Create a notification port and add its run loop event source to our run loop
	// This is how async notifications get set up.
	usb_notify_port = IONotificationPortCreate (masterPort);
	runLoopSource = IONotificationPortGetRunLoopSource (usb_notify_port);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);

	// Now set up two notifications, one to be called when a raw device is first matched by I/O Kit, and the other to be
	// called when the device is terminated.
	kr = IOServiceAddMatchingNotification (	usb_notify_port,
			kIOFirstMatchNotification,
			matchingDict,
			device_added,
			NULL,
			&usb_device_added_iter);

	// Iterate once to get already-present devices and arm the notification
	device_added (NULL, usb_device_added_iter);

	// Now done with the master_port
	mach_port_deallocate (mach_task_self(), masterPort);
	
	return 0;
}

static void
stop_listening(void)
{
	if (usb_device_notification)
	{
		// do this first to not get caught in a loop if called from device_notification()
		IOObjectRelease (usb_device_notification);
		usb_device_notification = NULL;
	}

	if (usb_opened && usb_device)
		control_request (usb_device, 0xc2, GENERIC_CLOSE_NOTIFICATION, NULL, 0x12);

	if (usb_interface)
	{
		(*usb_interface)->USBInterfaceClose (usb_interface);
		(*usb_interface)->Release (usb_interface);
	}

	if (usb_device)
	{
		(*usb_device)->USBDeviceClose (usb_device);
		(*usb_device)->Release (usb_device);
	}

	if (usb_device_added_iter)
	{
		IOObjectRelease (usb_device_added_iter);
		usb_device_added_iter = NULL;
	}

	if (usb_notify_port)
	{
		IONotificationPortDestroy(usb_notify_port);
		usb_notify_port = NULL;
	}

	usb_interface = NULL;
	usb_device = NULL;
	usb_opened = 0;
	usb_in_pipe_ref = 0;
	usb_out_pipe_ref = 0;
}

static void *
usb_thread_run(void *foo)
{
	if (start_listening() == 0)
	{
		// obtain the CFRunLoop for this thread
		pthread_mutex_lock(&usb_run_loop_mutex);
		usb_run_loop = CFRunLoopGetCurrent();
		pthread_mutex_unlock(&usb_run_loop_mutex);

		// signal main thread that init was successful
		pthread_mutex_lock(&usb_thread_ready_mutex);
		pthread_cond_signal(&usb_thread_ready_cond);
		pthread_mutex_unlock(&usb_thread_ready_mutex);

		CFRunLoopRun();

		pthread_mutex_lock(&usb_run_loop_mutex);
		usb_run_loop = 0;
		pthread_mutex_unlock(&usb_run_loop_mutex);

		stop_listening();
	}
	else
	{
		// signal main thread that init failed
		usb_thread = 0;
		usb_run_loop = NULL;
		pthread_mutex_lock(&usb_thread_ready_mutex);
		pthread_cond_signal(&usb_thread_ready_cond);
		pthread_mutex_unlock(&usb_thread_ready_mutex);
	}
	return NULL;
}

static void
device_added (void *refCon, io_iterator_t iterator)
{
	kern_return_t kr;
	io_service_t ioDevice;
	IOCFPlugInInterface **plugInInterface = NULL;
	IOUSBDeviceInterface **dev = NULL;
	HRESULT res;
	SInt32 score;
	UInt16 vendor, product;
	UInt8 inputPipeNumber, outputPipeNumber;

	while ((ioDevice = IOIteratorNext (iterator)))
	{
		if (usb_opened)
		{
			// we can only handle one connection at once
			IOObjectRelease (ioDevice);
			continue;
		}

		kr = IOCreatePlugInInterfaceForService (ioDevice,
				kIOUSBDeviceUserClientTypeID,
				kIOCFPlugInInterfaceID,
				&plugInInterface,
				&score);
		if (kr != kIOReturnSuccess || !plugInInterface)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: -> unable to create a plugin (kr=0x%08x)\n", kr));
			IOObjectRelease (ioDevice);
			continue;
		}

		res = (*plugInInterface)->QueryInterface (plugInInterface, CFUUIDGetUUIDBytes (kIOUSBDeviceInterfaceID), (LPVOID *)&dev);
		(*plugInInterface)->Release (plugInInterface);
		if (res || !dev)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: couldn't create a device interface (res=0x%08x)\n", (int) res));
			IOObjectRelease (ioDevice);
			continue;
		}

		kr = (*dev)->GetDeviceVendor (dev, &vendor);
		kr = (*dev)->GetDeviceProduct (dev, &product);
		if (accepts_device(vendor, product) == 0)
		{
			(*dev)->Release(dev);
			IOObjectRelease (ioDevice);
			continue;
		}

		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: Accepted USB device, vendor: 0x%04x product: 0x%04x\n", vendor, product));

		kr = (*dev)->USBDeviceOpen (dev);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to open device (kr=0x%08x)\n", kr));
			(*dev)->Release(dev);
			IOObjectRelease (ioDevice);
			continue;
		}

		kr  = (*dev)->ResetDevice (dev);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to reset device (kr=0x%08x)\n", kr));
			(*dev)->Release(dev);
			IOObjectRelease (ioDevice);
			continue;
		}

		kr = configure_device (dev, vendor, product, &inputPipeNumber, &outputPipeNumber);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to configure device (kr=0x%08x)\n", kr));
			(*dev)->USBDeviceClose (dev);
			(*dev)->Release (dev);
			IOObjectRelease (ioDevice);
			continue;
		}

		kr = find_interfaces(dev, vendor, product, inputPipeNumber, outputPipeNumber);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to find interfaces (kr=0x%08x)\n", kr));
			(*dev)->USBDeviceClose (dev);
			(*dev)->Release (dev);
			IOObjectRelease (ioDevice);
			continue;
		}

		// Register for an interest notification for this device,
		// so we get notified when it goes away
		kr = IOServiceAddInterestNotification(  usb_notify_port,
				ioDevice,
				kIOGeneralInterest,
				device_notification,
				NULL,
				&usb_device_notification);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable register for device interest notification (kr=0x%08x)\n", kr));
		}

		usb_device = dev;
		IOObjectRelease(ioDevice);
		break;
	}
}

static IOReturn
configure_device(IOUSBDeviceInterface **dev, unsigned short vendor, unsigned short product, UInt8 *inputPipeNumber, UInt8 *outputPipeNumber)
{
	UInt8 numConf;
	IOReturn kr;
	IOUSBConfigurationDescriptorPtr confDesc;

	*inputPipeNumber = 0xff;
	*outputPipeNumber = 0xff;

	kr = (*dev)->GetNumberOfConfigurations (dev, &numConf);
	if (!numConf)
		return -1;

	kr = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &confDesc);
	if (kr)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to get config descriptor for index %d (err = %08x)\n", 0, kr));
		return kr;
	}

	kr = (*dev)->SetConfiguration(dev, confDesc->bConfigurationValue);
	if (kr)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to set configuration to value %d (err=%08x)\n", (int)confDesc->bConfigurationValue, kr));
		return kr;
	}

	/*
	 * Device specific magic incantations
	 *
	 * Many devices agree on talking only if you say the "magic" incantation first.
	 * Usually, it's a control request or a sequence of control requests
	 *
	 */
	if (vendor == VENDOR_PALMONE && product == PRODUCT_PALMCONNECT_USB)
	{
		// PalmConnect USB is a serial <-> USB adapter. Even though it shows up
		// as a USB device, it really requires talking using a serial protocol
		return kIOReturnSuccess;
	}

	if (vendor == VENDOR_HANDSPRING && product == PRODUCT_HANDSPRING_VISOR)
	{
		kr = read_visor_connection_information (dev);
	}
	else if (vendor == VENDOR_SONY && product == PRODUCT_SONY_CLIE_3_5) {
		// according to linux code, PEG S-300 awaits these two requests
		kr = control_request (dev, USBmakebmRequestType(kUSBIn, kUSBStandard, kUSBDevice), 0x08 /* USB_REQ_GET_CONFIGURATION */, NULL, 1);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: Sony USB_REQ_GET_CONFIGURATION failed (err=%08x)\n", kr));
		}
		kr = control_request (dev, USBmakebmRequestType(kUSBIn, kUSBStandard, kUSBDevice), 0x0A /* USB_REQ_GET_INTERFACE */, NULL, 1);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: Sony USB_REQ_GET_INTERFACE failed (err=%08x)\n", kr));
		}
	}
	else
	{
		// other devices will either accept or deny this generic call
		kr = read_generic_connection_information (dev, inputPipeNumber, outputPipeNumber);

		if (vendor == VENDOR_TAPWAVE)
		{
			// Tapwave: for Zodiac, the TwUSBD.sys driver on Windows sends the ext-connection-info packet
			// two additional times.
			read_generic_connection_information (dev, NULL, NULL);
			read_generic_connection_information (dev, NULL, NULL);
		}
	}

	// query bytes available. Not that we really care,
	// but most devices expect to receive this before
	// they agree on talking to us.
	if (vendor != VENDOR_TAPWAVE)
	{
		unsigned char ba[2];
		kr = control_request (dev, 0xc2, GENERIC_REQUEST_BYTES_AVAILABLE, &ba[0] , 2);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: GENERIC_REQUEST_BYTES_AVAILABLE failed (err=%08x)\n", kr));
		}
#ifdef DEBUG_USB
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "GENERIC_REQUEST_BYTES_AVAILABLE returns 0x%02x%02x\n", ba[0], ba[1]));
#endif
	}

    return kIOReturnSuccess;
}

static IOReturn
find_interfaces(IOUSBDeviceInterface **dev, unsigned short vendor, unsigned short product, UInt8 inputPipeNumber, UInt8 outputPipeNumber)
{
	IOReturn kr;
	io_iterator_t iterator;
	io_service_t usbInterface;
	HRESULT res;
	SInt32 score;
	UInt8 intfClass;
	UInt8 intfSubClass;
	UInt8 intfNumEndpoints;
	int pipeRef;
	IOUSBFindInterfaceRequest request;
	IOCFPlugInInterface **plugInInterface = NULL;

	request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
	request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
	request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
	request.bAlternateSetting = kIOUSBFindInterfaceDontCare;

	kr = (*dev)->CreateInterfaceIterator (dev, &request, &iterator);

	while ((usbInterface = IOIteratorNext (iterator)))
	{
		kr = IOCreatePlugInInterfaceForService (usbInterface, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
		kr = IOObjectRelease (usbInterface);				// done with the usbInterface object now that I have the plugin
		if (kr != kIOReturnSuccess || !plugInInterface)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to create a plugin (%08x)\n", kr));
			continue;
		}

		// we have the interface plugin: we now need the interface interface
		res = (*plugInInterface)->QueryInterface (plugInInterface, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (LPVOID *) &usb_interface);
		(*plugInInterface)->Release (plugInInterface);			// done with this
		if (res || usb_interface == NULL)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: couldn't create an IOUSBInterfaceInterface (%08x)\n", (int) res));
			continue;
		}

		// get the interface class and subclass
		kr = (*usb_interface)->GetInterfaceClass (usb_interface, &intfClass);
		kr = (*usb_interface)->GetInterfaceSubClass (usb_interface, &intfSubClass);
#ifdef DEBUG_USB
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: interface class %d, subclass %d\n", intfClass, intfSubClass));
#endif

		// Now open the interface. This will cause the pipes to be instantiated that are
		// associated with the endpoints defined in the interface descriptor.
		kr = (*usb_interface)->USBInterfaceOpen (usb_interface);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to open interface (%08x)\n", kr));
			(*usb_interface)->Release (usb_interface);
			usb_interface = NULL;
			continue;
		}

		kr = (*usb_interface)->GetNumEndpoints (usb_interface, &intfNumEndpoints);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to get number of endpoints (%08x)\n", kr));
			(*usb_interface)->USBInterfaceClose (usb_interface);
			(*usb_interface)->Release (usb_interface);
			usb_interface = NULL;
			continue;
		}
#ifdef DEBUG_USB
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: interface has %d endpoints\n", intfNumEndpoints));
#endif

		// locate the pipes we're going to use for reading and writing. If we got a hint
		// from the PALM_GET_EXT_CONNECTION_INFORMATION, we try this one first. In case
		// we don't find both pipes with the endpoint hint, we try again without the hint
		// and take the first ones that come. 
try_pipes:
		for (pipeRef = 1; pipeRef <= intfNumEndpoints; pipeRef++)
		{
			UInt8 direction, number, transferType, interval;
			UInt16 maxPacketSize;

			kr = (*usb_interface)->GetPipeProperties (usb_interface, pipeRef, &direction, &number, &transferType, &maxPacketSize, &interval);
#ifdef DEBUG_USB
			if (kr == kIOReturnSuccess)
				LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: pipe %d: direction=0x%02x, number=0x%02x, transferType=0x%02x, maxPacketSize=%d, interval=0x%02x\n",pipeRef,(int)direction,(int)number,(int)transferType,(int)maxPacketSize,(int)interval));
#endif
			if (kr != kIOReturnSuccess)
				LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to get properties of pipe %d (kr=0x%08x)\n", pipeRef, kr));
			else
			{
				if (direction == kUSBIn && transferType == kUSBBulk && maxPacketSize == 64 && (inputPipeNumber == 0xff || number == inputPipeNumber))
					usb_in_pipe_ref = pipeRef;
				else if (direction == kUSBOut && transferType == kUSBBulk && maxPacketSize == 64 && (outputPipeNumber == 0xff || number == outputPipeNumber))
					usb_out_pipe_ref = pipeRef;
			}
		}
		if ((usb_in_pipe_ref==0 || usb_out_pipe_ref==0) && (inputPipeNumber != 0xff || outputPipeNumber != 0xff))
		{
			// just in case we failed finding the pipes (ie the connection info struct has changed format),
			// make a second try with "wild guess" method.
			inputPipeNumber = 0xff;
			outputPipeNumber = 0xff;
			goto try_pipes;
		}

		if (usb_in_pipe_ref && usb_out_pipe_ref)
		{
			// Just like with service matching notifications, we need to create an event source and add it
			// to our run loop in order to receive async completion notifications.
			CFRunLoopSourceRef runLoopSource;
			kr = (*usb_interface)->CreateInterfaceAsyncEventSource (usb_interface, &runLoopSource);
			if (kr != kIOReturnSuccess)
			{
				LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: Unable to create async event source (%08x)\n", kr));
			}
			else
			{
#ifdef DEBUG_USB
				LOG((PI_DBG_DEV, PI_DBG_LVL_INFO, "darwinusb: USBConnection OPENED usb_in_pipe_ref=%d usb_out_pipe_ref=%d\n", usb_in_pipe_ref, usb_out_pipe_ref));
#endif
				usb_opened = 1;
				CFRunLoopAddSource (CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
				//usb_read_ahead_size = MAX_AUTO_READ_SIZE;
				prime_read();
				break;
			}
		}

		// if we didn't find two suitable pipes, close the interface
		(*usb_interface)->USBInterfaceClose (usb_interface);
		(*usb_interface)->Release (usb_interface);
		usb_interface = NULL;
		usb_in_pipe_ref = 0;
		usb_out_pipe_ref = 0;
	}

	return usb_interface==NULL ? -1 : kIOReturnSuccess;
}

static IOReturn
control_request(IOUSBDeviceInterface **dev, UInt8 requestType, UInt8 request, void *pData, UInt16 maxReplyLength)
{
	IOReturn kr;
	IOUSBDevRequest req;
	void *pReply = pData;

	if (!pReply && maxReplyLength)
		pReply = malloc(maxReplyLength);

	req.bmRequestType = requestType;			// 0xc2=kUSBIn, kUSBVendor, kUSBEndpoint
	req.bRequest = request;
	req.wValue = 0;
	req.wIndex = 0;
	req.wLength = maxReplyLength;
	req.pData = pReply;
	req.wLenDone = 0;

	kr = (*dev)->DeviceRequest (dev, &req);

#ifdef DEBUG_USB
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: control_request(0x%02x) wLenDone=%d kr=0x%08lx\n", (int)request, req.wLenDone, kr));
#endif

	if (pReply && !pData)
		free (pReply);

	return kr;
}

static IOReturn
read_visor_connection_information (IOUSBDeviceInterface **dev)
{
	int i;
	kern_return_t kr;
	visor_connection_info ci;

	kr = control_request (dev, 0xc2, VISOR_GET_CONNECTION_INFORMATION, &ci, sizeof(ci));
	if (kr != kIOReturnSuccess)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: VISOR_GET_CONNECTION_INFORMATION failed (err=%08x)\n", kr));
	}
	else
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: VISOR_GET_CONNECTION_INFORMATION, num_ports=%d\n", ci.num_ports));
		if (ci.num_ports > 2)
			ci.num_ports = 2;
		for (i=0; i < ci.num_ports; i++)
		{
			char *function_str;
			switch (ci.connections[i].port_function_id)
			{
				case VISOR_FUNCTION_GENERIC:
					function_str="GENERIC";
					break;
				case VISOR_FUNCTION_DEBUGGER:
					function_str="DEBUGGER";
					break;
				case VISOR_FUNCTION_HOTSYNC:
					function_str="HOTSYNC";
					break;
				case VISOR_FUNCTION_CONSOLE:
					function_str="CONSOLE";
					break;
				case VISOR_FUNCTION_REMOTE_FILE_SYS:
					function_str="REMOTE_FILE_SYSTEM";
					break;
				default:
					function_str="UNKNOWN";
					break;
			}
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] port_function_id=0x%02x (%s)\n", i, ci.connections[i].port_function_id, function_str));
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] port=%d\n", i, ci.connections[i].port));
		}
	}
	return kr;
}

static IOReturn
read_generic_connection_information (IOUSBDeviceInterface **dev, UInt8 *inputPipeNumber, UInt8 *outputPipeNumber)
{
	int i;
	kern_return_t  kr;
	palm_ext_connection_info ci;

	kr = control_request (dev, 0xc2, PALM_GET_EXT_CONNECTION_INFORMATION, &ci, sizeof(ci));
	if (kr != kIOReturnSuccess)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: PALM_GET_EXT_CONNECTION_INFORMATION failed (err=%08x)\n", kr));
	}
	else
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: PALM_GET_EXT_CONNECTION_INFORMATION, num_ports=%d, endpoint_numbers_different=%d\n", ci.num_ports, ci.endpoint_numbers_different));
		for (i=0; i < ci.num_ports; i++)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] port_function_id=0x%08lx\n", i, ci.connections[i].port_function_id));
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] port=%d\n", i, ci.connections[i].port));
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] endpoint_info=%d\n", i, ci.connections[i].endpoint_info));
			if (ci.connections[i].port_function_id == 'cnys')
			{
				// 'sync': we found the pipes to use for synchronization
				// force find_interfaces to select this one rather than another one
				if (inputPipeNumber)
					*inputPipeNumber = ci.connections[i].endpoint_info >> 4;
				if (outputPipeNumber)
					*outputPipeNumber = ci.connections[i].endpoint_info & 0x0f;
			}
		}
	}
	return kr;
}

static void
device_notification(void *refCon, io_service_t service, natural_t messageType, void *messageArgument)
{
	if (messageType == kIOMessageServiceIsTerminated)
	{
		usb_opened = 0;		// so that stop_listening() does'nt try to send the control_request
		stop_listening ();

		// In case the reading thread is waiting for data,
		// we need to raise the usb_data_available cond once.
		// since darwin_usb_read tests usb_opened, it will
		// gracefully exit during a data wait.
		pthread_mutex_lock (&read_queue_mutex);
		pthread_cond_broadcast (&read_queue_data_avail_cond);
		pthread_mutex_unlock (&read_queue_mutex);

#ifdef DEBUG_USB
		LOG((PI_DBG_DEV, PI_DBG_LVL_INFO, "darwinusb: device_notification(): USBConnection CLOSED\n"));
#endif
	}
}

static void
read_completion (void *refCon, IOReturn result, void *arg0)
{
	size_t bytes_read = (size_t) arg0;

#ifdef DEBUG_USB
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: read_completion callback with %d bytes result=0x%08lx\n", bytes_read,(long)result));
#endif

	pthread_mutex_lock(&read_queue_mutex);

	if (result != kIOReturnSuccess)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_WARN, "darwinusb: async read completion received error code 0x%08x\n", result));
	}
	else
	{
		if (read_queue == NULL)
		{
			read_queue_size = ((bytes_read + 0xfffe) & ~0xffff) - 1;		// 64k chunks
			read_queue = (char *) malloc (read_queue_size);
			read_queue_used = 0;
		}
		else if ((read_queue_used + bytes_read) > read_queue_size)
		{
			read_queue_size += ((bytes_read + 0xfffe) & ~0xffff) - 1;
			read_queue = (char *) realloc (read_queue, read_queue_size);
		}

		//CHECK(PI_DBG_DEV, PI_DBG_LVL_DEBUG, dumpdata(usb_read_buffer, bytes_read));

		memcpy(read_queue + read_queue_used, usb_read_buffer, bytes_read);
		read_queue_used += bytes_read;

		pthread_cond_broadcast(&read_queue_data_avail_cond);
	}

	if (result != kIOReturnAborted)
	{
		if (result != kIOReturnSuccess)
			(*usb_interface)->ClearPipeStall (usb_interface, usb_in_pipe_ref);
		prime_read();
	}

	pthread_mutex_unlock(&read_queue_mutex);
}

static int
prime_read(void)
{
	if (usb_opened)
	{
		// select a correct read size (always use a multiple of the USB packet size)
		usb_last_read_ahead_size = usb_read_ahead_size & ~63;
		if (usb_last_read_ahead_size <= 0)
			usb_last_read_ahead_size = usb_auto_read_size;
		if (usb_last_read_ahead_size > MAX_AUTO_READ_SIZE)
			usb_last_read_ahead_size = MAX_AUTO_READ_SIZE;
		else if (usb_last_read_ahead_size <= 0)
			usb_last_read_ahead_size = 64;				// USB packet size

		//usb_last_read_ahead_size = MAX_AUTO_READ_SIZE;	// testing

#ifdef DEBUG_USB
		LOG((PI_DBG_DEV, PI_DBG_LVL_INFO, "darwinusb: prime_read() for %d bytes\n", usb_last_read_ahead_size));
#endif

		IOReturn kr;
		kr = (*usb_interface)->ReadPipeAsync (usb_interface, usb_in_pipe_ref, usb_read_buffer,
				usb_last_read_ahead_size, &read_completion, NULL);

		if (kr == kIOUSBPipeStalled)
		{
#ifdef DEBUG_USB
			LOG((PI_DBG_DEV, PI_DBG_LVL_INFO, "darwinusb: stalled -- clearing stall and re-priming\n"));
#endif
			(*usb_interface)->ClearPipeStall (usb_interface, usb_in_pipe_ref);
			kr = (*usb_interface)->ReadPipeAsync (	usb_interface, usb_in_pipe_ref, usb_read_buffer,
					usb_last_read_ahead_size, &read_completion, NULL);
		}

		if (kr == kIOReturnSuccess)
			return 1;

		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb:  prime_read(): ReadPipeAsync returned error 0x%08x\n", kr));
	}
	return 0;
}

static int
accepts_device(unsigned short vendor, unsigned short product)
{
	int i;

	for (i=0; i < (int)(sizeof(acceptedDevices) / sizeof(acceptedDevices[0])); i++)
	{
		if (vendor == acceptedDevices[i].vendorID)
		{
			if (acceptedDevices[i].productID == 0xffff || product == acceptedDevices[i].productID)
				return 1;
		}
	}
	return 0;
}



/***************************************************************************/
/*                                                                         */
/*                      ENTRY POINTS CALLED BY LIBPISOCK                   */
/*                                                                         */
/***************************************************************************/


static int
u_open(struct pi_socket *ps, struct pi_sockaddr *addr, size_t addrlen)
{
	// thread doesn't exist yet: create it and wait for
	// the init phase to be either successful or failed
	if (usb_thread == 0)
	{
		pthread_mutex_lock(&usb_thread_ready_mutex);
		pthread_create(&usb_thread, NULL, usb_thread_run, NULL);
		pthread_cond_wait(&usb_thread_ready_cond, &usb_thread_ready_mutex);
		pthread_mutex_unlock(&usb_thread_ready_mutex);
		if (usb_thread != 0)
			return 1;

		errno = EINVAL;
		return pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);
	}
	
	// thread exists: empty the read queue
	pthread_mutex_lock(&read_queue_mutex);
	if (read_queue)
	{
		free(read_queue);
		read_queue = NULL;
	}
	read_queue_size = 0;
	read_queue_used = 0;
	pthread_mutex_unlock(&read_queue_mutex);
	return 1;
}

static int
u_close(struct pi_socket *ps)
{
	if (usb_thread)
	{
		pthread_mutex_lock(&usb_run_loop_mutex);
		if (usb_run_loop != NULL)
			CFRunLoopStop(usb_run_loop);
		pthread_mutex_unlock(&usb_run_loop_mutex);

		pthread_join(usb_thread, NULL);
		usb_thread = 0;
		
		if (read_queue)
		{
			free (read_queue);
			read_queue = NULL;
			read_queue_size = read_queue_used = 0;
		}
	}
	return 0;
}

static int
u_poll(struct pi_socket *ps, int timeout)
{
	pthread_mutex_lock(&read_queue_mutex);
	int available = read_queue_used;
	if (!available)
	{
		if (timeout)
		{
			struct timeval now;
			struct timespec when;

			gettimeofday(&now, NULL);
			when.tv_sec = now.tv_sec + timeout / 1000;
			when.tv_nsec = now.tv_usec + (timeout % 1000) * 1000 * 1000;
			if (when.tv_nsec >= 1000000000)
			{
				when.tv_nsec -= 1000000000;
				when.tv_sec++;
			}

			if (pthread_cond_timedwait(&read_queue_data_avail_cond, &read_queue_mutex, &when) == ETIMEDOUT)
			{
				pthread_mutex_unlock(&read_queue_mutex);
				return 0;
			}
			available = read_queue_used;
		}
		else
		{
			// wait forever for some data to arrive
			pthread_cond_wait(&read_queue_data_avail_cond, &read_queue_mutex);
			available = read_queue_used;
		}
	}
	pthread_mutex_unlock(&read_queue_mutex);
	return available;
}

static int
u_write(struct pi_socket *ps, unsigned char *buf, size_t len, int flags)
{
	if (!usb_opened)
	{
		// make sure we report broken connections
		if (ps->state == PI_SOCK_CONAC || ps->state == PI_SOCK_CONIN)
			ps->state = PI_SOCK_CONBK;
		return 0;
	}

	IOReturn kr = (*usb_interface)->WritePipe(usb_interface, usb_out_pipe_ref, buf, len);
	if (kr != kIOReturnSuccess) {
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: darwin_usb_write(): WritePipe returned kr=0x%08lx\n", kr));
	}

	return (kr != kIOReturnSuccess) ? 0 : len;
}


static int
u_read(struct pi_socket *ps, pi_buffer_t *buf, size_t len, int flags)
{
	int bytes_read;
	int timeout = ((struct pi_usb_data *)ps->device->data)->timeout;

	if (!usb_opened) {
		// make sure we report broken connections
		if (ps->state == PI_SOCK_CONAC || ps->state == PI_SOCK_CONIN)
			ps->state = PI_SOCK_CONBK;
		return 0;
	}

	if (pi_buffer_expect (buf, len) == NULL) {
		errno = ENOMEM;
		return pi_set_error(ps->sd, PI_ERR_GENERIC_MEMORY);
	}

#ifdef DEBUG_USB
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: darwin_usb_read(len=%d, timeout=%d, flags=%d)\n", len, timeout, flags));
#endif

	pthread_mutex_lock(&read_queue_mutex);

	if (flags == PI_MSG_PEEK && len > 256)
		len = 256;

	if (read_queue_used < len)
	{
		struct timeval now;
		struct timespec when, nownow;
		gettimeofday(&now, NULL);
		when.tv_sec = now.tv_sec + timeout / 1000;
		when.tv_nsec = now.tv_usec + (timeout % 1000) * 1000 * 1000;
		if (when.tv_nsec >= 1000000000) {
			when.tv_nsec -= 1000000000;
			when.tv_sec++;
		}

		do
		{
			usb_read_ahead_size = len - read_queue_used - usb_last_read_ahead_size;		// next prime_read() will use a bigger read request

			if (timeout)
			{
				gettimeofday(&now, NULL);
				TIMEVAL_TO_TIMESPEC(&now, &nownow);
				if ((nownow.tv_sec == when.tv_sec ? (nownow.tv_nsec > when.tv_nsec) : (nownow.tv_sec > when.tv_sec)))
					break;
				if (pthread_cond_timedwait(&read_queue_data_avail_cond, &read_queue_mutex, &when) == ETIMEDOUT)
					break;
			}
			else
			{
				pthread_cond_wait(&read_queue_data_avail_cond, &read_queue_mutex);
			}
		}
		while (usb_opened && (read_queue_used < len || (flags == PI_MSG_PEEK && read_queue_used >= 256)));

		usb_read_ahead_size = 0;
	}

	if (!usb_opened)
	{
		// make sure we report broken connections
		if (ps->state == PI_SOCK_CONAC || ps->state == PI_SOCK_CONIN)
			ps->state = PI_SOCK_CONBK;
		bytes_read = PI_ERR_SOCK_DISCONNECTED;
	}
	else
	{
		if (read_queue_used < len)
			len = read_queue_used;

		bytes_read = (int)len;

		if (len)
		{
			pi_buffer_append (buf, read_queue, len);

			if (flags != PI_MSG_PEEK)
			{
				read_queue_used -= len;
				if (read_queue_used)
					memmove(read_queue, read_queue + len, read_queue_used);
				if ((read_queue_size - read_queue_used) > (16L * 65535L))
				{
					// if we have more than 1M free in the read queue, we'd better
					// shrink the buffer
					read_queue_size = ((read_queue_used + 0xfffe) & ~0xffff) - 1;
					read_queue = (char *) realloc (read_queue, read_queue_size);
				}
			}
		}
	}

#ifdef DEBUG_USB
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: read done, len=%d, remaining bytes in queue=%d\n", len, read_queue_used));
#endif
	
	pthread_mutex_unlock(&read_queue_mutex);
	return len;
}

static int
u_flush(pi_socket_t *ps, int flags)
{
	if (flags & PI_FLUSH_INPUT) {
		pthread_mutex_lock(&read_queue_mutex);
		read_queue_used = 0;
		pthread_mutex_unlock(&read_queue_mutex);
	}
	return 0;
}

void
pi_usb_impl_init (struct pi_usb_impl *impl)
{
	impl->open = u_open;
	impl->close = u_close;
	impl->write = u_write;
	impl->read = u_read;
	impl->flush = u_flush;
	impl->poll = u_poll;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
