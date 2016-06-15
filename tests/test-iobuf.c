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
	struct iobuf *buf = iobuf_new();
	TEST_ASSERT(iobuf_rstart(buf) == NULL);
	TEST_ASSERT(iobuf_rsize(buf) == 0);
	TEST_ASSERT(iobuf_wstart(buf) == NULL);
	TEST_ASSERT(iobuf_wsize(buf) == 0);

	iobuf_drain(buf, 0);
	TEST_ASSERT(iobuf_rstart(buf) == NULL);
	TEST_ASSERT(iobuf_rsize(buf) == 0);
	TEST_ASSERT(iobuf_wstart(buf) == NULL);
	TEST_ASSERT(iobuf_wsize(buf) == 0);

	iobuf_reserve(buf, 0);
	TEST_ASSERT(iobuf_rstart(buf) == NULL);
	TEST_ASSERT(iobuf_rsize(buf) == 0);
	TEST_ASSERT(iobuf_wstart(buf) == NULL);
	TEST_ASSERT(iobuf_wsize(buf) == 0);

	iobuf_fill(buf, 0);
	TEST_ASSERT(iobuf_rstart(buf) == NULL);
	TEST_ASSERT(iobuf_rsize(buf) == 0);
	TEST_ASSERT(iobuf_wstart(buf) == NULL);
	TEST_ASSERT(iobuf_wsize(buf) == 0);

	iobuf_free(buf);
}

static void test_reserved()
{
	struct iobuf *buf = iobuf_new();
	iobuf_reserve(buf, 4096);
	TEST_ASSERT(iobuf_wstart(buf) != NULL);
	TEST_ASSERT(iobuf_wsize(buf) == 4096);
	iobuf_free(buf);
}

static void test_one_byte()
{
	struct iobuf *buf = iobuf_new();

	iobuf_reserve(buf, 1);
	TEST_ASSERT(iobuf_rstart(buf) != NULL);
	TEST_ASSERT(iobuf_rsize(buf) == 0);
	TEST_ASSERT(iobuf_wstart(buf) != NULL);
	TEST_ASSERT(iobuf_wsize(buf) == 1);

	*((char *)iobuf_wstart(buf)) = 42;
	iobuf_fill(buf, 1);
	TEST_ASSERT(iobuf_rstart(buf) != NULL);
	TEST_ASSERT(iobuf_rsize(buf) == 1);
	TEST_ASSERT(iobuf_wstart(buf) != NULL);
	TEST_ASSERT(iobuf_wsize(buf) == 0);

	TEST_ASSERT(*((char *)iobuf_rstart(buf)) == 42);
	iobuf_drain(buf, 1);

	TEST_ASSERT(iobuf_rstart(buf) != NULL);
	TEST_ASSERT(iobuf_rsize(buf) == 0);
	TEST_ASSERT(iobuf_wstart(buf) != NULL);
	TEST_ASSERT(iobuf_wsize(buf) == 1);

	iobuf_free(buf);
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
	struct iobuf *buf = iobuf_new1(4096);
	TEST_ASSERT(iobuf_rstart(buf) != NULL);
	TEST_ASSERT(iobuf_rsize(buf) == 0);
	TEST_ASSERT(iobuf_wstart(buf) != NULL);
	TEST_ASSERT(iobuf_wsize(buf) == 4096);

	size_t size = iobuf_wsize(buf);
	memset(iobuf_wstart(buf), 42, size);
	iobuf_fill(buf, size);

	TEST_ASSERT(iobuf_rstart(buf) != NULL);
	TEST_ASSERT(iobuf_rsize(buf) == 4096);
	TEST_ASSERT(iobuf_wstart(buf) != NULL);
	TEST_ASSERT(iobuf_wsize(buf) == 0);

	char *rbuf = ((char *)iobuf_rstart(buf));
	for (size_t i = 0; i < 4096; ++i) {
		TEST_ASSERT(rbuf[i] == 42);
		TEST_ASSERT(iobuf_rsize(buf) >= 1);
		iobuf_drain(buf, 1);
	}

	TEST_ASSERT(iobuf_rsize(buf) == 0);
	TEST_ASSERT(iobuf_wsize(buf) == 4096);

	iobuf_free(buf);
}

static void test_grow()
{
	struct iobuf *buf = iobuf_new();

	for (size_t i = 0; i < 4096; ++i) {
		iobuf_reserve(buf, i);
		TEST_ASSERT(iobuf_wsize(buf) == i);
	}

	for (size_t i = 4096; i != 0; --i) {
		iobuf_reserve(buf, i);
		TEST_ASSERT(iobuf_wsize(buf) == 4096);
	}

	iobuf_free(buf);
}

static void test_fill_drain()
{
	struct iobuf *buf = iobuf_new();

	iobuf_reserve(buf, 4096);
	TEST_ASSERT(iobuf_wsize(buf) == 4096);

	iobuf_fill(buf, 2048);
	TEST_ASSERT(iobuf_wsize(buf) == 2048);
	TEST_ASSERT(iobuf_rsize(buf) == 2048);
	iobuf_reserve(buf, 4096);
	TEST_ASSERT(iobuf_wsize(buf) == 4096);
	TEST_ASSERT(iobuf_rsize(buf) == 2048);
	iobuf_fill(buf, 2048);
	TEST_ASSERT(iobuf_wsize(buf) == 2048);
	TEST_ASSERT(iobuf_rsize(buf) == 4096);
	iobuf_drain(buf, 2048);
	TEST_ASSERT_EQUAL(2048, iobuf_wsize(buf));
	TEST_ASSERT(iobuf_rsize(buf) == 2048);
	iobuf_drain(buf, 2048);
	TEST_ASSERT_EQUAL(4096 + 2048, iobuf_wsize(buf));

	iobuf_free(buf);
}

static void test_reclaim()
{
	struct iobuf *buf = iobuf_new();

	iobuf_reserve(buf, 4096);
	TEST_ASSERT(iobuf_wsize(buf) == 4096);
	memset(iobuf_wstart(buf), 1, 2048);
	iobuf_fill(buf, 2048);
	TEST_ASSERT(iobuf_wsize(buf) == 2048);
	TEST_ASSERT(iobuf_rsize(buf) == 2048);
	TEST_ASSERT(memneq(iobuf_rstart(buf), 1, iobuf_rsize(buf)));
	iobuf_drain(buf, 1024);
	TEST_ASSERT_EQUAL(2048, iobuf_wsize(buf));
	TEST_ASSERT_EQUAL(1024, iobuf_rsize(buf));
	iobuf_reserve(buf, 4096);
	TEST_ASSERT_EQUAL(4096, iobuf_wsize(buf));
	TEST_ASSERT_EQUAL(1024, iobuf_rsize(buf));
	TEST_ASSERT(memneq(iobuf_rstart(buf), 1, iobuf_rsize(buf)));

	iobuf_free(buf);
}

static void test_reclaim_full()
{
	struct iobuf *buf = iobuf_new();
	iobuf_reserve(buf, 4096);
	TEST_ASSERT(iobuf_wsize(buf) == 4096);
	memset(iobuf_wstart(buf), 1, 4096);
	iobuf_fill(buf, 4096);
	TEST_ASSERT(iobuf_wsize(buf) == 0);
	TEST_ASSERT(iobuf_rsize(buf) == 4096);
	TEST_ASSERT(memneq(iobuf_rstart(buf), 1, iobuf_rsize(buf)));
	iobuf_drain(buf, 2048);
	TEST_ASSERT(iobuf_wsize(buf) == 0);
	TEST_ASSERT(iobuf_rsize(buf) == 2048);
	iobuf_reserve(buf, 2048);
	TEST_ASSERT(iobuf_wsize(buf) == 2048);
	TEST_ASSERT(iobuf_rsize(buf) == 2048);
	TEST_ASSERT(memneq(iobuf_rstart(buf), 1, iobuf_rsize(buf)));
	iobuf_free(buf);
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
	struct iobuf *buf = iobuf_new();
	for (;;) {
		iobuf_reserve(buf, 1024);
		TEST_ASSERT(iobuf_wsize(buf) >= 1024);
		ssize_t ret = fake_read(&fd, iobuf_wstart(buf), 1024);
		if (ret == 0) {
			break;
		}

		TEST_ASSERT(ret);
		read += ret;
	}

	TEST_ASSERT_EQUAL(sizeof(fd.in), read);

	iobuf_free(buf);
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
