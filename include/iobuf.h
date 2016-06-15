/*
   Copyright (c) 2015, 2016, Andreas Fett
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IOBUF_H
#define IOBUF_H

#include <stddef.h>

/*

     <--------------- capacity -------------->
    +-----------------------------------------+
    | | | | | | | | | |D|D|D|D|D| | | | | | | |
    +^-----------------^---------^------------+
     |                 |         |
     |                 |         |
   data             rstart     wstart

*/

#ifdef __cplusplus
extern "C" {
#endif

struct iobuf {
	void *data;
	size_t capacity;
	size_t rstart;
	size_t wstart;
};

/* create a new iobuf */
struct iobuf *iobuf_new();

/* create a new iobuf with the given wsize*/
struct iobuf *iobuf_new1(size_t);

/* free iobuf and it's data */
void iobuf_free(struct iobuf *);

/* call reserve before writing at most iobuf_wsize() bytes
 * starting at iobuf_wstart()
 *
 * this will invalidate any results from previous calls to
 * rsize, rstart, wsize, wstart
 */
void iobuf_reserve(struct iobuf *, size_t);

/* call fill after writing at most iobuf_wsize() bytes
 * starting at iobuf_wstart()
 *
 * this will invalidate any results from previous calls to
 * rsize and wsize
 */
void iobuf_fill(struct iobuf *, size_t);

/* maximum size that may be read from rstart, and passed
 * to drain
 */
size_t iobuf_rsize(struct iobuf *);

/* start of data to read */
void *iobuf_rstart(struct iobuf *);

/* call drain after you read at most iobuf_rsize() bytes
 * starting at iobuf_rstart()
 *
 * this will invalidate any results from previous calls to
 * rsize and wsize
 */
void iobuf_drain(struct iobuf *, size_t);

/* maximum size that may be written to wstart */
size_t iobuf_wsize(struct iobuf *);

/* start of data to write */
void *iobuf_wstart(struct iobuf *);

#ifdef __cplusplus
}
#endif

#endif
