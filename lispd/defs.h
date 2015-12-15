/*
 *
 * Copyright (C) 2011, 2015 Cisco Systems, Inc.
 * Copyright (C) 2015 CBA research group, Technical University of Catalonia.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef DEFS_H_
#define DEFS_H_

#include <stdint.h>

typedef enum {
    xTR_MODE ,
    MS_MODE,
    RTR_MODE,
    MN_MODE
} lisp_dev_type_e;

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

typedef struct lisp_ctrl_dev lisp_ctrl_dev_t;
typedef struct lisp_ctrl lisp_ctrl_t;
typedef struct shash shash_t;
typedef struct fwd_info_ fwd_info_t;
typedef struct sockmstr sockmstr_t;
typedef struct htable_ptrs htable_ptrs_t;
typedef struct data_plane_struct data_plane_struct_t;
typedef struct htable_nonces_ htable_nonces_t;

/* Protocols constants related with timeouts */
#define LISPD_INITIAL_MRQ_TIMEOUT       2  // Initial expiration timer for the first MRq
#define LISPD_INITIAL_SMR_TIMEOUT       3  // Initial expiration timer for the first MRq SMR
#define LISPD_INITIAL_MREG_TIMEOUT      3  // Initial expiration timer for the first Encapsulated Map Register
#define LISPD_SMR_TIMEOUT               5  // Time since interface status change until balancing arrays and SMR is done
#define LISPD_MAX_MRQ_TIMEOUT           32 // Max expiration timer for the subsequent MRq
#define LISPD_EXPIRE_TIMEOUT            1  // Time interval in which events are expired
#define LISPD_MAX_MR_RETRANSMIT         2  // Maximum amount of Map Request retransmissions
#define LISPD_MAX_SMR_RETRANSMIT        2  // Maximum amount of SMR MRq retransmissions
#define LISPD_MAX_PROBE_RETRANSMIT      1  // Maximum amount of RLOC probe MRq retransmissions
#define LISPD_MAX_RETRANSMITS           5  // Maximum amount of retransmits of a message
#define LISPD_MIN_RETRANSMIT_INTERVAL   1  // Minimum time between retransmits of control messages


#define DEFAULT_MAP_REQUEST_RETRIES             3

#define MAP_REGISTER_INTERVAL                   60
#define MS_SITE_EXPIRATION                      180

#define RLOC_PROBING_INTERVAL                   30
#define DEFAULT_RLOC_PROBING_RETRIES            2
#define DEFAULT_RLOC_PROBING_RETRIES_INTERVAL   5   /* Interval in seconds between RLOC probing retries  */

#define DEFAULT_DATA_CACHE_TTL                  10
#define DEFAULT_SELECT_TIMEOUT                  1000/* ms */

#define FIELD_AFI_LEN                    2
#define FIELD_PORT_LEN                   2

/*
 *  lispd constants
 */

#define EVER            ;;
#define LISPD_VERSION   "v0.5.2"
#define LISPD           "lispd"
#define PID_FILE        "/var/run/lispd.pid"

/*
 *  misc parameters
 */

#define IP6VERSION      6   /* what's the symbol? */
#define PACKED          __attribute__ ((__packed__))
#define uchar           u_char

#define GOOD                1
#define BAD                 0
#define ERR_SRC_ADDR        -1
#define ERR_AFI             -2
#define ERR_DB              -3
#define ERR_MALLOC          -4
#define ERR_EXIST           -5
#define ERR_NO_EXIST        -6
#define ERR_CTR_IFACE       -7
#define ERR_NOT_LISP        -8
#define ERR_SOCKET          -9

#define TRUE                1
#define FALSE               0
#define UP                  1
#define DOWN                0
#define UNKNOWN            -1

/***** NAT status *******/
//#define UNKNOWN          -1
#define NO_NAT              0
#define PARTIAL_NAT         1
#define FULL_NAT            2


/*
 * Structure to simplify netlink processing
 */
typedef struct nlsock_handle
{
    int         fd;       // netlink socket fd
    uint32_t    seq;      // netlink message seq number
} nlsock_handle;




/* Use the nonce to calculate the source port for a map request
 * message. */
#define LISP_PKT_MAP_REQUEST_UDP_SPORT(Nonce) (0xf000 | (Nonce & 0xfff))
#define LISP_PKT_MAP_REQUEST_TTL 32
#define LISP_PKT_MAP_REQUEST_MAX_ITR_RLOCS 31

#endif /* DEFS_H_ */
