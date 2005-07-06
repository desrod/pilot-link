/*
 * darwinusb.c: I/O support for Darwin (Mac OS X) USB
 *
 * Copyright (c) 2004-2005, Florent Pillet.
 *
 * $Id$
 *
 * libpisock interface modeled after linuxusb.c by Jeff Dionne and 
 * Kenneth Albanowski
 * Some structures & defines extracted from Linux "visor.c",
 * which is Copyright (C) 1999 - 2003 Greg Kroah-Hartman (greg@kroah.com)
 * KLSI adapter (PalmConnect USB) support implemented thanks for the
 * Linux implementation made by Utz-Uwe Haus (haus@uuhaus.de)
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

/* Theory of operation
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
 *   use for input and output.
 * - We register for notifications coming from the device. When the device
 *   goes away, our notification callback is called and we can cleanly close
 *   things.
 * - Once everything is initialized, we fire a first "read" from the read
 *   pipe.  Subsequent reads are fired directly from the previous read's
 *   completion routine [1].
 * - In case the thread or application is aborted, the IOKit will clean things
 *   up for us.
 *
 * [1] Reading is done asynchronously and in a chained way: we fire a read
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
#include "pi-util.h"

/* Define this to make debug logs include USB debug info */
#ifdef PI_DEBUG
    #define DEBUG_USB 1
#endif
//#undef DEBUG_USB        /* comment out to leave debug enabled */

/* Macro to log more information when debugging USB. Note that this is for
 * my own use, mostly, as the info logged is primarily being used to
 * debug complex thread/usb issues
 */
#ifdef DEBUG_USB
	#define ULOG(a) LOG(a)
#else
	#define ULOG(a) do {} while(0)
#endif

/* These values are somewhat tricky.  Priming reads with a size of exactly one
 * USB packet works best (no timeouts).  Probably best to leave these as they are.
 */
#define MAX_AUTO_READ_SIZE	4096
#define AUTO_READ_SIZE		64

/* Options */
#if HAVE_PTHREAD
static int accept_multiple_simultaneous_connections = 1;	/* enabled by default when compiling with --enable-threads */
#else
static int accept_multiple_simultaneous_connections = 0;	/* disabled by default, set to 1 to enable */
#endif

/* IOKit interface */
static IONotificationPortRef usb_notify_port;
static io_iterator_t usb_device_added_iter;

/* RunLoop / threading management */
static CFRunLoopRef usb_run_loop = 0;
static pthread_mutex_t usb_run_loop_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t usb_thread_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t usb_thread_ready_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t usb_connections_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t usb_connection_added_cond = PTHREAD_COND_INITIALIZER;
static pthread_t usb_thread = 0;

/* Device interface linked list */
typedef struct usb_connection_t
{
	struct usb_connection_t *next;		/* linked list */
	struct pi_socket *ps;			/* the pilot-link socket we're associated with (if already paired) */

	/* refcount management */
	pthread_mutex_t ref_count_mutex;
	int ref_count;

	IOUSBInterfaceInterface190 **interface;	/* IOUSBInterface190 is 1.9.0 available on OS X 10.2 and later */
	IOUSBDeviceInterface **device;
	io_object_t device_notification;	/* for device removal */

	unsigned short vendorID;		/* connected USB device vendor ID */
	unsigned short productID;		/* connected USB device product ID */
	unsigned short dev_flags;		/* copy of the flags from the acceptedDevices structure */

	int opened;				/* set to != 0 if the connection is opened */
	int device_present;
	int read_pending;			/* set to 1 when a prime_read() has been issued and the read_completion() has not been called yet */
	int in_pipe_ref;			/* pipe for reads */
	int out_pipe_ref;			/* pipe for writes */
	int out_pipe_bulk_size;			/* size of bulk packets on the out pipe (used when talking with a PalmConnect USB serial adapter) */

	/* these provide hints about the size of the next read */
	int auto_read_size;			/* if != 0, prime reads to the input pipe to get data permanently */
	int read_ahead_size;			/* when waiting for big chunks of data, used as a hint to make bigger read requests */
	int last_read_ahead_size;		/* also need this to properly compute the size of the next read */

	pthread_mutex_t read_queue_mutex;
	pthread_cond_t read_queue_data_avail_cond;
	char *read_queue;			/* stores completed reads, grows by 64k chunks */
	size_t read_queue_size;
	size_t read_queue_used;

	char read_buffer[MAX_AUTO_READ_SIZE];
} usb_connection_t;

static usb_connection_t *usb_connections = NULL;	/* linked list of active connections */

/* USB control requests we send to the devices
 * Got them from linux/drivers/usb/serial/visor.h
 */
#define	GENERIC_REQUEST_BYTES_AVAILABLE		0x01
#define	GENERIC_CLOSE_NOTIFICATION		0x02
#define VISOR_GET_CONNECTION_INFORMATION	0x03
#define PALM_GET_EXT_CONNECTION_INFORMATION	0x04

/* Structures defining the info a device returns
 * Got them from linux/drivers/usb/serial/visor.h
 */
typedef struct
{
	UInt16 num_ports;
	struct
	{
		UInt8 port_function_id;
		UInt8 port;
	} connections[8];
} visor_connection_info;

/* struct visor_connection_info.connection[x].port defines: */
#define VISOR_ENDPOINT_1			0x01
#define VISOR_ENDPOINT_2			0x02

/* struct visor_connection_info.connection[x].port_function_id defines: */
#define VISOR_FUNCTION_GENERIC			0x00
#define VISOR_FUNCTION_DEBUGGER			0x01
#define VISOR_FUNCTION_HOTSYNC			0x02
#define VISOR_FUNCTION_CONSOLE			0x03
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
	} connections[8];
} palm_ext_connection_info;

/* PalmConnect USB specific information
 * Extracted from Linux kl5kusb105 Copyright (C) 2001 Utz-Uwe Haus <haus@uuhaus.de>
 * Using documentation Utz-Uwe provided
 */
enum {
	/* Values for KLSI_GET/SET_COMM_DESCRIPTOR */
	KLSI_BAUD_115200 = 0,
	KLSI_BAUD_57600  = 1,
 	KLSI_BAUD_38400  = 2,
	KLSI_BAUD_28800  = 3,
	KLSI_BAUD_19200  = 4,
	KLSI_BAUD_14400  = 5,
	KLSI_BAUD_9600   = 6,
	KLSI_BAUD_7200   = 7,
	KLSI_BAUD_4800   = 8,
	KLSI_BAUD_2400   = 9,
	KLSI_BAUD_1200   = 10,
	KLSI_BAUD_600    = 11,
	KLSI_BAUD_230400 = 12,
	
	KLSI_PARITY_NONE = 0,
	KLSI_PARITY_ODD  = 1,
	KLSI_PARITY_EVEN = 2,
	KLSI_PARITY_MARK = 3,
	
	KLSI_STOPBITS_0 = 0,
	KLSI_STOPBITS_2 = 2,
	
	/* Handshake values for KLSI_GET_HANDSHAKE_LINES */
	KLSI_GETHS_DCD = 0x80,		/* Data Carrier Detect */
	KLSI_GETHS_RI  = 0x40,		/* Ring Indicator */
	KLSI_GETHS_DSR = 0x20,		/* Data Set Ready */
	KLSI_GETHS_CTS = 0x10,		/* Clear To Send */
	
	/* Handshake values for KLST_SET_HANDSHAKE_LINES */
	KLSI_SETHS_RTS = 0x02,		/* Ready To Send */
	KLSI_SETHS_DTR = 0x01,		/* Data Terminal Ready */
	
	/* Flow control values */
	KLSI_FLOW_USE_RTS = 0x01,	/* use RTS/CTS */
	KLSI_FLOW_USE_DSR = 0x02,	/* use DSR/CD */
	KLSI_FLOW_USE_XON = 0x04	/* use XON/XOFF */
};

#define KLSI_GET_COMM_DESCRIPTOR		0
#define KLSI_SET_COMM_DESCRIPTOR		1
#define KLSI_GET_HANDSHAKE_LINES		2
#define KLSI_SET_HANDSHAKE_LINES		3
#define KLSI_GET_FLOWCONTROL			4
#define	KLSI_SET_FLOWCONTROL			5

typedef struct
{
	unsigned char	DCBLength	__attribute__((packed));
	unsigned char	BaudRateIndex	__attribute__((packed));
	unsigned char	DataBits	__attribute__((packed));
	unsigned char	Parity		__attribute__((packed));
	unsigned char	StopBits	__attribute__((packed));
} klsi_port_settings;

/* Some vendor and product codes we use */
#define	VENDOR_SONY			0x054c
#define	VENDOR_KEYSPAN			0x06cd
#define	VENDOR_HANDSPRING		0x082d
#define	VENDOR_PALMONE			0x0830
#define	VENDOR_TAPWAVE			0x12ef
#define PRODUCT_PALMCONNECT_USB		0x0080
#define	PRODUCT_HANDSPRING_VISOR	0x0100
#define PRODUCT_SONY_CLIE_3_5		0x0038

/* This table helps us determine whether a connecting USB device is
 * one we'd like to talk to. Don't forget to update it as new
 * devices come out. To accept ALL the devices from a vendor, add
 * an entry with the vendorID and 0xFFFF as productID.
 */

#define FLAG_ANSWERS_CONN_INFO		0x0001		/* device is known to answer connection information requests */
#define FLAG_REJECT			0x8000		/* device is known but not supported yet */

static struct {
	unsigned short vendorID;
	unsigned short productID;
	unsigned short flags;
}
acceptedDevices[] = {
	/* Sony */
	{0x054c, 0x0038},	/* Sony Palm OS 3.5 devices, S300 */
	{0x054c, 0x0066},	/* Sony T, S320, SJ series, and other Palm OS 4.0 devices */
	{0x054c, 0x0095},	/* Sony S360 */
	{0x054c, 0x000a},	/* Sony NR and other Palm OS 4.1 devices */
	{0x054c, 0x009a},	/* Sony NR70V/U */
	{0x054c, 0x00da},	/* Sony NX */
	{0x054c, 0x00e9},	/* Sony NZ */
	{0x054c, 0x0144},	/* Sony UX */
	{0x054c, 0x0169},	/* Sony TJ */

	/* Keyspan serial-to-USB PDA adapter */
	{0x06cd, 0x0103, FLAG_REJECT},	/* ID sent by an adapter which firmware has not been uploaded yet */
	{0x06cd, 0x0104, FLAG_REJECT},	/* ID sent by an adapter with proper firmware uploaded */

	/* AlphaSmart */
	{0x081e, 0xdf00},	/* Dana */

	/* HANDSPRING (vendor 0x082d) */
	{0x082d, 0x0100},	/* Visor */
	{0x082d, 0x0200},	/* Treo */
	{0x082d, 0x0300, FLAG_ANSWERS_CONN_INFO},	/* Treo 600 */

	/* PalmOne, Palm Inc */
	{0x0830, 0x0001, FLAG_ANSWERS_CONN_INFO},	/* m500 */
	{0x0830, 0x0002, FLAG_ANSWERS_CONN_INFO},	/* m505 */
	{0x0830, 0x0003, FLAG_ANSWERS_CONN_INFO},	/* m515 */
	{0x0830, 0x0010},
	{0x0830, 0x0011},
	{0x0830, 0x0020},	/* i705 */
	{0x0830, 0x0030},
	{0x0830, 0x0031},	/* Tungsten|W */
	{0x0830, 0x0040},	/* m125 */
	{0x0830, 0x0050},	/* m130 */
	{0x0830, 0x0051},
	{0x0830, 0x0052},
	{0x0830, 0x0053},
	{0x0830, 0x0060, FLAG_ANSWERS_CONN_INFO},	/* Tungsten series, Zire 71 */
	{0x0830, 0x0061, FLAG_ANSWERS_CONN_INFO},	/* Zire 31, 72, T5, LifeDrive */
	{0x0830, 0x0062},
	{0x0830, 0x0063},
	{0x0830, 0x0070, FLAG_ANSWERS_CONN_INFO},	/* Zire */
	{0x0830, 0x0071},
	{0x0830, 0x0080},	/* palmOne serial adapter (actually, a Keyspan adapter) */
	{0x0830, 0x0099},
	{0x0830, 0x0100},

	/* GARMIN */
	{0x091e, 0x0004},	/* IQUE 3600 */

	/* Kyocera */
	{0x0c88, 0x0021},	/* 7135 Smartphone */
	{0x0c88, 0xa226},	/* 6035 Smartphone */

	/* Tapwave */
	{0x12ef, 0x0100, FLAG_ANSWERS_CONN_INFO},	/* Zodiac, Zodiac2 */

	/* ACEECA */
	{0x4766, 0x0001},	/* MEZ1000 */

	/* Samsung */
	{0x04e8, 0x8001}	/* I330 */
};


/* local prototypes */
static int change_refcount(usb_connection_t *c, int increment);
static void stop_listening(usb_connection_t *c);
static IOReturn control_request (IOUSBDeviceInterface **dev, UInt8 requestType, UInt8 request, UInt16 value, UInt16 index, void *pData, UInt16 maxReplyLength);
static void device_added (void *refCon, io_iterator_t iterator);
static void device_notification (usb_connection_t *connexion, io_service_t service, natural_t messageType, void *messageArgument);
static void read_completion (usb_connection_t *connexion, IOReturn result, void *arg0);
static int accepts_device (unsigned short vendor, unsigned short product, unsigned short *flags);
static IOReturn	configure_device (IOUSBDeviceInterface **dev, unsigned short vendor, unsigned short product, unsigned short flags, int *port_number, int *input_pipe_number, int *output_pipe_number, int *pipe_info_retrieved);
static IOReturn	find_interfaces (usb_connection_t *usb, IOUSBDeviceInterface **dev, unsigned short vendor, unsigned short product, int port_number, int input_pipe_number, int output_pipe_number, int pipe_info_retrieved);
static int prime_read (usb_connection_t *connexion);
static IOReturn	read_visor_connection_information (IOUSBDeviceInterface **dev, int *port_number, int *input_pipe, int *output_pipe);
static IOReturn	decode_generic_connection_information(palm_ext_connection_info *ci, int *port_number, int *input_pipe, int *output_pipe);
static IOReturn	read_generic_connection_information (IOUSBDeviceInterface **dev, int *port_number, int *input_pipe_number, int *output_pipe_number);
static IOReturn klsi_set_portspeed(IOUSBDeviceInterface **dev, int speed);


/***************************************************************************/
/*                                                                         */
/*                GLOBAL DARWINUSB OPTIONS FOR CLIENT CODE                 */
/*                                                                         */
/***************************************************************************/
void
darwinusb_setoptions(int multiple_connections_support)
{
	accept_multiple_simultaneous_connections = multiple_connections_support;
}


/***************************************************************************/
/*                                                                         */
/*                   CONNECTIONS LINKED LIST MANAGEMENT                    */
/*                                                                         */
/***************************************************************************/
static void
add_connection(usb_connection_t *c)
{
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: adding connection %p to linked list\n", c));
	pthread_mutex_lock(&usb_connections_mutex);
	c->next = usb_connections;
	usb_connections = c;
	pthread_cond_signal(&usb_connection_added_cond);
	pthread_mutex_unlock(&usb_connections_mutex);
}

static int
remove_connection(usb_connection_t *c)
{
	/* remove the connection from the linked list if it exists
	 * and return != 0. Otherwise return 0. Don't free the
	 * connection structure or variables.
	 */
	usb_connection_t *previous = NULL, *elem;
	pthread_mutex_lock(&usb_connections_mutex);
	elem = usb_connections;
	while (elem && elem != c)
	{
		previous = elem;
		elem = elem->next;
	}
	if (elem)
	{
		if (previous)
			previous->next = elem->next;
		else
			usb_connections = elem->next;
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: removed connection %p from linked list\n", c));
	}
	pthread_mutex_unlock(&usb_connections_mutex);
	return elem != NULL;
}

static int
device_already_in_use(IOUSBDeviceInterface **device)
{
	usb_connection_t *elem;
	pthread_mutex_lock(&usb_connections_mutex);
	elem = usb_connections;
	while (elem != NULL && elem->device != device)
		elem = elem->next;
	pthread_mutex_unlock(&usb_connections_mutex);
	return elem != NULL;
}

static usb_connection_t*
connection_for_socket(pi_socket_t *ps)
{
	/* if there is no active connection associated with this socket,
	 * try to associate the first unassociated connection
	 */
	usb_connection_t *c = ((pi_usb_data_t *)ps->device->data)->ref;
	if (change_refcount(c, +1) > 0)
		return c;

	pthread_mutex_lock(&usb_connections_mutex);
	c = usb_connections;
	while (c && (c->ps != NULL || c->opened == 0))		/* skip connections which are being disposed of (opened=0) */
		c = c->next;
	if (change_refcount(c, +1) <= 0)
		c = NULL;
	else
	{
		c->ps = ps;
		((pi_usb_data_t *)ps->device->data)->ref = c;
	}
	pthread_mutex_unlock(&usb_connections_mutex);
	return c;
}

static int
change_refcount(usb_connection_t *c, int increment)
{
	/* update the refcount on the connection structure. If the refcount becomes
	 * zero, call the stop_listening() function
	 */
	int rc;
	if (c == NULL)
		return 0;
	if (pthread_mutex_lock(&c->ref_count_mutex) != 0)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: connection %p, can't lock ref_count_mutex (ref_count=%d)\n",c,c->ref_count_mutex));
		return 0;
	}
	rc = c->ref_count = c->ref_count + increment;
	if (rc < 0)
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: connection %p's refcount became < 0 (%d)\n", c, c->ref_count));
	if (rc == 0)
	{
		if (remove_connection(c))
			stop_listening(c);
	}
	else
		pthread_mutex_unlock(&c->ref_count_mutex);
	return rc;
}

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

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: start_listening for connections\n"));

	/* first create a master_port for my task */
	kr = IOMasterPort (MACH_PORT_NULL, &masterPort);
	if (kr || !masterPort)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: couldn't create a master IOKit Port(%08x)\n", kr));
		return PI_ERR_GENERIC_SYSTEM;
	}

	/* Set up the matching criteria for the devices we're interested in
	 * Interested in instances of class IOUSBDevice and its subclasses
	 * Since we are supporting many USB devices, we just get notifications
	 * for all USB devices and sort out the ones that we want later.
	 */
	matchingDict = IOServiceMatching (kIOUSBDeviceClassName);
	if (!matchingDict)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: can't create a USB matching dictionary\n"));
		mach_port_deallocate (mach_task_self(), masterPort);
		return PI_ERR_GENERIC_SYSTEM;
	}

	/* Create a notification port and add its run loop event source to our run loop
	 * This is how async notifications get set up.
	 */
	usb_notify_port = IONotificationPortCreate (masterPort);
	runLoopSource = IONotificationPortGetRunLoopSource (usb_notify_port);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);

	/* Set up a notifications to be called when a raw device is first matched by I/O Kit */
	kr = IOServiceAddMatchingNotification (
			usb_notify_port,
			kIOFirstMatchNotification,
			matchingDict,
			device_added,
			NULL,
			&usb_device_added_iter);

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: added service matching notification (kr=0x%08lx)\n", kr));

	/* Iterate once to get already-present devices and arm the notification */
	device_added (NULL, usb_device_added_iter);

	/* Now done with the master_port */
	mach_port_deallocate (mach_task_self(), masterPort);

	return 0;
}

static void
stop_listening(usb_connection_t *c)
{
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: stop_listening for connection %p\n",c));

	c->opened = 0;
	c->in_pipe_ref = 0;
	c->out_pipe_ref = 0;

	if (c->ps)
	{
		/* we do this because if the connection abruptly ends before pilot-link notices,
		 * we want to avoid pi_close() trying to do a dlp_EndOfSync()
		 */
		c->ps->state = PI_SOCK_CLOSE;
		if (c->ps->device != NULL && c->ps->device->data != NULL)
			((pi_usb_data_t *)c->ps->device->data)->ref = NULL;
	}
	c->ps = NULL;

	if (c->device_notification)
	{
		IOObjectRelease (c->device_notification);
		c->device_notification = 0;
	}

	if (c->interface)
	{
		(*c->interface)->USBInterfaceClose (c->interface);
		(*c->interface)->Release(c->interface);
		c->interface = NULL;
	}

	if (c->device)
	{
		(*c->device)->USBDeviceClose (c->device);
		(*c->device)->Release(c->device);
		c->device = NULL;
	}

	pthread_mutex_destroy(&c->read_queue_mutex);
	pthread_mutex_destroy(&c->ref_count_mutex);
	free(c);
}

static void *
usb_thread_run(void *foo)
{
	if (start_listening() == 0)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: usb_thread_run, starting...\n"));

		/* obtain the CFRunLoop for this thread */
		pthread_mutex_lock(&usb_run_loop_mutex);
		usb_run_loop = CFRunLoopGetCurrent();
		pthread_mutex_unlock(&usb_run_loop_mutex);

		/* signal main thread that init was successful */
		pthread_mutex_lock(&usb_thread_ready_mutex);
		pthread_cond_broadcast(&usb_thread_ready_cond);
		pthread_mutex_unlock(&usb_thread_ready_mutex);

		CFRunLoopRun();

		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: usb_thread_run, done with runloop\n"));

		pthread_mutex_lock(&usb_run_loop_mutex);
		usb_run_loop = 0;
		pthread_mutex_unlock(&usb_run_loop_mutex);

		if (usb_device_added_iter)
		{
			IOObjectRelease (usb_device_added_iter);
			usb_device_added_iter = 0;
		}

		if (usb_notify_port)
		{
			IONotificationPortDestroy(usb_notify_port);
			usb_notify_port = NULL;
		}

		/* decrement the refcount of each structure. If there
		 * was a pending read, decrement once more since
		 * prime_read() increments the refcount
		 */
		usb_connection_t *elem, *next, *prev = NULL;
		pthread_mutex_lock(&usb_connections_mutex);
		elem = usb_connections;
		while (elem != NULL)
		{
			next = elem->next;
			pthread_mutex_lock(&elem->ref_count_mutex);
			elem->ref_count--;
			if (elem->read_pending)
				elem->ref_count--;
			if (elem < 0)
				LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: while stopping usb_thread, connection %p's refcount became < 0 (%d)", elem, elem->ref_count));
			if (elem == 0)
			{
				if (prev != NULL)
					prev->next = next;
				else
					usb_connections = next;
				stop_listening(elem);
				elem = NULL;
			}
			else
			{
				pthread_mutex_unlock(&elem->ref_count_mutex);
				prev = elem;
			}
		}
		pthread_mutex_unlock(&usb_connections_mutex);

		usb_thread = 0;
		usb_run_loop = NULL;

		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: usb_thread_run, exited.\n"));
	}
	else
	{
		/* signal main thread that init failed */
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
	unsigned short accept_flags;
	int port_number = 0xff,
		input_pipe_number = 0xff,
		output_pipe_number = 0xff,
		pipe_info_retrieved = 0;

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: device_added\n"));

	while ((ioDevice = IOIteratorNext (iterator)))
	{
		if (usb_connections != NULL && !accept_multiple_simultaneous_connections)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: new device plugged but we already have a running connection\n"));
			IOObjectRelease (ioDevice);
			continue;
		}

		kr = IOCreatePlugInInterfaceForService (ioDevice, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
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

		/* make sure this device is not already being handled (this may happen
		 * with some handhelds that reconnect immediately after disconnecting, like the T5)
		 */
		if (device_already_in_use(dev))
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: device %p already in use, skipping it\n"));
			IOObjectRelease (ioDevice);
			continue;
		}

		kr = (*dev)->GetDeviceVendor (dev, &vendor);
		kr = (*dev)->GetDeviceProduct (dev, &product);
		if (accepts_device(vendor, product, &accept_flags) == 0)
		{
			ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: not accepting device (vendor=0x%04x product=0x%04x)\n", vendor, product));
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

		/* configure the device and query for its preferred I/O pipes */
		kr = configure_device (dev, vendor, product, accept_flags, &port_number, &input_pipe_number, &output_pipe_number, &pipe_info_retrieved);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, (kr == kIOReturnNotReady)
				?	"darwinusb: device not ready to synchonize\n"
				:	"darwinusb: unable to configure device (kr=0x%08x)\n", kr));
			(*dev)->USBDeviceClose (dev);
			(*dev)->Release (dev);
			IOObjectRelease (ioDevice);
			continue;
		}

		/* allocate and initialize the USB connection structure */
		usb_connection_t *c = malloc(sizeof(usb_connection_t));
		if (c == NULL)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "out of memory"));
			(*dev)->USBDeviceClose (dev);
			(*dev)->Release (dev);
			IOObjectRelease (ioDevice);
			break;
		}

		memset(c, 0, sizeof(usb_connection_t));
		c->auto_read_size = AUTO_READ_SIZE;
		c->device = dev;
		c->ref_count = 1;
		c->opened = 1;
		c->device_present = 1;
		c->vendorID = vendor;
		c->productID = product;
		c->dev_flags = accept_flags;

		/* try to locate the pipes we need to talk to the device */
		kr = find_interfaces(c, dev, vendor, product, port_number, input_pipe_number, output_pipe_number, pipe_info_retrieved);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to find interfaces (kr=0x%08x)\n", kr));
			free(c);
			(*dev)->USBDeviceClose (dev);
			(*dev)->Release (dev);
			IOObjectRelease (ioDevice);
			continue;
		}

		/* Just like with service matching notifications, we need to create an event source and add it
		 * to our run loop in order to receive async completion notifications.
		 */
		CFRunLoopSourceRef runLoopSource;
		kr = (*c->interface)->CreateInterfaceAsyncEventSource (c->interface, &runLoopSource);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: Unable to create async event source (%08x)\n", kr));
			free(c);
			(*dev)->USBDeviceClose (dev);
			(*dev)->Release (dev);
			IOObjectRelease (ioDevice);
			continue;
		}
		CFRunLoopAddSource (CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);

		LOG((PI_DBG_DEV, PI_DBG_LVL_INFO, "darwinusb: USBConnection %p OPENED c->in_pipe_ref=%d c->out_pipe_ref=%d\n",c,c->in_pipe_ref,c->out_pipe_ref));

		/* Register for an interest notification for this device,
		 * so we get notified when it goes away
		 */
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: registering for disconnect notification\n"));
		kr = IOServiceAddInterestNotification(
				usb_notify_port,
				ioDevice,
				kIOGeneralInterest,
				(IOServiceInterestCallback)device_notification,
				(void *)c,
				&c->device_notification);
		IOObjectRelease(ioDevice);

		/* add the device to our linked list, then only start reading data
		 * (order of operations matters here to avoid race conditions)
		 */
		pthread_mutex_init(&c->read_queue_mutex, NULL);
		pthread_mutex_init(&c->ref_count_mutex, NULL);
		pthread_cond_init(&c->read_queue_data_avail_cond, NULL);

		add_connection(c);
		prime_read(c);
	}
}

static IOReturn
configure_device(IOUSBDeviceInterface **di,
				 unsigned short vendor,
				 unsigned short product,
				 unsigned short flags,
				 int *port_number,
				 int *input_pipe_number,
				 int *output_pipe_number,
				 int *pipe_info_retrieved)
{
	UInt8 numConf, conf, deviceClass;
	IOReturn kr;
	IOUSBConfigurationDescriptorPtr confDesc;

	/* Get the device class. Most handhelds are registered as composite devices
	 * and therefore already opened & configured by OS X drivers! It seems that
	 * reconfiguring them as we did before is what caused some Sony devices to
	 * refuse talking to us.
	 */
	kr = (*di)->GetDeviceClass (di, &deviceClass);
	if (kr != kIOReturnSuccess)
		return kr;

	if (deviceClass != kUSBCompositeClass)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: Not a composite device: performing confiuration\n"));

		kr = (*di)->GetNumberOfConfigurations (di, &numConf);
		if (!numConf)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: device has zero configurations!\n"));
			return -1;
		}

		/* try all possible configurations if the first one fails
		 * (shouldn't happen in most cases though)
		 */
		for (conf=0; conf < numConf; conf++)
		{
			kr = (*di)->GetConfigurationDescriptorPtr(di, 0, &confDesc);
			if (kr)
			{
				LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to get config descriptor for index %d (err=%08x numConf=%d)\n",
					0, kr, (int)numConf));
				continue;
			}

			kr = (*di)->SetConfiguration(di, confDesc->bConfigurationValue);
			if (kr == kIOReturnSuccess)
			{
				LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: successfully set configuration %d\n",(int)confDesc->bConfigurationValue));
				break;
			}

			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to set configuration to value %d (err=%08x numConf=%d)\n",
				(int)confDesc->bConfigurationValue, kr, (int)numConf));
		}
		if (conf == numConf)
			return kr;
	}

	if (vendor == VENDOR_PALMONE && product == PRODUCT_PALMCONNECT_USB)
	{
		kr = klsi_set_portspeed(di, 9600);
		*input_pipe_number = 0x01;
		*output_pipe_number = 0x02;
		*pipe_info_retrieved = 1;
		return kr;
	}

	/* Try reading pipe information. Most handhelds support a control request that returns info about the ports and
	 * pipes. We first try the generic control code, and if it doesn't work we try the Visor one which seems to be
	 * supported by some devices Also, we can detect that a T5 is in "wait" mode (device seen on USB but not synchronizing)
	 * and in this case we return a kIOReturnNotReady code.
	 */
	kr = read_generic_connection_information (di, port_number, input_pipe_number, output_pipe_number);
	if (kr != kIOReturnSuccess && kr != kIOReturnNotReady)
		kr = read_visor_connection_information (di, port_number, input_pipe_number, output_pipe_number);
	if (kr == kIOReturnNotReady)
		return kr;
	if (kr != kIOReturnSuccess && (flags & FLAG_ANSWERS_CONN_INFO))
		return kIOReturnNotReady;

	*pipe_info_retrieved = (kr == kIOReturnSuccess);

	/* query bytes available. Not that we really care, but most devices expect to receive this
	 * before they agree on talking to us.
	 */
	if (vendor != VENDOR_TAPWAVE)
	{
		unsigned char ba[2];
		kr = control_request (di, 0xc2, GENERIC_REQUEST_BYTES_AVAILABLE, 0, 0, &ba[0] , 2);
		if (kr != kIOReturnSuccess)
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: GENERIC_REQUEST_BYTES_AVAILABLE failed (err=%08x)\n", kr));
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "GENERIC_REQUEST_BYTES_AVAILABLE returns 0x%02x%02x\n", ba[0], ba[1]));
	}

	return kIOReturnSuccess;
}

static IOReturn
find_interfaces(usb_connection_t *c,
				IOUSBDeviceInterface **di,
				unsigned short vendor,
				unsigned short product,
				int port_number,
				int input_pipe_number,
				int output_pipe_number,
				int pipe_info_retrieved)
{
	IOReturn kr;
	io_iterator_t iterator;
	io_service_t usbInterface;
	HRESULT res;
	SInt32 score;
	UInt8 intfClass, intfSubClass, intfNumEndpoints;
	int pipeRef, pass;
	IOUSBFindInterfaceRequest request;
	IOCFPlugInInterface **plugInInterface = NULL;
	UInt8 direction, number, transferType, interval;
	UInt16 maxPacketSize;
	int pair_index;

	request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
	request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
	request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
	request.bAlternateSetting = kIOUSBFindInterfaceDontCare;

	kr = (*di)->CreateInterfaceIterator (di, &request, &iterator);

	while ((usbInterface = IOIteratorNext (iterator)))
	{
		kr = IOCreatePlugInInterfaceForService (usbInterface, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
		kr = IOObjectRelease (usbInterface);				// done with the usbInterface object now that I have the plugin
		if (kr != kIOReturnSuccess || !plugInInterface)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to create a plugin (%08x)\n", kr));
			continue;
		}

		/* we have the interface plugin: we now need the interface interface */
		res = (*plugInInterface)->QueryInterface (plugInInterface, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (LPVOID *) &c->interface);
		(*plugInInterface)->Release (plugInInterface);			/* done with this */
		if (res || c->interface == NULL)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: couldn't create an IOUSBInterfaceInterface (%08x)\n", (int) res));
			continue;
		}

		/* get the interface class and subclass */
		kr = (*c->interface)->GetInterfaceClass (c->interface, &intfClass);
		kr = (*c->interface)->GetInterfaceSubClass (c->interface, &intfSubClass);
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: interface class %d, subclass %d\n", intfClass, intfSubClass));

		/* Now open the interface. This will cause the pipes to be instantiated that are
		 * associated with the endpoints defined in the interface descriptor.
		 */
		kr = (*c->interface)->USBInterfaceOpen (c->interface);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to open interface (%08x)\n", kr));
			(*c->interface)->Release (c->interface);
			c->interface = NULL;
			continue;
		}

		kr = (*c->interface)->GetNumEndpoints (c->interface, &intfNumEndpoints);
		if (kr != kIOReturnSuccess)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to get number of endpoints (%08x)\n", kr));
			(*c->interface)->USBInterfaceClose (c->interface);
			(*c->interface)->Release (c->interface);
			c->interface = NULL;
			continue;
		}
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: interface has %d endpoints\n", intfNumEndpoints));

		/* If device didn't answer to the connection_information request,
		 * try to read a preamble sent by the device. This is sent by devices which chipsets
		 * don't support the vendor control endpoint requests. Look for the first pipe on
		 * which a preamble is available.
		 */
		if (!pipe_info_retrieved)
		{
			int reqTimeout;
			int preambleFound = 0;
			for (reqTimeout = 100; reqTimeout <= 300 && !preambleFound; reqTimeout += 100)
			{
				LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: checking for pipe_info sent by device with timeout %dms\n",reqTimeout));
				for (pipeRef = 1; pipeRef <= intfNumEndpoints; pipeRef++)
				{
					kr = (*c->interface)->GetPipeProperties (c->interface, pipeRef, &direction, &number,
										  &transferType, &maxPacketSize, &interval);
					if (kr != kIOReturnSuccess)
						continue;
					if (direction == kUSBIn)
					{
						UInt32 size = sizeof(c->read_buffer)-1;
						LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: trying pipe %d type=%d\n", pipeRef, (int)transferType));
						if (transferType == kUSBBulk)
							kr = (*c->interface)->ReadPipeTO (c->interface, pipeRef, &c->read_buffer, &size, reqTimeout, 250);
						else
							kr = (*c->interface)->ReadPipe (c->interface, pipeRef, &c->read_buffer, &size);

						if (kr == kIOReturnSuccess && size >= 8)
						{
							/* got something! */
							LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: got %d bytes there!\n", (int)size));
							CHECK(PI_DBG_DEV, PI_DBG_LVL_DEBUG, pi_dumpdata(c->read_buffer, size));
							if (!memcmp(c->read_buffer, "VNDR10", 6))
							{
								/* VNDR version 1.0 */
								palm_ext_connection_info ci;
								memcpy(&ci, &c->read_buffer[6], sizeof(ci));
								decode_generic_connection_information(&ci, &port_number, &input_pipe_number, &output_pipe_number);
								preambleFound = 1;
								break;
							}
						}
					}
				}
			}
		}

		/* Locate the pipes we're going to use for reading and writing.
		 * We have four chances to find the right pipes:
		 * 1. If we got a hint from the device with input/output pipes, we try this one first.
		 * 2. If we didn't get both pipes, try using the port number hint. There is at least one recent
		 *    device (LifeDrive) on which the port_number hint is actually an endpoint pair index.
		 *    The code below tries to cope with that.
		 * 3. If we're still missing one or two pipes, give a second try looking for pipes with a
		 *    64 bytes transfer size
		 * 4. Finally of this failed, forget about the transfer size and take the first ones that
		 *    come (i.e. Tungsten W has a 64 bytes IN pipe and a 32 bytes OUT pipe).
		 */
		for (pass=1; pass <= 4 && (c->in_pipe_ref==0 || c->out_pipe_ref==0); pass++)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: pass %d looking for pipes, port_number=0x%02x, input_pipe_number=0x%02x, output_pipe_number=0x%02x\n",
				pass, port_number, input_pipe_number, output_pipe_number));
			c->in_pipe_ref = 0;
			c->out_pipe_ref = 0;

			for (pipeRef = 1; pipeRef <= intfNumEndpoints; pipeRef++)
			{
				UInt8 prev_dir = direction;
				kr = (*c->interface)->GetPipeProperties (c->interface, pipeRef, &direction, &number,
									  &transferType, &maxPacketSize, &interval);
				if (kr != kIOReturnSuccess)
				{
					LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: unable to get properties of pipe %d (kr=0x%08x)\n", pipeRef, kr));
				}
				else
				{
					pair_index = (pipeRef > 1 && direction == prev_dir) ? (pair_index + 1) : 1;
					LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: pipe %d: direction=0x%02x, number=0x%02x, transferType=0x%02x, maxPacketSize=%d, interval=0x%02x, pair_index=%d\n",
						pipeRef,(int)direction,(int)number,(int)transferType,(int)maxPacketSize,(int)interval,pair_index));
					if (c->in_pipe_ref == 0 &&
					    direction == kUSBIn &&
					    transferType == kUSBBulk)
					{
						if ((pass == 1 && input_pipe_number != 0xff && number == input_pipe_number) ||
						    (pass == 2 && port_number != 0xff && number == port_number) ||
						    (pass == 3 && ((port_number != 0xff && pair_index == port_number) || (port_number == 0xff && maxPacketSize == 64))) ||
						     pass == 4)
							c->in_pipe_ref = pipeRef;
					}
					else if (c->out_pipe_ref == 0 &&
							 direction == kUSBOut &&
							 transferType == kUSBBulk)
					{
						if ((pass == 1 && output_pipe_number != 0xff && number == output_pipe_number) ||
						    (pass == 2 && port_number != 0xff && number == port_number) ||
						    (pass == 3 && ((port_number != 0xff && pair_index == port_number) || (port_number == 0xff && maxPacketSize == 64))) ||
						     pass == 4)
						{
							c->out_pipe_ref = pipeRef;
							c->out_pipe_bulk_size = maxPacketSize;
						}
					}
				}
			}
		}

		if (c->in_pipe_ref && c->out_pipe_ref)
			return kIOReturnSuccess;

		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: couldn't find any suitable pipes pair on this interface\n"));
		(*c->interface)->USBInterfaceClose (c->interface);
		(*c->interface)->Release (c->interface);
		c->interface = NULL;
		c->in_pipe_ref = 0;
		c->out_pipe_ref = 0;
	}

	return c->interface==NULL ? -1 : kIOReturnSuccess;
}

static IOReturn
control_request(IOUSBDeviceInterface **dev, UInt8 requestType, UInt8 request, UInt16 value, UInt16 index, void *pData, UInt16 maxReplyLength)
{
	IOReturn kr;
	IOUSBDevRequest req;
	void *pReply = pData;

	if (!pReply && maxReplyLength)
		pReply = malloc(maxReplyLength);

	req.bmRequestType = requestType;	/* i.e. 0xc2=kUSBIn, kUSBVendor, kUSBEndpoint */
	req.bRequest = request;
	req.wValue = value;
	req.wIndex = index;
	req.wLength = maxReplyLength;
	req.pData = pReply;
	req.wLenDone = 0;

	kr = (*dev)->DeviceRequest (dev, &req);

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: control_request(0x%02x) wLenDone=%d kr=0x%08lx\n", (int)request, req.wLenDone, kr));

	if (pReply && !pData)
		free (pReply);

	return kr;
}

static IOReturn
read_visor_connection_information (IOUSBDeviceInterface **dev, int *port_number, int *input_pipe, int *output_pipe)
{
	int i;
	kern_return_t kr;
	visor_connection_info ci;

	memset(&ci, 0, sizeof(ci));
	kr = control_request (dev, 0xc2, VISOR_GET_CONNECTION_INFORMATION, 0, 0, &ci, sizeof(ci));
	if (kr != kIOReturnSuccess)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: VISOR_GET_CONNECTION_INFORMATION failed (err=%08x)\n", kr));
	}
	else
	{
		CHECK(PI_DBG_DEV, PI_DBG_LVL_DEBUG, pi_dumpdata((const char *)&ci, sizeof(ci)));
		ci.num_ports >>= 8;		/* number of ports is little-endian */
		if (ci.num_ports > 8)
			ci.num_ports = 8;
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: VISOR_GET_CONNECTION_INFORMATION, num_ports=%d\n", ci.num_ports));
		kr = kIOReturnNotReady;
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
					if (port_number)
						*port_number = ci.connections[i].port;
					kr = kIOReturnSuccess;
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
decode_generic_connection_information(palm_ext_connection_info *ci, int *port_number, int *input_pipe, int *output_pipe)
{
	int i;

	CHECK(PI_DBG_DEV, PI_DBG_LVL_DEBUG, pi_dumpdata((const char *)ci, sizeof(palm_ext_connection_info)));
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: decode_generic_connection_information num_ports=%d, endpoint_numbers_different=%d\n", ci->num_ports, ci->endpoint_numbers_different));

	for (i=0; i < ci->num_ports; i++)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] port_function_id=0x%08lx ('%4.4s')\n", i, ci->connections[i].port_function_id, (char*)&ci->connections[i].port_function_id));
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] port=%d\n", i, ci->connections[i].port));
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "\t[%d] endpoint_info=%d (0x%02x)\n", i, ci->connections[i].endpoint_info, (int)ci->connections[i].endpoint_info));
		if (ci->connections[i].port_function_id == 'Lsfr')
		{
			/* This is a T5 in USB connected but not synchronizing:
			 * don't bother trying to talk to it
			 */
			return kIOReturnNotReady;
		}
		if (ci->connections[i].port_function_id == 'cnys')
		{
			/* 'sync': we found the port/pipes to use for synchronization
			 * If endpoint_numbers_different is != 0, then the number of each
			 * endpoint to use for IN and OUT is stored in endpoint_info.
			 * Otherwise, the port number (same for both endpoints) is in
			 * port.
			 */
			if (!ci->endpoint_numbers_different)
			{
				if (port_number)
					*port_number = ci->connections[i].port;
			}
			else if (ci->connections[i].endpoint_info)
			{
				if (input_pipe)
					*input_pipe = ci->connections[i].endpoint_info >> 4;
				if (output_pipe)
					*output_pipe = ci->connections[i].endpoint_info & 0x0f;
			}
		}
	}

	return kIOReturnSuccess;
}

static IOReturn
read_generic_connection_information (IOUSBDeviceInterface **dev, int *port_number, int *input_pipe, int *output_pipe)
{
	kern_return_t  kr;
	palm_ext_connection_info ci;

	memset(&ci, 0, sizeof(ci));
	kr = control_request (dev, 0xc2, PALM_GET_EXT_CONNECTION_INFORMATION, 0, 0, &ci, sizeof(ci));
	if (kr != kIOReturnSuccess)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: PALM_GET_EXT_CONNECTION_INFORMATION failed (err=%08x)\n", kr));
		return kr;
	}
	return decode_generic_connection_information(&ci, port_number, input_pipe, output_pipe);
}

static IOReturn
klsi_set_portspeed(IOUSBDeviceInterface **dev, int speed)
{
	/* set the comms speed for a KLSI serial adapter (PalmConnect USB) */
	kern_return_t kr;
	klsi_port_settings settings = {5, KLSI_BAUD_9600, 8, KLSI_PARITY_NONE, KLSI_STOPBITS_0};
	switch (speed)
	{
		case 230400:	settings.BaudRateIndex = KLSI_BAUD_230400;	break;
		case 115200:	settings.BaudRateIndex = KLSI_BAUD_115200;	break;
		case 57600:	settings.BaudRateIndex = KLSI_BAUD_57600;	break;
		case 38400:	settings.BaudRateIndex = KLSI_BAUD_38400;	break;
		case 28800:	settings.BaudRateIndex = KLSI_BAUD_28800;	break;
		case 19200:	settings.BaudRateIndex = KLSI_BAUD_19200;	break;
		case 14400:	settings.BaudRateIndex = KLSI_BAUD_14400;	break;
		case 9600:	settings.BaudRateIndex = KLSI_BAUD_9600;	break;
		case 7200:	settings.BaudRateIndex = KLSI_BAUD_7200;	break;
		case 4800:	settings.BaudRateIndex = KLSI_BAUD_4800;	break;
		case 2400:	settings.BaudRateIndex = KLSI_BAUD_2400;	break;
		case 1200:	settings.BaudRateIndex = KLSI_BAUD_1200;	break;
		case 600:	settings.BaudRateIndex = KLSI_BAUD_600;		break;
		default:							break;
	}

	kr = control_request(dev, 0x40, KLSI_SET_COMM_DESCRIPTOR, 0, 0, &settings, 5);
	if (kr == kIOReturnSuccess)
	{
		kr = control_request(dev, 0x40, KLSI_SET_COMM_DESCRIPTOR, 0, 0, &settings, 5);
		if (kr == kIOReturnSuccess)
			kr = control_request(dev, 0x40, KLSI_SET_FLOWCONTROL, (speed > 9600) ? KLSI_FLOW_USE_RTS : 0, 0, NULL, 0);

		control_request(dev, 0x40, KLSI_SET_HANDSHAKE_LINES, KLSI_SETHS_DTR | KLSI_SETHS_RTS, 0, NULL, 0);
	}
	return kr;
}

static void
device_notification(usb_connection_t *c, io_service_t service, natural_t messageType, void *messageArgument)
{
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: device_notification (c=%p messageType=0x%08lx)\n", c, messageType));

	if (messageType == kIOMessageServiceIsTerminated && c != NULL)
	{
		c->device_present = 0;
		c->opened = 0;		/* so that stop_listening() does'nt try to send the control_request */
		if (change_refcount(c,-1) > 0)
		{
			/* In case the reading thread is waiting for data,
			 * we need to raise the usb_data_available cond once.
			 * since darwin_usb_read tests usb.opened, it will
			 * gracefully exit during a data wait.
			 */
			ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: device_notification signaling c->read_queue_data_avail_cond for c=%p\n",c));
			pthread_mutex_lock (&c->read_queue_mutex);
			pthread_cond_signal (&c->read_queue_data_avail_cond);
		    	pthread_mutex_unlock (&c->read_queue_mutex);
		}
	}
}

static void
read_completion (usb_connection_t *c, IOReturn result, void *arg0)
{
	size_t bytes_read = (size_t) arg0;

	ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: read_completion(c=%p, result=0x%08lx, bytes_read=%d)\n", c, (long)result, bytes_read));

	if (!c->opened)
	{
	    	change_refcount(c, -1);
	    	return;
	}

	if (result != kIOReturnSuccess)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_WARN, "darwinusb: async read completion(%p) received error code 0x%08x\n", c, result));
	}
	else
	{
		if (c->vendorID == VENDOR_PALMONE && c->productID == PRODUCT_PALMCONNECT_USB)
		{
			/* decode PalmConnect USB frame */
			if (bytes_read < 2)
				bytes_read = 0;
			else
			{
				int data_size = c->read_buffer[0] | ((int)c->read_buffer[1] << 8);
				if ((data_size + 2) > bytes_read)
				{
					LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: invalid PalmConnect packet (%d bytes, says %d content bytes)\n",
						bytes_read, data_size));
					bytes_read = 0;
				} else {
					memmove(&c->read_buffer[0], &c->read_buffer[2], data_size);
					bytes_read = data_size;
				}
			}
		}
		if (bytes_read > 0)
		{		
//pi_dumpdata(c->read_buffer, bytes_read);
			pthread_mutex_lock(&c->read_queue_mutex);
			if (c->read_queue == NULL)
			{
				c->read_queue_size = ((bytes_read + 0xfffe) & ~0xffff) - 1;		/* 64k chunks */
				c->read_queue = (char *) malloc (c->read_queue_size);
				c->read_queue_used = 0;
			}
			else if ((c->read_queue_used + bytes_read) > c->read_queue_size)
			{
				c->read_queue_size += ((bytes_read + 0xfffe) & ~0xffff) - 1;
				c->read_queue = (char *) realloc (c->read_queue, c->read_queue_size);
			}
			if (c->read_queue)
			{
				memcpy(c->read_queue + c->read_queue_used, c->read_buffer, bytes_read);
				c->read_queue_used += bytes_read;
				ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: signaling c->read_queue_data_avail_cond for c=%p\n",c));
				pthread_cond_signal(&c->read_queue_data_avail_cond);
			}
			else
			{
				c->read_queue_used = 0;
				c->read_queue_size = 0;
			}
			pthread_mutex_unlock(&c->read_queue_mutex);
		}
	}

	if (result != kIOReturnAborted && c->opened && usb_run_loop)
	{
		if (result == kIOUSBPipeStalled)
		{
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: input pipe stalled\n"));
			(*c->interface)->ClearPipeStall (c->interface, c->in_pipe_ref);
		}
		prime_read(c);
	}

	change_refcount(c, -1);
}

static int
prime_read(usb_connection_t *c)
{
	/* increment refcount */
	if (pthread_mutex_lock(&c->ref_count_mutex) != 0)
	{
		/* c became invalid? */
        	LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb:  prime_read(%p): can't lock c->ref_count_mutex (structure freed?)\n", c));
        	return 0;
	}
	c->ref_count++;
	pthread_mutex_unlock(&c->ref_count_mutex);

	if (c->opened)
	{
		/* select a correct read size (always use a multiple of the USB packet size) */
		if (c->vendorID == VENDOR_PALMONE && c->productID == PRODUCT_PALMCONNECT_USB)
		{
			/* with PalmConnect USB, always use 64 bytes */
			c->last_read_ahead_size = 64;
		}
		else
		{
			c->last_read_ahead_size = c->read_ahead_size & ~63;
			if (c->last_read_ahead_size <= 0)
				c->last_read_ahead_size = c->auto_read_size;
			if (c->last_read_ahead_size > MAX_AUTO_READ_SIZE)
				c->last_read_ahead_size = MAX_AUTO_READ_SIZE;
			else if (c->last_read_ahead_size <= 0)
				c->last_read_ahead_size = 64;			// USB packet size
		}

		ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: prime_read(%p) for %d bytes\n", c, c->last_read_ahead_size));

		IOReturn kr = (*c->interface)->ReadPipeAsyncTO (c->interface, c->in_pipe_ref,
				c->read_buffer, c->last_read_ahead_size, 5000, 5000, (IOAsyncCallback1)&read_completion, (void *)c);

		if (kr == kIOUSBPipeStalled)
		{
			ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: stalled -- clearing stall and re-priming\n"));
			(*c->interface)->ClearPipeStall (c->interface, c->in_pipe_ref);
			kr = (*c->interface)->ReadPipeAsyncTO (c->interface, c->in_pipe_ref,
					c->read_buffer, c->last_read_ahead_size, 5000, 5000, (IOAsyncCallback1)&read_completion, (void *)c);
		}
		if (kr == kIOReturnSuccess)
		{
			c->read_pending = 1;
			return 1;
		}
		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb:  prime_read(%p): ReadPipeAsync returned error 0x%08x\n", c, kr));
	}

	change_refcount(c, -1);
	return 0;
}

static int
accepts_device(unsigned short vendor, unsigned short product, unsigned short *flags)
{
	int i;
	for (i=0; i < (int)(sizeof(acceptedDevices) / sizeof(acceptedDevices[0])); i++)
	{
		if (vendor == acceptedDevices[i].vendorID)
		{
			if (acceptedDevices[i].flags & FLAG_REJECT)
				return 0;
			if (acceptedDevices[i].productID == 0xffff || product == acceptedDevices[i].productID)
			{
				if (flags)
					*flags = acceptedDevices[i].flags;
				return 1;
			}
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
u_flush(pi_socket_t *ps, int flags)
{
	usb_connection_t *c = ((pi_usb_data_t *)ps->device->data)->ref;
	if (change_refcount(c, +1) <= 0)
	    return PI_ERR_SOCK_DISCONNECTED;
	if (!c->opened)
	{
		change_refcount(c, -1);
		return PI_ERR_SOCK_DISCONNECTED;
	}
	if (flags & PI_FLUSH_INPUT)
	{
		pthread_mutex_lock(&c->read_queue_mutex);
		c->read_queue_used = 0;
		pthread_mutex_unlock(&c->read_queue_mutex);
	}
	change_refcount(c, -1);
	return 0;
}

static int
u_open(struct pi_socket *ps, struct pi_sockaddr *addr, size_t addrlen)
{
	pthread_mutex_lock(&usb_thread_ready_mutex);
	if (usb_thread == 0)
	{
		/* thread doesn't exist yet: create it and wait for
		 * the init phase to be either successful or failed
		 */
		pthread_create(&usb_thread, NULL, usb_thread_run, NULL);
		pthread_cond_wait(&usb_thread_ready_cond, &usb_thread_ready_mutex);
		pthread_mutex_unlock(&usb_thread_ready_mutex);
		if (usb_thread != 0)
			return 1;

		errno = EINVAL;
		return pi_set_error(ps->sd, PI_ERR_GENERIC_SYSTEM);
	}
	pthread_mutex_unlock(&usb_thread_ready_mutex);
	return 1;
}

static int
u_close(struct pi_socket *ps)
{
	usb_connection_t *c = ((pi_usb_data_t *)ps->device->data)->ref;
	ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: u_close(ps=%p c=%p\n",ps,c));
	if (c && change_refcount(c, 1) > 0) 
	{
		//if (c->vendorID == VENDOR_PALMONE && c->productID == PRODUCT_PALMCONNECT_USB)
		//	control_request(c->device, 0x40, KLSI_SET_HANDSHAKE_LINES, KLSI_SETHS_RTS, 0, NULL, 0);

		c->opened = 0;		/* set opened to 0 so that other threads don't try to acquire this connection, as it is on the way out */
		c->ps = NULL;
		change_refcount(c, -2);	/* decrement current refcount + disconnect */
	}
	return close(ps->sd);
}

static int
u_wait_for_device(struct pi_socket *ps, int *timeout)
{
	usb_connection_t *c = connection_for_socket(ps);
	struct timespec to, when;

	ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: u_wait_for_device(ps=%p c=%p, timeout=%d)\n",ps,c,timeout ? *timeout : 0));

	if (timeout && *timeout)
	{
		pi_timeout_to_timespec(*timeout, &when);
		to.tv_sec = *timeout / 1000;
		to.tv_nsec = (*timeout % 1000) * 1000 * 1000;
	}

	if (c == NULL)
	{
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: u_wait_for_device -> waiting for a connection to come up\n"));
		pthread_mutex_lock(&usb_connections_mutex);
		if (timeout && *timeout)
		{
			if (pthread_cond_timedwait_relative_np(&usb_connection_added_cond, &usb_connections_mutex, &to) == ETIMEDOUT)
//			if (pthread_cond_timedwait(&usb_connection_added_cond, &usb_connections_mutex, &when) == ETIMEDOUT)
			{
				LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: u_wait_for_device -> connection wait timed out\n"));
				pthread_mutex_unlock(&usb_connections_mutex);
				return PI_ERR_SOCK_TIMEOUT;
			}
		}
		else
			pthread_cond_wait(&usb_connection_added_cond, &usb_connections_mutex);
		pthread_mutex_unlock(&usb_connections_mutex);

		c = connection_for_socket(ps);

		ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: u_wait_for_device -> end of wait, c=%p\n",c));

		if (c && c->vendorID==VENDOR_PALMONE && c->productID==PRODUCT_PALMCONNECT_USB)
		{
			/* when working with a PalmConnect USB, make sure we use the right speed 
			 * then let the adapter send us data
			 */
			klsi_set_portspeed(c->device, ((pi_usb_data_t *)ps->device->data)->rate);
		}

		if (timeout && *timeout)
		{
			/* if there was a timeout, compute the remaining timeout time */
			*timeout = pi_timespec_to_timeout(&when);
		}
	}
	return (c != NULL);
}

static int
u_poll(struct pi_socket *ps, int timeout)
{
	usb_connection_t *c = connection_for_socket(ps);
	if (c == NULL)
		return PI_ERR_SOCK_DISCONNECTED;

	struct timespec to;

	ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: u_poll(ps=%p c=%p, timeout=%d)\n",ps,c,timeout));

	if (timeout)
	{
		to.tv_sec = timeout / 1000;
		to.tv_nsec = (timeout % 1000) * 1000 * 1000;
	}

	pthread_mutex_lock(&c->read_queue_mutex);
	int available = c->read_queue_used;
	if (!available)
	{
		ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: u_poll -> waiting data to be available for c=%p\n",c));
		if (timeout)
		{
			if (pthread_cond_timedwait_relative_np(&c->read_queue_data_avail_cond, &c->read_queue_mutex, &to) == ETIMEDOUT)
			{
				ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: u_poll -> data wait timed out\n"));
				available = PI_ERR_SOCK_TIMEOUT;
			}
			else
				available = c->read_queue_used;
		}
		else
		{
			/* wait forever for some data to arrive */
			pthread_cond_wait(&c->read_queue_data_avail_cond, &c->read_queue_mutex);
			available = c->read_queue_used;
		}
	}
	pthread_mutex_unlock(&c->read_queue_mutex);
	change_refcount(c, -1);

	ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: u_poll -> end, c=%p, result=%d\n",c,available));

	return available;
}

static ssize_t
u_write(struct pi_socket *ps, const unsigned char *buf, size_t len, int flags)
{
	IOReturn kr;
	size_t	transferred_bytes = 0,
		data_size;
	usb_connection_t *c = ((pi_usb_data_t *)ps->device->data)->ref;
	if (change_refcount(c,+1)<=0 || !c->opened)
	{
		/* make sure we report broken connections */
		if (ps->state == PI_SOCK_CONN_ACCEPT || ps->state == PI_SOCK_CONN_INIT)
			ps->state = PI_SOCK_CONN_BREAK;
		change_refcount(c, -1);
		return PI_ERR_SOCK_DISCONNECTED;
	}

	ULOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: u_write(ps=%p, c=%p, len=%d, flags=%d)\n",ps,c,(int)len,flags));

	if (c->vendorID == VENDOR_PALMONE && c->productID == PRODUCT_PALMCONNECT_USB)
	{
		/* format packets for the PalmConnect USB adapter */
		unsigned char *packet = malloc(c->out_pipe_bulk_size);
		if (packet == NULL)
			return pi_set_error(ps->sd, PI_ERR_GENERIC_MEMORY);

		while (len > 0)
		{
			data_size = len;
			if (data_size > (size_t)(c->out_pipe_bulk_size - 2))
				data_size = (size_t)(c->out_pipe_bulk_size - 2);
			packet[0] = data_size;
			packet[1] = data_size >> 8;
			memcpy(&packet[2], &buf[transferred_bytes], data_size);

			kr = (*c->interface)->WritePipeTO(c->interface, c->out_pipe_ref, packet, c->out_pipe_bulk_size, 5000, 5000);
			if (kr != kIOReturnSuccess) {
				LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: darwin_usb_write(): WritePipe returned kr=0x%08lx\n", kr));
				break;
			}

			transferred_bytes += data_size;
			len -= data_size;
		}
		free(packet);
	}
	else
	{
		kr = (*c->interface)->WritePipeTO(c->interface, c->out_pipe_ref, (void *)buf, len, 5000, 5000);
		if (kr != kIOReturnSuccess) {
			LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "darwinusb: darwin_usb_write(): WritePipe returned kr=0x%08lx\n", kr));
		} else
			transferred_bytes = len;
	}

	if (change_refcount(c, -1) <= 0)
		return PI_ERR_SOCK_DISCONNECTED;

	return transferred_bytes;
}

static ssize_t
u_read(struct pi_socket *ps, pi_buffer_t *buf, size_t len, int flags)
{
	int timeout = ((struct pi_usb_data *)ps->device->data)->timeout;
	int timed_out = 0;
	usb_connection_t *c = ((pi_usb_data_t *)ps->device->data)->ref;

	if (change_refcount(c,+1)<=0 || !c->opened)
	{
		/* make sure we report broken connections */
		if (ps->state == PI_SOCK_CONN_ACCEPT || ps->state == PI_SOCK_CONN_INIT)
			ps->state = PI_SOCK_CONN_BREAK;
		if (c != NULL)
			change_refcount(c, -1);
		return PI_ERR_SOCK_DISCONNECTED;
	}

	if (pi_buffer_expect (buf, len) == NULL)
	{
		errno = ENOMEM;
		change_refcount(c, -1);
		return pi_set_error(ps->sd, PI_ERR_GENERIC_MEMORY);
	}

#ifdef DEBUG_USB
	struct timeval startTime, endTime;
	gettimeofday(&startTime, NULL);
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: u_read(ps=%p, c=%p, len=%d, timeout=%d, flags=%d)\n", ps, c, (int)len, timeout, flags));
#endif

	pthread_mutex_lock(&c->read_queue_mutex);

	if (flags == PI_MSG_PEEK && len > 256)
		len = 256;

	if (c->read_queue_used < len)
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
		do
		{
			/* next prime_read() will use a bigger read request */
			c->read_ahead_size = len - c->read_queue_used - c->last_read_ahead_size;
			if (timeout)
			{
				if (pthread_cond_timedwait(&c->read_queue_data_avail_cond, &c->read_queue_mutex, &when) == ETIMEDOUT)
				{
					timed_out = 1;
					break;
				}
			}
			else
				pthread_cond_wait(&c->read_queue_data_avail_cond, &c->read_queue_mutex);
		}
		while (c->opened && (c->read_queue_used < len || (flags == PI_MSG_PEEK && c->read_queue_used >= 256)));

		c->read_ahead_size = 0;
	}

	if (!c->opened)
	{
		/* make sure we report broken connections */
		if (ps->state == PI_SOCK_CONN_ACCEPT || ps->state == PI_SOCK_CONN_INIT)
			ps->state = PI_SOCK_CONN_BREAK;
		len = PI_ERR_SOCK_DISCONNECTED;
	}
	else
	{
		if (c->read_queue_used < len)
			len = c->read_queue_used;

		if (len)
		{
			pi_buffer_append (buf, c->read_queue, len);

			if (flags != PI_MSG_PEEK)
			{
				c->read_queue_used -= len;
				if (c->read_queue_used > 0)
					memmove(c->read_queue, c->read_queue + len, c->read_queue_used);
				if ((c->read_queue_size - c->read_queue_used) > (16L * 65535L))
				{
					/* if we have more than 1M free in the read queue, we'd better
					 * shrink the buffer
					 */
					c->read_queue_size = ((c->read_queue_used + 0xfffe) & ~0xffff) - 1;
					c->read_queue = (char *) realloc (c->read_queue, c->read_queue_size);
				}
			}
		}
		else if (timed_out)
			len = PI_ERR_SOCK_TIMEOUT;
	}

#ifdef DEBUG_USB
	double a,b;
	gettimeofday(&endTime, NULL);
	a = (double)startTime.tv_sec + (double)startTime.tv_usec / (double)1000000;
	b = (double)endTime.tv_sec + (double)endTime.tv_usec / (double)1000000;
	if (len >= 0) {
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: -> u_read complete (bytes_read=%d, remaining bytes in queue=%d) in %.06fs\n",
		     len, c->read_queue_used,b-a));
	} else {
		LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: -> u_read end with error (err=%d) in %.06fs\n",
		     len,b-a));
	}
#endif

	pthread_mutex_unlock(&c->read_queue_mutex);
	if (change_refcount(c, -1) <= 0)
		len = PI_ERR_SOCK_DISCONNECTED;
	return len;
}

static int
u_changebaud(pi_socket_t *ps)
{
	/* Change the baud rate. This is only useful for serial-to-USB adapters,
	 * as these adapters need to know which rate we use to talk to the device.
	 * We currently only support the PalmConnect USB adapter.
	 */
	pi_usb_data_t *data = (pi_usb_data_t *)ps->device->data;
	usb_connection_t *c = data->ref;
	if (c == NULL)
		return PI_ERR_SOCK_DISCONNECTED;

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "darwinusb: u_changebaud(ps=%p, c=%p, rate=%d)\n", ps, c, data->rate));

	if (c->vendorID == VENDOR_PALMONE && c->productID == PRODUCT_PALMCONNECT_USB)
		return klsi_set_portspeed(c->device, data->rate);

	return 0;
}

void
pi_usb_impl_init (struct pi_usb_impl *impl)
{
	impl->open		= u_open;
	impl->close		= u_close;
	impl->write		= u_write;
	impl->read		= u_read;
	impl->flush		= u_flush;
	impl->poll		= u_poll;
	impl->wait_for_device	= u_wait_for_device;
	impl->changebaud	= u_changebaud;
	impl->control_request	= NULL;   /* that is, until we factor out common code */
}

/* vi: set ts=4 sw=4 sts=4 noexpandtab: cin */
/* Local Variables: */
/* indent-tabs-mode: t */
/* c-basic-offset: 4 */
/* End: */