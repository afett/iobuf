/*
   Copyright (c) 2015, Andreas Fett
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <iobuf.h>

static void grow(struct iobuf *, size_t);
static void reclaim(struct iobuf *);

void iobuf_fill(struct iobuf *self, size_t size)
{
	assert(self);
	assert(self->wstart + size <= self->capacity);
	self->wstart += size;
}

size_t iobuf_rsize(struct iobuf *self)
{
	assert(self);
	return self->wstart - self->rstart;
}

void *iobuf_rstart(struct iobuf *self)
{
	assert(self);
	return (char *)self->data + self->rstart;
}

size_t iobuf_wsize(struct iobuf *self)
{
	assert(self);
	return self->capacity - self->wstart;
}

void *iobuf_wstart(struct iobuf *self)
{
	assert(self);
	return (char *)self->data + self->wstart;
}

void iobuf_drain(struct iobuf *self, size_t size)
{
	assert(self);
	assert(size <= iobuf_rsize(self));

	self->rstart += size;
	if (self->rstart == self->wstart) {
		self->wstart = 0;
		self->rstart = 0;
	}
}

struct iobuf *iobuf_new()
{
	return static_cast<iobuf*>(calloc(1, sizeof(struct iobuf)));
}

struct iobuf *iobuf_new1(size_t size)
{
	struct iobuf *self = static_cast<iobuf*>(calloc(1, sizeof(struct iobuf)));
	if (self) {
		grow(self, size);
	}
	return self;
}

void iobuf_free(struct iobuf *self)
{
	assert(self);

	free(self->data);
	free(self);
}

void iobuf_reserve(struct iobuf *self, size_t size)
{
	if (size == 0) {
		return;
	}

	assert(self);
	if (size <= iobuf_wsize(self)) {
		return;
	}

	if (self->rstart != 0) {
		reclaim(self);
	}

	if (size <= iobuf_wsize(self)) {
		return;
	}

	grow(self, self->capacity + (size - iobuf_wsize(self)));
}

static void grow(struct iobuf *self, size_t capacity)
{
	assert(self);
	assert(capacity > self->capacity);

	void *n = realloc(self->data, capacity);
	if (n != NULL) {
		self->data = n;
		self->capacity = capacity;
	}
}

static void reclaim(struct iobuf *self)
{
	assert(self);

	size_t rsize = iobuf_rsize(self);
	memmove(self->data, iobuf_rstart(self), rsize);
	self->rstart = 0;
	self->wstart = rsize;
}
