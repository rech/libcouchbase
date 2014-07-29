/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2010-2012 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

/**
 * Basic platform includes.
 */
#ifndef LCB_SYSDEFS_H
#define LCB_SYSDEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
    #include <stddef.h>
    #include <winsock2.h>
    #include <basetsd.h>

    typedef __int64 lcb_int64_t;
    typedef __int32 lcb_int32_t;
    typedef SIZE_T lcb_size_t;
    typedef SSIZE_T lcb_ssize_t;
    typedef unsigned __int8 lcb_uint8_t;
    typedef unsigned __int16 lcb_vbucket_t;
    typedef unsigned __int16 lcb_uint16_t;
    typedef unsigned __int32 lcb_uint32_t;
    typedef unsigned __int64 lcb_cas_t;
    typedef unsigned __int64 lcb_uint64_t;

    /** FIXME: This should be a native type, but it's already defined here.. */
    typedef unsigned __int32 lcb_time_t;
#else
    #include <sys/types.h>
    #include <stdint.h>
    #include <time.h>

    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #include <stddef.h>
    #endif

    typedef int64_t lcb_int64_t;
    typedef int32_t lcb_int32_t;
    typedef size_t lcb_size_t;
    typedef ssize_t lcb_ssize_t;
    typedef uint16_t lcb_vbucket_t;
    typedef uint8_t lcb_uint8_t;
    typedef uint16_t lcb_uint16_t;
    typedef uint32_t lcb_uint32_t;
    typedef uint64_t lcb_cas_t;
    typedef uint64_t lcb_uint64_t;
    typedef time_t lcb_time_t;
#endif

#ifdef __cplusplus
}
#endif

#endif /* LCB_SYSDEFS_H */
