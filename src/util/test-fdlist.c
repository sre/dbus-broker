/*
 * Test Utility FD List
 */

#undef NDEBUG
#include <c-stdaux.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "util/fdlist.h"

static void test_setup(void) {
        _c_cleanup_(fdlist_freep) FDList *l = NULL;
        int r;

        r = fdlist_new_with_fds(&l, NULL, 0);
        assert(!r);

        l = fdlist_free(l);
        assert(!l);
}

static void test_dummy(void) {
        FDList *l;
        int r, dummies[] = { 7, 6, 5, 4, 3, 2, 1, 0 };
        size_t i, j;

        /*
         * Allocate FDList objects with sizes 0-8, with data taken from
         * @dummies. We verify that the file-descriptors are not touched (since
         * they are dummy values that do not correspond to an FD), and that the
         * fdlist returns the correct data.
         */

        for (i = 0; i < C_ARRAY_SIZE(dummies) + 1; ++i) {
                r = fdlist_new_with_fds(&l, dummies, i);
                assert(!r);

                assert(fdlist_count(l) == i);

                for (j = 0; j < i; ++j)
                        assert((size_t)fdlist_get(l, j) == C_ARRAY_SIZE(dummies) - j - 1);

                l = fdlist_free(l);
        }
}

static void test_consumer(void) {
        int r, prev, p[2];
        FDList *l;

        /*
         * Verify that the `consume' feature of FDList objects actually takes
         * posession of passed FDs and closes them on destruction. We use
         * epoll-fds as examples here, though any FD would work.
         *
         * We use the fact that FD spaces are sparse to verify that a given FD
         * was actually properly closed.
         */

        prev = epoll_create1(EPOLL_CLOEXEC);
        assert(prev >= 0);

        p[0] = epoll_create1(EPOLL_CLOEXEC);
        assert(p[0] >= 0);
        assert(p[0] == prev + 1);

        p[1] = epoll_create1(EPOLL_CLOEXEC);
        assert(p[1] >= 0);
        assert(p[1] == prev + 2);

        r = fdlist_new_consume_fds(&l, p, C_ARRAY_SIZE(p));
        assert(!r);

        assert(fdlist_count(l) == C_ARRAY_SIZE(p));
        assert(!memcmp(fdlist_data(l), p, sizeof(p)));

        fdlist_truncate(l, 0);

        r = epoll_create1(EPOLL_CLOEXEC);
        assert(r == prev + 1);
        close(r);

        close(prev);

        l = fdlist_free(l);
}

int main(int argc, char **argv) {
        test_setup();
        test_dummy();
        test_consumer();
        return 0;
}
