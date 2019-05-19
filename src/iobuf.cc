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

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <iobuf.h>


void iobuf::fill(size_t size)
{
	assert(wstart_ + size <= capacity_);
	wstart_ += size;
}

size_t iobuf::rsize() const
{
	return wstart_ - rstart_;
}

void *iobuf::rstart() const
{
	return static_cast<char *>(data_) + rstart_;
}

size_t iobuf::wsize() const
{
	return capacity_ - wstart_;
}

void *iobuf::wstart() const
{
	return static_cast<char *>(data_) + wstart_;
}

void iobuf::drain(size_t size)
{
	assert(size <= rsize());

	rstart_ += size;
	if (rstart_ == wstart_) {
		wstart_ = 0;
		rstart_ = 0;
	}
}

iobuf::iobuf()
:
	data_(nullptr),
	capacity_(0),
	rstart_(0),
	wstart_(0)
{ }

iobuf::iobuf(size_t size)
:
	data_(nullptr),
	capacity_(0),
	rstart_(0),
	wstart_(0)
{
	grow(size);
}

iobuf::~iobuf()
{
	free(data_);
}

void iobuf::reserve(size_t size)
{
	if (size == 0) {
		return;
	}

	if (size <= wsize()) {
		return;
	}

	if (rstart_ != 0) {
		reclaim();
	}

	if (size <= wsize()) {
		return;
	}

	grow(capacity_ + (size - wsize()));
}

void iobuf::grow(size_t capacity)
{
	assert(capacity > capacity_);

	void *n(realloc(data_, capacity));
	if (n != nullptr) {
		data_ = n;
		capacity_ = capacity;
	}
}

void iobuf::reclaim()
{
	size_t srsize(rsize());
	memmove(data_, rstart(), srsize);
	rstart_ = 0;
	wstart_ = srsize;
}
