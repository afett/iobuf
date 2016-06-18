#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <iobuf.h>

#define TEST_ASSERT(expr) \
	assert(expr);

#define TEST_ASSERT_EQUAL(exp, res) \
	if (!((res) == (exp))) {    \
		fprintf(stderr, # exp " != " # res ", got:%zu\n", (res)); \
		abort();            \
	}                           \

#define RUN_TEST(test)                  \
{                                       \
	printf("running %s\n", # test); \
	test();                         \
}

static void test_empty()
{
	iobuf buf;
	TEST_ASSERT(buf.rstart() == NULL);
	TEST_ASSERT(buf.rsize() == 0);
	TEST_ASSERT(buf.wstart() == NULL);
	TEST_ASSERT(buf.wsize() == 0);

	buf.drain(0);
	TEST_ASSERT(buf.rstart() == NULL);
	TEST_ASSERT(buf.rsize() == 0);
	TEST_ASSERT(buf.wstart() == NULL);
	TEST_ASSERT(buf.wsize() == 0);

	buf.reserve(0);
	TEST_ASSERT(buf.rstart() == NULL);
	TEST_ASSERT(buf.rsize() == 0);
	TEST_ASSERT(buf.wstart() == NULL);
	TEST_ASSERT(buf.wsize() == 0);

	buf.fill(0);
	TEST_ASSERT(buf.rstart() == NULL);
	TEST_ASSERT(buf.rsize() == 0);
	TEST_ASSERT(buf.wstart() == NULL);
	TEST_ASSERT(buf.wsize() == 0);
}

static void test_reserved()
{
	iobuf buf;
	buf.reserve(4096);
	TEST_ASSERT(buf.wstart() != NULL);
	TEST_ASSERT(buf.wsize() == 4096);
}

static void test_one_byte()
{
	iobuf buf;

	buf.reserve(1);
	TEST_ASSERT(buf.rstart() != NULL);
	TEST_ASSERT(buf.rsize() == 0);
	TEST_ASSERT(buf.wstart() != NULL);
	TEST_ASSERT(buf.wsize() == 1);

	*((char *)buf.wstart()) = 42;
	buf.fill(1);
	TEST_ASSERT(buf.rstart() != NULL);
	TEST_ASSERT(buf.rsize() == 1);
	TEST_ASSERT(buf.wstart() != NULL);
	TEST_ASSERT(buf.wsize() == 0);

	TEST_ASSERT(*((char *)buf.rstart()) == 42);
	buf.drain(1);

	TEST_ASSERT(buf.rstart() != NULL);
	TEST_ASSERT(buf.rsize() == 0);
	TEST_ASSERT(buf.wstart() != NULL);
	TEST_ASSERT(buf.wsize() == 1);
}

static bool memneq(void *start, int value, size_t len)
{
	for (size_t i = 0; i < len; ++i) {
		if (((char*)start)[i] != value) {
			return false;
		}
	}

	return true;
}

static void test_preallocated()
{
	iobuf buf(4096);
	TEST_ASSERT(buf.rstart() != NULL);
	TEST_ASSERT(buf.rsize() == 0);
	TEST_ASSERT(buf.wstart() != NULL);
	TEST_ASSERT(buf.wsize() == 4096);

	size_t size = buf.wsize();
	memset(buf.wstart(), 42, size);
	buf.fill(size);

	TEST_ASSERT(buf.rstart() != NULL);
	TEST_ASSERT(buf.rsize() == 4096);
	TEST_ASSERT(buf.wstart() != NULL);
	TEST_ASSERT(buf.wsize() == 0);

	char *rbuf = ((char *)buf.rstart());
	for (size_t i = 0; i < 4096; ++i) {
		TEST_ASSERT(rbuf[i] == 42);
		TEST_ASSERT(buf.rsize() >= 1);
		buf.drain(1);
	}

	TEST_ASSERT(buf.rsize() == 0);
	TEST_ASSERT(buf.wsize() == 4096);
}

static void test_grow()
{
	iobuf buf;

	for (size_t i = 0; i < 4096; ++i) {
		buf.reserve(i);
		TEST_ASSERT(buf.wsize() == i);
	}

	for (size_t i = 4096; i != 0; --i) {
		buf.reserve(i);
		TEST_ASSERT(buf.wsize() == 4096);
	}
}

static void test_fill_drain()
{
	iobuf buf;

	buf.reserve(4096);
	TEST_ASSERT(buf.wsize() == 4096);

	buf.fill(2048);
	TEST_ASSERT(buf.wsize() == 2048);
	TEST_ASSERT(buf.rsize() == 2048);
	buf.reserve(4096);
	TEST_ASSERT(buf.wsize() == 4096);
	TEST_ASSERT(buf.rsize() == 2048);
	buf.fill(2048);
	TEST_ASSERT(buf.wsize() == 2048);
	TEST_ASSERT(buf.rsize() == 4096);
	buf.drain(2048);
	TEST_ASSERT_EQUAL(2048, buf.wsize());
	TEST_ASSERT(buf.rsize() == 2048);
	buf.drain(2048);
	TEST_ASSERT_EQUAL(4096 + 2048, buf.wsize());
}

static void test_reclaim()
{
	iobuf buf;

	buf.reserve(4096);
	TEST_ASSERT(buf.wsize() == 4096);
	memset(buf.wstart(), 1, 2048);
	buf.fill(2048);
	TEST_ASSERT(buf.wsize() == 2048);
	TEST_ASSERT(buf.rsize() == 2048);
	TEST_ASSERT(memneq(buf.rstart(), 1, buf.rsize()));
	buf.drain(1024);
	TEST_ASSERT_EQUAL(2048, buf.wsize());
	TEST_ASSERT_EQUAL(1024, buf.rsize());
	buf.reserve(4096);
	TEST_ASSERT_EQUAL(4096, buf.wsize());
	TEST_ASSERT_EQUAL(1024, buf.rsize());
	TEST_ASSERT(memneq(buf.rstart(), 1, buf.rsize()));
}

static void test_reclaim_full()
{
	iobuf buf;
	buf.reserve(4096);
	TEST_ASSERT(buf.wsize() == 4096);
	memset(buf.wstart(), 1, 4096);
	buf.fill(4096);
	TEST_ASSERT(buf.wsize() == 0);
	TEST_ASSERT(buf.rsize() == 4096);
	TEST_ASSERT(memneq(buf.rstart(), 1, buf.rsize()));
	buf.drain(2048);
	TEST_ASSERT(buf.wsize() == 0);
	TEST_ASSERT(buf.rsize() == 2048);
	buf.reserve(2048);
	TEST_ASSERT(buf.wsize() == 2048);
	TEST_ASSERT(buf.rsize() == 2048);
	TEST_ASSERT(memneq(buf.rstart(), 1, buf.rsize()));
}

struct fake_fd {
	size_t read;
	char in[65536];
};

static void fake_fd_init(struct fake_fd *src)
{
	srandom(time(NULL));

	for (size_t i = 0; i < sizeof(src->in); ++i) {
		src->in[i] = (rand() % 255) + 1;
	}

	for (size_t i = 0; i < 1000; ++i) {
		size_t pos = 0;
		do {
			pos = rand() % sizeof(src->in);
		} while (src->in[pos] == 0);
		src->in[pos] = 0;
	}

	src->read = 0;
}

static ssize_t fake_read(struct fake_fd *src, void *buf, size_t count)
{
	if (src->read == sizeof(src->in)) {
		return 0;
	}

	size_t ret = (rand() % count) + 1;
	if (src->read + ret > sizeof(src->in)) {
		ret = sizeof(src->in) - src->read;
	}

	memcpy(buf, &src->in[src->read], ret);
	src->read += ret;
	return ret;
}

static void test_random_access()
{
	struct fake_fd fd;
	fake_fd_init(&fd);
	char out[sizeof(fd.in)];
	memset(out, 0, sizeof(out));

	size_t read = 0;
	iobuf buf;
	for (;;) {
		buf.reserve(1024);
		TEST_ASSERT(buf.wsize() >= 1024);
		ssize_t ret = fake_read(&fd, buf.wstart(), 1024);
		if (ret == 0) {
			break;
		}

		TEST_ASSERT(ret);
		read += ret;
	}

	TEST_ASSERT_EQUAL(sizeof(fd.in), read);
}

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	RUN_TEST(test_empty);
	RUN_TEST(test_reserved);
	RUN_TEST(test_one_byte);
	RUN_TEST(test_preallocated);
	RUN_TEST(test_grow);
	RUN_TEST(test_fill_drain);
	RUN_TEST(test_reclaim);
	RUN_TEST(test_reclaim_full);
	RUN_TEST(test_random_access);

	return EXIT_SUCCESS;
}
