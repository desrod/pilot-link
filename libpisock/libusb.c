/*
 * libusb.c: device i/o for libusb
 *
 * Copyright (c) 2004 Zephaniah E. Hull & Florent Pillet.
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>		/* Needed for Redhat 6.x machines */
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-usb.h"
#include "pi-util.h"

#ifdef HAVE_SYS_IOCTL_COMPAT_H
#include <sys/ioctl_compat.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <usb.h>


static int u_open(struct pi_socket *ps, struct pi_sockaddr *addr, size_t addrlen);
static int u_close(struct pi_socket *ps);
static ssize_t u_write(struct pi_socket *ps, const unsigned char *buf, size_t len, int flags);
static ssize_t u_read(struct pi_socket *ps, pi_buffer_t *buf, size_t len, int flags);
static int u_read_i(struct pi_socket *ps, pi_buffer_t *buf, size_t len, int flags, int timeout);
static int u_poll(struct pi_socket *ps, int timeout);
static int u_wait_for_device(struct pi_socket *ps, int *timeout);
static int u_flush(pi_socket_t *ps, int flags);
static int u_control_request (pi_usb_data_t *usb_data, int request_type, int request, int value, int control_index, void *data, int size, int timeout);

void pi_usb_impl_init (struct pi_usb_impl *impl)
{
	impl->open 		= u_open;
	impl->close		= u_close;
	impl->write		= u_write;
	impl->read 		= u_read;
	impl->flush		= u_flush;
	impl->poll 		= u_poll;
	impl->wait_for_device	= u_wait_for_device;
	impl->changebaud	= NULL;		/* we don't need this one for libusb (yet) */
	impl->control_request	= u_control_request;
}



/***********************************************************************
 *
 * Start of the device identification code.
 *
 ***********************************************************************/

static usb_dev_handle	*USB_handle;
static int				USB_interface;
static int				USB_in_endpoint;
static int				USB_out_endpoint;

static int
USB_open (pi_usb_data_t *data)
{
	usb_init ();

	return 1;
}

static int
USB_poll (pi_usb_data_t *data)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	int ret;
	u_int8_t input_endpoint = 0xFF, output_endpoint = 0xFF;
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
	int first;
#endif

	usb_find_busses ();
	usb_find_devices ();
	CHECK (PI_DBG_DEV, PI_DBG_LVL_DEBUG, usb_set_debug (2));

	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			int i;
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s: checking device %p\n", __FILE__, dev));

			if (dev->descriptor.bNumConfigurations < 1)
				continue;
			if (!dev->config)
				continue;
			if (dev->config[0].bNumInterfaces < 1)
				continue;
			if (dev->config[0].interface[0].num_altsetting < 1)
				continue;
			if (dev->config[0].interface[0].altsetting[0].bNumEndpoints < 2)
				continue;

			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s: %d, 0x%04x 0x%04x.\n", __FILE__, __LINE__, dev->descriptor.idVendor, dev->descriptor.idProduct));
			if (USB_check_device (data, dev->descriptor.idVendor, dev->descriptor.idProduct))
				continue;

			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s: trying to open device %p\n", __FILE__, dev));
			USB_handle = usb_open(dev);
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s: USB_handle=%p\n", __FILE__, USB_handle));

			data->ref = USB_handle;

			input_endpoint = output_endpoint = 0xFF;
			USB_in_endpoint = USB_out_endpoint = 0xFF;

			ret = USB_configure_device (data, &input_endpoint, &output_endpoint);

			for (i = 0; i < dev->config[0].interface[0].altsetting[0].bNumEndpoints; i++) {
				struct usb_endpoint_descriptor *endpoint;
				u_int8_t address;

				endpoint = &dev->config[0].interface[0].altsetting[0].endpoint[i];

				if (endpoint->wMaxPacketSize != 0x40)
					continue;
				if ((endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK) != USB_ENDPOINT_TYPE_BULK)
					continue;
				address = endpoint->bEndpointAddress;
				if ((address & USB_ENDPOINT_DIR_MASK)) {
					LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "In: 0x%x 0x%x.\n", address, input_endpoint));
					if (input_endpoint == 0xFF)
						USB_in_endpoint = address;
					else if ((address & USB_ENDPOINT_ADDRESS_MASK) == input_endpoint)
						USB_in_endpoint = address;
				} else {
					LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "Out: 0x%x 0x%x.\n", address, output_endpoint));
					if (output_endpoint == 0xFF)
						USB_out_endpoint = address;
					else if ((address & USB_ENDPOINT_ADDRESS_MASK) == output_endpoint)
						USB_out_endpoint = address;
				}
			}

			if (USB_in_endpoint == 0xFF || USB_out_endpoint == 0xFF) {
				usb_close (USB_handle);
				continue;
			}

			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "Config: %d, 0x%x 0x%x | 0x%x 0x%x.\n", ret, input_endpoint, output_endpoint, USB_in_endpoint, USB_out_endpoint));

			USB_interface = dev->config[0].interface[0].altsetting[0].bInterfaceNumber;
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
			first = 1;
claim:
#endif
			i = usb_claim_interface (USB_handle, USB_interface);
			if (i < 0) {
				if (i == -EBUSY) {
					LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "Unable to claim device: Busy.\n"));
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
					if (first) {
						usb_detach_kernel_driver_np (USB_handle, USB_interface);
						first = 0;
						goto claim;
					}
#endif
				} else if (i == -ENOMEM)
					LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "Unable to claim device: No memory.\n"));
				else
					LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "Unable to claim device: %d.\n", i));
				usb_close (USB_handle);

				errno = -i;
				LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s: %d.\n", __FILE__, __LINE__));
				return 0;
			}

			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s: %d.\n", __FILE__, __LINE__));
			return 1;
		}
	}

	errno = ENODEV;
	CHECK (PI_DBG_DEV, PI_DBG_LVL_DEBUG, usb_set_debug (0));
	return 0;
}

static int
USB_close (void)
{
	if (!USB_handle)
		return 0;

	usb_release_interface (USB_handle, USB_interface);
	usb_close (USB_handle);
	USB_handle = NULL;
	return 1;
}


/***********************************************************************
 *
 * Start of the read thread code, please note that all of this runs
 * in a separate thread.
 *
 ***********************************************************************/

#define MAX_READ_SIZE	16384
#define AUTO_READ_SIZE	64
static char				*RD_buffer = NULL;
static size_t			RD_buffer_size;
static size_t			RD_buffer_used;
static pthread_mutex_t	RD_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t	RD_buffer_available_cond = PTHREAD_COND_INITIALIZER;
static int				RD_wanted;
static int				RD_running = 0;
static char				RD_usb_buffer[MAX_READ_SIZE];
static pthread_t		RD_thread = 0;

static void
RD_do_read (int timeout)
{
	int	bytes_read, read_size;

	read_size = RD_wanted - RD_buffer_used;
	if (read_size < AUTO_READ_SIZE)
		read_size = AUTO_READ_SIZE;
	else if (read_size > MAX_READ_SIZE)
		read_size = MAX_READ_SIZE;

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "Reading: len: %d, timeout: %d.\n", read_size, timeout));
	bytes_read = usb_bulk_read (USB_handle, USB_in_endpoint, RD_usb_buffer, read_size, timeout);
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s): %d\n", __FILE__, __LINE__, __FUNCTION__, bytes_read));
	if (bytes_read < 0) {
		if (bytes_read == -ENODEV) {
			LOG((PI_DBG_DEV, PI_DBG_LVL_NONE, "Device went byebye!\n"));
			RD_running = 0;
			return;
#ifdef ELAST
		} else if (bytes_read == -(ELAST + 1)) {
			usb_clear_halt (USB_handle, USB_in_endpoint);
			return;
#endif
		} else if (bytes_read == -ETIMEDOUT)
			return;

		LOG((PI_DBG_DEV, PI_DBG_LVL_ERR, "libusb: USB bulk read returned error code %d\n", bytes_read));
		return;
	}
	if (!bytes_read)
		return;

	
	pthread_mutex_lock (&RD_buffer_mutex);
	if ((RD_buffer_used + bytes_read) > RD_buffer_size) {
		RD_buffer_size = ((RD_buffer_used + bytes_read + 0xfffe) & ~0xffff) - 1;	/* 64k chunks. */
		RD_buffer = realloc (RD_buffer, RD_buffer_size);
	}

	memcpy (RD_buffer + RD_buffer_used, RD_usb_buffer, bytes_read);
	RD_buffer_used += bytes_read;
	pthread_cond_broadcast (&RD_buffer_available_cond);
	pthread_mutex_unlock (&RD_buffer_mutex);
}

static void *
RD_main (void *foo)
{
	RD_buffer_used = 0;
	RD_buffer = NULL;
	RD_buffer_size = 0;

	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	while (RD_running == 1) {
		RD_do_read (0);
	}

	RD_running = 0;

	return NULL;
}


static int
RD_start (void)
{
	if (RD_thread || RD_running)
		return 0;

	RD_running = 1;
	pthread_create (&RD_thread, NULL, RD_main, NULL);

	return 1;
}

static int
RD_stop (void)
{
	int start, now;

	if (!RD_thread || !RD_running)
		return 0;

	RD_running = -1;

	/*
	 * FIXME: Evil kluge.
	 * Handles the case where the app wants to close,
	 * and the PDA stays put and quiet.
	 */
	start = time(NULL);
	while (RD_running) {
		now = time(NULL);
		if ((now - start) >= 5) {
			LOG((PI_DBG_DEV, PI_DBG_LVL_WARN, "libusb: Killing read thread.\n"));
			pthread_cancel (RD_thread);
			RD_running = 0;
			return 1;
		}
		sleep (1);
	}

	pthread_join (RD_thread, NULL);

	/* Clean up the buffers. */
	free (RD_buffer);
	RD_buffer = NULL;
	RD_buffer_size = 0;

	return 1;
}


/***********************************************************************
 *
 * Start of the glue code which makes this whole mess WORK.
 *
 ***********************************************************************/


static int
u_open(struct pi_socket *ps, struct pi_sockaddr *addr, size_t addrlen)
{
	pi_usb_data_t *data = (pi_usb_data_t *)ps->device->data;

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s).\n", __FILE__, __LINE__, __FUNCTION__));
	if (RD_running)
		return -1;
	if (!USB_open (data))
		return -1;
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s).\n", __FILE__, __LINE__, __FUNCTION__));

	return 1;
}

static int
u_close(struct pi_socket *ps)
{
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s).\n", __FILE__, __LINE__, __FUNCTION__));
	RD_stop ();
	USB_close ();
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s).\n", __FILE__, __LINE__, __FUNCTION__));
	return close (ps->sd);
}

static int
u_wait_for_device(struct pi_socket *ps, int *timeout)
{
	pi_usb_data_t *data = (pi_usb_data_t *)ps->device->data;
	struct timespec when;
	int ret = 0;

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s).\n", __FILE__, __LINE__, __FUNCTION__));

	if (*timeout)
		pi_timeout_to_timespec (*timeout, &when);

	while (1) {
		ret = USB_poll (data);
		if (ret > 0) {
			/* Evil, calculate how much longer the timeout is. */
			if (*timeout) {
				*timeout = pi_timespec_to_timeout (&when);
				if (*timeout <= 0)
					*timeout = 1;
			}
			if (!RD_start ()) {
				USB_close ();
				return -1;
			}
			return ret;

		}

		if (*timeout) {
			if (pi_timeout_expired(&when)) {
				*timeout = 1;
				return 0;
			}
		}
		usleep (500000);
	}

	return 0;
}

static int
u_poll(struct pi_socket *ps, int timeout)
{
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s).\n", __FILE__, __LINE__, __FUNCTION__));

	return u_read_i (ps, NULL, 1, PI_MSG_PEEK, timeout);
}

static ssize_t
u_write(struct pi_socket *ps, const unsigned char *buf, size_t len, int flags)
{
	int timeout = ((struct pi_usb_data *)ps->device->data)->timeout;
	int ret;

	if (!RD_running)
		return -1;

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "Writing: len: %d, flags: %d, timeout: %d.\n", len, flags, timeout));
	if (len <= 0)
		return 0;

	ret = usb_bulk_write (USB_handle, USB_out_endpoint, buf, len, timeout);
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "Wrote: %d.\n", ret));
	if (ret > 0)
		CHECK (PI_DBG_DEV, PI_DBG_LVL_DEBUG, pi_dumpdata (buf, ret));

	return (ssize_t)ret;
}

static ssize_t
u_read(struct pi_socket *ps, pi_buffer_t *buf, size_t len, int flags)
{
	int ret;

	ret = u_read_i (ps, buf, len, flags, ((struct pi_usb_data *)ps->device->data)->timeout);
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "Read: %d (%d).\n", ret, len));
	if (ret > 0)
		CHECK (PI_DBG_DEV, PI_DBG_LVL_DEBUG, pi_dumpdata (buf->data, ret));

	return (ssize_t)ret;
}

static int
u_read_i(struct pi_socket *ps, pi_buffer_t *buf, size_t len, int flags, int timeout)
{
	if (!RD_running)
		return PI_ERR_SOCK_DISCONNECTED;

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s): %d %d %d\n", __FILE__, __LINE__, __FUNCTION__, len, flags, timeout));
	pthread_mutex_lock (&RD_buffer_mutex);
	if (flags & PI_MSG_PEEK && len > 256)
		len = 256;

	if (RD_buffer_used < len) {
		struct timeval now;
		struct timespec when, nownow;
		int last_used;
		gettimeofday(&now, NULL);
		when.tv_sec = now.tv_sec + timeout / 1000;
		when.tv_nsec = (now.tv_usec + (timeout % 1000) * 1000) * 1000;
		if (when.tv_nsec >= 1000000000) {
			when.tv_nsec -= 1000000000;
			when.tv_sec++;
		}

		RD_wanted = len;
		do {
			last_used = RD_buffer_used;
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s): %d %d.\n", __FILE__, __LINE__, __FUNCTION__, len, RD_buffer_used));
			if (timeout) {
				gettimeofday(&now, NULL);
				nownow.tv_sec = now.tv_sec;
				nownow.tv_nsec = now.tv_usec * 1000;
				if ((nownow.tv_sec == when.tv_sec ? (nownow.tv_nsec > when.tv_nsec) : (nownow.tv_sec > when.tv_sec))) {
					LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s): %d %d.\n", __FILE__, __LINE__, __FUNCTION__, len, RD_buffer_used));
					break;
				}
				LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s): %d %d.\n", __FILE__, __LINE__, __FUNCTION__, len, RD_buffer_used));
				if (pthread_cond_timedwait (&RD_buffer_available_cond, &RD_buffer_mutex, &when) == ETIMEDOUT) {
					LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s): %d %d.\n", __FILE__, __LINE__, __FUNCTION__, len, RD_buffer_used));
					break;
				}
			} else
				pthread_cond_wait (&RD_buffer_available_cond, &RD_buffer_mutex);
			LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s): %d %d.\n", __FILE__, __LINE__, __FUNCTION__, len, RD_buffer_used));
		} while (RD_buffer_used < len);

		RD_wanted = 0;
	}

	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s): %d %d.\n", __FILE__, __LINE__, __FUNCTION__, len, RD_buffer_used));

	if (!RD_running) {
		pthread_mutex_unlock (&RD_buffer_mutex);
		return PI_ERR_SOCK_DISCONNECTED;
	}

	if (RD_buffer_used < len)
		len = RD_buffer_used;
	
	if (len && buf) {
		pi_buffer_append (buf, RD_buffer, len);
		if (!(flags & PI_MSG_PEEK)) {
			RD_buffer_used -= len;
			if (RD_buffer_used)
				memmove (RD_buffer, RD_buffer + len, RD_buffer_used);

			if ((RD_buffer_size - RD_buffer_used) > (1024 * 1024)) {
				/* If we have more then 1M free in the buffer, shrink it. */
				RD_buffer_size = ((RD_buffer_used + 0xfffe) & ~0xffff) - 1;
				RD_buffer = realloc (RD_buffer, RD_buffer_size);
			}
		}
	}

	pthread_mutex_unlock (&RD_buffer_mutex);
	LOG((PI_DBG_DEV, PI_DBG_LVL_DEBUG, "%s %d (%s).\n", __FILE__, __LINE__, __FUNCTION__));
	return len;
}

static int
u_flush(pi_socket_t *ps, int flags)
{
	if (flags & PI_FLUSH_INPUT) {
		/* clear internal buffer */
		pthread_mutex_lock (&RD_buffer_mutex);
		RD_buffer_used = 0;
		pthread_mutex_unlock (&RD_buffer_mutex);
	}
	return 0;
}

static int
u_control_request (pi_usb_data_t *usb_data, int request_type, int request,
		int value, int control_index, void *data, int size, int timeout)
{
	return usb_control_msg (usb_data->ref, request_type, request, value, control_index, data, size, timeout);
}

/* vi: set ts=4 sw=4 sts=0 noexpandtab: cin */
/* Local Variables: */
/* indent-tabs-mode: t */
/* c-basic-offset: 4 */
/* End: */
