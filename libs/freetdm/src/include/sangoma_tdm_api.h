/*****************************************************************************
 * sangoma_tdm_api.h	Sangoma TDM API Portability functions
 *
 * Author(s):	Anthony Minessale II <anthmct@yahoo.com>
 *              Nenad Corbic <ncorbic@sangoma.com>
 *				Michael Jerris <mike@jerris.com>
 *				David Rokhvarg <davidr@sangoma.com>
 *
 * Copyright:	(c) 2006 Nenad Corbic <ncorbic@sangoma.com>
 *                       Anthony Minessale II
 *				(c) 1984-2007 Sangoma Technologies Inc.
 *
 * ============================================================================
 */

#ifndef _SANGOMA_TDM_API_H
#define _SANGOMA_TDM_API_H

/* This entire block of defines and includes from this line, through #define FNAME_LEN probably dont belong here */
/* most of them probably belong in wanpipe_defines.h, then each header file listed included below properly included */
/* in the header files that depend on them, leaving only the include for wanpipe_tdm_api.h left in this file or */
/* possibly integrating the rest of this file diretly into wanpipe_tdm_api.h */
#ifndef __WINDOWS__
#if defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32)
#define __WINDOWS__
#endif /* defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32) */
#endif /* ndef __WINDOWS__ */

#if defined(__WINDOWS__)
#if defined(_MSC_VER)
/* disable some warnings caused by wanpipe headers that will need to be fixed in those headers */
#pragma warning(disable:4201 4214)

/* sang_api.h(74) : warning C4201: nonstandard extension used : nameless struct/union */

/* wanpipe_defines.h(219) : warning C4214: nonstandard extension used : bit field types other than int */
/* wanpipe_defines.h(220) : warning C4214: nonstandard extension used : bit field types other than int */
/* this will break for any compilers that are strict ansi or strict c99 */

/* The following definition for that struct should resolve this warning and work for 32 and 64 bit */
#if 0
struct iphdr {
	
#if defined(__LITTLE_ENDIAN_BITFIELD)
	unsigned    ihl:4,
							version:4;
#elif defined (__BIG_ENDIAN_BITFIELD)
	unsigned    version:4,
							ihl:4;
#else
# error  "unknown byteorder!"
#endif
	unsigned    tos:8;
	unsigned		tot_len:16;
	unsigned		id:16;
	unsigned		frag_off:16;
	__u8    ttl;
	__u8    protocol;
	__u16   check;
	__u32   saddr;
	__u32   daddr;
	/*The options start here. */
};
#endif /* #if 0 */

#define __inline__ __inline
#endif /* defined(_MSC_VER) */
#include <windows.h>
/* do we like the name WP_INVALID_SOCKET or should it be changed? */
#define WP_INVALID_SOCKET INVALID_HANDLE_VALUE
#else /* defined(__WINDOWS__) */ 
#define WP_INVALID_SOCKET -1
#include <stropts.h>
#include <poll.h>
#include <sys/socket.h>
#endif

#include <wanpipe_defines.h>
#include <wanpipe_cfg.h>
#include <wanpipe_tdm_api.h>
#include <sdla_te1_pmc.h>
#ifdef __WINDOWS__
#include <sang_status_defines.h>
#include <sang_api.h>
#endif
#include <sdla_aft_te1.h>

#define FNAME_LEN	50

/* what should the returns from this function be?? */
/* I dont think they are currently consistant between windows and *nix */
#ifdef __WINDOWS__
static __inline__ int tdmv_api_ioctl(sng_fd_t fd, wanpipe_tdm_api_t *tdm_api_cmd)
{
	/* can we make the structure passed for this on nix and windows the same */
	/* so we don't have to do the extra 2 memcpy's on windows for this ? */
	wan_udp_hdr_t	wan_udp;
	DWORD			ln;
    unsigned char	id = 0;

	wan_udp.wan_udphdr_request_reply = 0x01;
	wan_udp.wan_udphdr_id			 = id;
   	wan_udp.wan_udphdr_return_code	 = WAN_UDP_TIMEOUT_CMD;

	wan_udp.wan_udphdr_command	= WAN_TDMV_API_IOCTL;
	wan_udp.wan_udphdr_data_len	= sizeof(wanpipe_tdm_api_cmd_t);

	memcpy(	wan_udp.wan_udphdr_data, (void*)tdm_api_cmd, sizeof(wanpipe_tdm_api_cmd_t));

	if (DeviceIoControl(
			fd,
			IoctlManagementCommand,
			(LPVOID)&wan_udp,
			sizeof(wan_udp_hdr_t),
			(LPVOID)&wan_udp,
			sizeof(wan_udp_hdr_t),
			(LPDWORD)(&ln),
			(LPOVERLAPPED)NULL
			) == FALSE){
		return 1;
	}

	if (wan_udp.wan_udphdr_return_code != WAN_CMD_OK){
		return 2;
	}

	memcpy(	(void*)tdm_api_cmd, wan_udp.wan_udphdr_data, sizeof(wanpipe_tdm_api_cmd_t));
	return 0;
}

#else
static __inline__ int tdmv_api_ioctl(sng_fd_t fd, wanpipe_tdm_api_t *tdm_api_cmd)
{
	return ioctl(fd, SIOC_WANPIPE_TDM_API, tdm_api_cmd);
}
#endif

void __inline__ tdmv_api_close_socket(sng_fd_t *sp) 
{
	if (	*sp != WP_INVALID_SOCKET){
#if defined(__WINDOWS__)
		CloseHandle(*sp);
#else
		close(*sp);
#endif
		*sp = WP_INVALID_SOCKET;
	}
}

static __inline__ sng_fd_t tdmv_api_open_span_chan(int span, int chan) 
{
   	char fname[FNAME_LEN];
	sng_fd_t fd = WP_INVALID_SOCKET;
#if defined(__WINDOWS__)
	DWORD			ln;
	wan_udp_hdr_t	wan_udp;

	/* NOTE: under Windows Interfaces are zero based but 'chan' is 1 based. */
	/*		Subtract 1 from 'chan'. */
	_snprintf(fname , FNAME_LEN, "\\\\.\\WANPIPE%d_IF%d", span, chan - 1);

	fd = CreateFile(	fname, 
						GENERIC_READ | GENERIC_WRITE, 
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						(LPSECURITY_ATTRIBUTES)NULL, 
						OPEN_EXISTING,
						FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
						(HANDLE)NULL
						);

	/* make sure that we are the only ones who have this chan open */
	/* is this a threadsafe way to make sure that we are ok and will */
	/* never return a valid handle to more than one thread for the same channel? */

	wan_udp.wan_udphdr_command = GET_OPEN_HANDLES_COUNTER; 
	wan_udp.wan_udphdr_data_len = 0;

	DeviceIoControl(
		fd,
		IoctlManagementCommand,
		(LPVOID)&wan_udp,
		sizeof(wan_udp_hdr_t),
		(LPVOID)&wan_udp,
		sizeof(wan_udp_hdr_t),
		(LPDWORD)(&ln),
		(LPOVERLAPPED)NULL
		);

	if ((wan_udp.wan_udphdr_return_code) || (*(int*)&wan_udp.wan_udphdr_data[0] != 1)){
		/* somone already has this channel, or somthing else is not right. */
		tdmv_api_close_socket(&fd);
	}

#else
	/* Does this fail if another thread already has this chan open? */
	/* if not, we need to add some code to make sure it does */
	snprintf(fname, FNAME_LEN, "/dev/wptdm_s%dc%d",span,chan);

	fd = open(fname, O_RDWR);

	if (fd < 0) {
		fd = WP_INVALID_SOCKET;
	}

#endif
	return fd;  
}            

#if defined(__WINDOWS__)
/* This might be broken on windows, as POLL_EVENT_TELEPHONY seems to be commented out in sang_api.h.. it should be added to POLLPRI */
#define POLLPRI (POLL_EVENT_LINK_STATE | POLL_EVENT_LINK_CONNECT | POLL_EVENT_LINK_DISCONNECT)
#endif

/* return -1 for error, 0 for timeout, or POLLIN | POLLOUT | POLLPRI based on the result of the poll */
/* on windows we actually have POLLPRI defined with several events, so we could theoretically poll */
/* for specific events.  Is there any way to do this on *nix as well? */ 

/* a cross platform way to poll on an actual pollset (span and/or list of spans) will probably also be needed for analog */
/* so we can have one analong handler thread that will deal with all the idle analog channels for events */
/* the alternative would be for the driver to provide one socket for all of the oob events for all analog channels */
static __inline__ int tdmv_api_wait_socket(sng_fd_t fd, int timeout, int *flags)
{
#if defined(__WINDOWS__)
	DWORD ln;
	API_POLL_STRUCT	api_poll;

	memset(&api_poll, 0x00, sizeof(API_POLL_STRUCT));
	
	api_poll.user_flags_bitmap = *flags;
	api_poll.timeout = timeout;

	if (!DeviceIoControl(
			fd,
			IoctlApiPoll,
			(LPVOID)NULL,
			0L,
			(LPVOID)&api_poll,
			sizeof(API_POLL_STRUCT),
			(LPDWORD)(&ln),
			(LPOVERLAPPED)NULL)) {
		return -1;
	}

	*flags = 0;

	switch(api_poll.operation_status)
	{
		case SANG_STATUS_RX_DATA_AVAILABLE:
			break;

		case SANG_STATUS_RX_DATA_TIMEOUT:
			return 0;

		default:
			return -1;
	}

	if (api_poll.poll_events_bitmap == 0){
		return -1;
	}

	if (api_poll.poll_events_bitmap & POLL_EVENT_TIMEOUT) {
		return 0;
	}

	*flags = api_poll.poll_events_bitmap;

	return 1;
#else
    struct pollfd pfds[1];
    int res;

    memset(&pfds[0], 0, sizeof(pfds[0]));
    pfds[0].fd = fd;
    pfds[0].events = *flags;
    res = poll(pfds, 1, timeout);
	*flags = 0;

	if (pfds[0].revents & POLLERR) {
		res = -1;
	}

	if (res > 0) {
		*flags = pfds[0].revents;
	}

    return res;
#endif
}

/* on windows right now, there is no way to specify if we want to read events here or not, we allways get them here */
/* we need some what to select if we are reading regular tdm msgs or events */
/* need to either have 2 functions, 1 for events, 1 for regural read, or a flag on this function to choose */
/* 2 functions preferred.  Need implementation for the event function for both nix and windows that is threadsafe */
static __inline__ int tdmv_api_readmsg_tdm(sng_fd_t fd, void *hdrbuf, int hdrlen, void *databuf, int datalen)
{
	/* What do we need to do here to avoid having to do all */
	/* the memcpy's on windows and still maintain api compat with nix */
	int rx_len=0;
#if defined(__WINDOWS__)
	static RX_DATA_STRUCT	rx_data;
	api_header_t			*pri;
	wp_tdm_api_rx_hdr_t		*tdm_api_rx_hdr;
	wp_tdm_api_rx_hdr_t		*user_buf = (wp_tdm_api_rx_hdr_t*)hdrbuf;
	DWORD ln;

	if (hdrlen != sizeof(wp_tdm_api_rx_hdr_t)){
		return -1;
	}

	if (!DeviceIoControl(
			fd,
			IoctlReadCommand,
			(LPVOID)NULL,
			0L,
			(LPVOID)&rx_data,
			sizeof(RX_DATA_STRUCT),
			(LPDWORD)(&ln),
			(LPOVERLAPPED)NULL
			)){
		return -1;
	}

	pri = &rx_data.api_header;
	tdm_api_rx_hdr = (wp_tdm_api_rx_hdr_t*)rx_data.data;

	user_buf->wp_tdm_api_event_type = pri->operation_status;

	switch(pri->operation_status)
	{
	case SANG_STATUS_RX_DATA_AVAILABLE:
		if (pri->data_length > datalen){
			break;
		}
		memcpy(databuf, rx_data.data, pri->data_length);
		rx_len = pri->data_length;
		break;

	default:
		break;
	}

#else
	struct msghdr msg;
	struct iovec iov[2];

	memset(&msg,0,sizeof(struct msghdr));

	iov[0].iov_len=hdrlen;
	iov[0].iov_base=hdrbuf;

	iov[1].iov_len=datalen;
	iov[1].iov_base=databuf;

	msg.msg_iovlen=2;
	msg.msg_iov=iov;

	rx_len = read(fd,&msg,datalen+hdrlen);

	if (rx_len <= sizeof(wp_tdm_api_rx_hdr_t)){
		return -EINVAL;
	}

	rx_len-=sizeof(wp_tdm_api_rx_hdr_t);
#endif
    return rx_len;
}                    

static __inline__ int tdmv_api_writemsg_tdm(sng_fd_t fd, void *hdrbuf, int hdrlen, void *databuf, unsigned short datalen)
{
	/* What do we need to do here to avoid having to do all */
	/* the memcpy's on windows and still maintain api compat with nix */
	int bsent = 0;
#if defined(__WINDOWS__)
	static TX_DATA_STRUCT	local_tx_data;
	api_header_t			*pri;
	DWORD ln;

	/* Are these really not needed or used???  What about for nix?? */
	(void)hdrbuf;
	(void)hdrlen;

	pri = &local_tx_data.api_header;

	pri->data_length = datalen;
	memcpy(local_tx_data.data, databuf, pri->data_length);

	if (!DeviceIoControl(
			fd,
			IoctlWriteCommand,
			(LPVOID)&local_tx_data,
			(ULONG)sizeof(TX_DATA_STRUCT),
			(LPVOID)&local_tx_data,
			sizeof(TX_DATA_STRUCT),
			(LPDWORD)(&ln),
			(LPOVERLAPPED)NULL
			)){
		return -1;
	}

	if (local_tx_data.api_header.operation_status == SANG_STATUS_SUCCESS) {
		bsent = datalen;
	}
#else
	struct msghdr msg;
	struct iovec iov[2];

	memset(&msg,0,sizeof(struct msghdr));

	iov[0].iov_len = hdrlen;
	iov[0].iov_base = hdrbuf;

	iov[1].iov_len = datalen;
	iov[1].iov_base = databuf;

	msg.msg_iovlen = 2;
	msg.msg_iov = iov;

	bsent = write(fd, &msg, datalen + hdrlen);
	if (bsent > 0){
		bsent -= sizeof(wp_tdm_api_tx_hdr_t);
	}
#endif
	return bsent;
}

#endif /* _SANGOMA_TDM_API_H */
