/*
 * test_unit.c:
 * Test unit for media library.
 *
 * Copyright (c) 2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>

#include <fcntl.h> /* for O_CREAT, O_EXCL, O_WRONLY */
#include <stdlib.h>
#include <unistd.h>

#include <sys/wait.h>

#include "media/image.h"

char* jpeg_image_list[] = {
        "tests/resources/jpg_test_file_1.jpg",
        "tests/resources/jpg_test_file_2.jpg",
        "tests/resources/jpg_test_file_3.jpg"
};

char* ico_image_list[] = {
        "tests/resources/ico_test_file_1.ico"
};

void test_no_error_on_null_data()
{
    unsigned char *jpegdata = NULL;
    size_t jpeglen = 0;

    find_jpeg_image(NULL, 0, &jpegdata, &jpeglen);

    assert_null(jpegdata);
    assert_int_equal(0, jpeglen);
}

unsigned char *find_jpeg(char* file_path, unsigned char **jpegdata, size_t *jpeglen)
{
    int fd1 = 0;
    int file_len;

    fd1 = open(file_path, O_RDONLY, 0666);

    if (fd1 == -1) {
        fail();
    }

    file_len = lseek(fd1, 0, SEEK_END);

    if (file_len <= 0) {
        fail();
    }

    lseek(fd1, 0, SEEK_SET);

    unsigned char* image_data = (unsigned char *) malloc(file_len);

    if (image_data == NULL) {
        fail();
    }

    if(read(fd1, image_data, file_len) < 0)
        fail();

    close(fd1);

    return find_jpeg_image(image_data, file_len, jpegdata, jpeglen);
}

void test_parse_images()
{
    unsigned char *jpegdata = NULL;
    size_t jpeglen = 0;
    unsigned char *ret;

    for (int idx = 0; idx < 3; idx++) {

        ret = find_jpeg(jpeg_image_list[idx], &jpegdata, &jpeglen);

        assert_non_null(ret);
        assert_non_null(jpegdata);
        assert_int_equal(ret - jpegdata, jpeglen);

    }
}

void test_dont_parse_corrupt_data()
{
    unsigned char *jpegdata = NULL;
    size_t jpeglen = 0;
    const unsigned char *data = (unsigned char*)"\xff\xd8\x01\x03\x02\x00";
    const size_t len = 6;
    unsigned char *ret;

    ret = find_jpeg_image(data, len, &jpegdata, &jpeglen);
    ret = find_jpeg_image(ret, len - 2, &jpegdata, &jpeglen);

    assert_non_null(ret);
    assert_null(jpegdata);
    assert_int_equal(0, jpeglen);
}

void test_dont_parse_other_formats()
{
    unsigned char *jpegdata = NULL;
    size_t jpeglen = 0;
    unsigned char *ret;

    for (int idx = 0; idx <1; idx++) {

        ret = find_jpeg(ico_image_list[idx], &jpegdata, &jpeglen);

        assert_non_null(ret);
        assert_null(jpegdata);
        assert_int_equal(0, jpeglen);

    }
}

int main(void)
{
    const struct CMUnitTest tests[] = {
            cmocka_unit_test(test_no_error_on_null_data),
            cmocka_unit_test(test_parse_images),
            cmocka_unit_test(test_dont_parse_corrupt_data),
            cmocka_unit_test(test_dont_parse_other_formats)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}