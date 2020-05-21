
/*
 *
 *  This code is derived from BlueZ source code, with following
 *  copyrights:
 *
 *  Copyright (C) 2011-2014  Intel Corporation
 *  Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <time.h>
#include <stdatomic.h>
#include "btlemon.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define le16_to_cpu(val) (val)
#define le32_to_cpu(val) (val)
#define le64_to_cpu(val) (val)
#define cpu_to_le16(val) (val)
#define cpu_to_le32(val) (val)
#define cpu_to_le64(val) (val)
#define be16_to_cpu(val) bswap_16(val)
#define be32_to_cpu(val) bswap_32(val)
#define be64_to_cpu(val) bswap_64(val)
#define cpu_to_be16(val) bswap_16(val)
#define cpu_to_be32(val) bswap_32(val)
#define cpu_to_be64(val) bswap_64(val)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define le16_to_cpu(val) bswap_16(val)
#define le32_to_cpu(val) bswap_32(val)
#define le64_to_cpu(val) bswap_64(val)
#define cpu_to_le16(val) bswap_16(val)
#define cpu_to_le32(val) bswap_32(val)
#define cpu_to_le64(val) bswap_64(val)
#define be16_to_cpu(val) (val)
#define be32_to_cpu(val) (val)
#define be64_to_cpu(val) (val)
#define cpu_to_be16(val) (val)
#define cpu_to_be32(val) (val)
#define cpu_to_be64(val) (val)
#else
#error "Unknown byte order"
#endif

#define BTSNOOP_MAX_PACKET_SIZE		(1486 + 4)
#define MAX_EPOLL_EVENTS 10
#define MGMT_HDR_SIZE	6
#define BTSNOOP_OPCODE_EVENT_PKT 3
#define LE_META_EVENT 0x3e
#define LE_META_EVENT_SIZE 1
#define LE_ADVERTISING_REPORT_EVENT 0x02
#define LE_ADVERTISING_REPORT_EVENT_SIZE 1

typedef void (*callback_func)(int fd);

struct loop_data {
  int fd;
  callback_func callback;
};

static void print_callback(const uint8_t addr[6], const int8_t *rssi, const uint8_t *data, uint8_t data_len) {
  int i;
  printf("%ld %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X %d ", time(NULL),
         addr[5], addr[4], addr[3],
         addr[2], addr[1], addr[0],
         *rssi);
  for (i=0; i<data_len; i++) {
    printf("%2.2X", data[i]);
  }
  printf("\n");
}

static int epoll_fd;
static atomic_int epoll_terminate;
static struct loop_data bt_data;
static struct loop_data sig_data;
static btlemon_callback ble_callback = print_callback;

struct mgmt_hdr {
  uint16_t opcode;
  uint16_t index;
  uint16_t len;
} __attribute__ ((packed));

struct bt_hci_evt_le_adv_report {
  uint8_t  num_reports;
  uint8_t  event_type;
  uint8_t  addr_type;
  uint8_t  addr[6];
  uint8_t  data_len;
  uint8_t  data[0];
} __attribute__ ((packed));

static void le_adv_report_evt(const void *data, uint8_t size) {
  const struct bt_hci_evt_le_adv_report *evt = data;
  uint8_t evt_len;
  int8_t *rssi;

  report:
  rssi = (int8_t *) (evt->data + evt->data_len);
  ble_callback(evt->addr, rssi, evt->data, evt->data_len);
  evt_len = sizeof(*evt) + evt->data_len + 1;
  if (size > evt_len) {
    data += evt_len - 1;
    size -= evt_len - 1;
    evt = data;
    goto report;
  }
}

static void le_meta_event_evt(const void *data, uint8_t size) {
  uint8_t subevent = *((const uint8_t *) data);
  if (subevent == LE_ADVERTISING_REPORT_EVENT) {

    if (size < LE_ADVERTISING_REPORT_EVENT_SIZE) {
      printf("ERROR: too short packet");
      return;
    }

    le_adv_report_evt(data + 1, size - 1);
  }
}

static void packet_hci_event(const void *data, uint8_t size) {
  const hci_event_hdr *hci_hdr = data;

  data += HCI_EVENT_HDR_SIZE;
  size -= HCI_EVENT_HDR_SIZE;

  if (hci_hdr->evt == LE_META_EVENT) {
    if (size != hci_hdr->plen) {
      printf("ERROR: invalid packet size");
      return;
    }

    if (hci_hdr->plen < LE_META_EVENT_SIZE) {
      printf("ERROR: too short packet");
      return;
    }

    le_meta_event_evt(data, hci_hdr->plen);
  }
}

static void sig_callback(int fd) {

  ssize_t result;
  struct signalfd_siginfo si;

  result = read(fd, &si, sizeof(si));
  if (result != sizeof(si)) {
    return;
  }

  switch (si.ssi_signo) {
    case SIGINT:
    case SIGTERM:
      epoll_terminate = 1;
      break;
  }
}

static void data_callback(int fd) {

  unsigned char buf[BTSNOOP_MAX_PACKET_SIZE];
  unsigned char control[64];
  struct mgmt_hdr hdr;
  struct msghdr msg;
  struct iovec iov[2];
  const void *data = buf;

  iov[0].iov_base = &hdr;
  iov[0].iov_len = MGMT_HDR_SIZE;
  iov[1].iov_base = buf;
  iov[1].iov_len = sizeof(buf);

  memset(&msg, 0, sizeof(msg));
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;
  msg.msg_control = control;
  msg.msg_controllen = sizeof(control);

  while(1) {
    uint16_t opcode, index, pktlen;
    ssize_t len;

    len = recvmsg(fd, &msg, MSG_DONTWAIT);
    if (len < 0)
      break;

    if (len < MGMT_HDR_SIZE)
      break;

    opcode = le16_to_cpu(hdr.opcode);
    pktlen = le16_to_cpu(hdr.len);

    if (opcode == BTSNOOP_OPCODE_EVENT_PKT) {
      if (pktlen < HCI_EVENT_HDR_SIZE) {
        return;
      }

      packet_hci_event(data, pktlen);
    }
  }
}

static int connect_socket() {
  int fd;
  struct sockaddr_hci addr;

  fd = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
  if (fd < 0) {
    perror("Failed to open channel");
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.hci_family = AF_BLUETOOTH;
  addr.hci_dev = HCI_DEV_NONE;
  addr.hci_channel = HCI_CHANNEL_MONITOR;

  if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    perror("Failed to bind channel");
    close(fd);
    return -1;
  }

  return fd;
}

static int add_bt_fd() {
  int fd;
  struct epoll_event ev;

  fd = connect_socket();
  if (fd < 0) {
    return -1;
  }

  memset(&bt_data, 0, sizeof(bt_data));
  bt_data.fd = fd;
  bt_data.callback = data_callback;

  memset(&ev, 0, sizeof(ev));
  ev.events = EPOLLIN;
  ev.data.ptr = &bt_data;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
    return -1;
  }

  return fd;
}

static int add_sig_fd() {
  sigset_t mask;
  int fd;
  struct epoll_event ev;

  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);

  if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
    return -1;
  }

  fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
  if (fd < 0) {
    return -1;
  }

  memset(&sig_data, 0, sizeof(sig_data));
  sig_data.fd = fd;
  sig_data.callback = sig_callback;

  memset(&ev, 0, sizeof(ev));
  ev.events = EPOLLIN;
  ev.data.ptr = &sig_data;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
    return -1;
  }

  return fd;
}

static void remove_fd(struct loop_data *data) {
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);
  close(data->fd);
}

void btlemon_set_callback(btlemon_callback callback) {
  ble_callback = callback;
}

void btlemon_stop() {
  epoll_terminate = 1;
}

int btlemon_run(int handle_signal) {
  struct epoll_event events[MAX_EPOLL_EVENTS];

  epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  epoll_terminate = 0;

  if (add_bt_fd() < 0) {
    return -1;
  }

  if (handle_signal && add_sig_fd() < 0) {
    return -1;
  }

  while (!epoll_terminate) {
    int n, nfds;

    nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, 1000);
    if (nfds < 0)
      continue;

    for (n = 0; n < nfds; n++) {
      struct loop_data *data = events[n].data.ptr;
      data->callback(data->fd);
    }
  }

  remove_fd(&bt_data);

  if (handle_signal) {
    remove_fd(&sig_data);
  }

  return 0;
}
