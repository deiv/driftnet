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
#include "media/media.h"

char* gif_image_list[] = {
        "tests/resources/gif_test_file_1.gif",
        "tests/resources/gif_test_file_2.gif",
        "tests/resources/gif_test_file_3.gif"
};

char* jpeg_image_list[] = {
        "tests/resources/jpg_test_file_1.jpg",
        "tests/resources/jpg_test_file_2.jpg",
        "tests/resources/jpg_test_file_3.jpg"
};

char* png_image_list[] = {
        "tests/resources/png_test_file_1.png",
        "tests/resources/png_test_file_2.png",
        "tests/resources/png_test_file_3.png"
};

char* ico_image_list[] = {
        "tests/resources/ico_test_file_1.ico"
};

typedef struct test_media_state_t {

    char** image_list;
    unsigned char *(*find_media_func)(const unsigned char *data, const size_t len, unsigned char **found, size_t *foundlen);

} test_media_state_t;

test_media_state_t gif_test_media_resource  = {.image_list = gif_image_list,  .find_media_func = find_gif_image};
test_media_state_t jpeg_test_media_resource = {.image_list = jpeg_image_list, .find_media_func = find_jpeg_image};
test_media_state_t png_test_media_resource  = {.image_list = png_image_list,  .find_media_func = find_png_image};


/**
 * Setup the needed state for gif media group tests
 *
 * @param state
 * @return
 */
static int gif_media_test_group_setup (void** state)
{
    *state = &gif_test_media_resource;

    return 0;
}

/**
 * Setup the needed state for jpeg media group tests
 *
 * @param state
 * @return
 */
static int jpeg_media_test_group_setup (void** state)
{
    *state = &jpeg_test_media_resource;

    return 0;
}

/**
 * Setup the needed state for png media group tests
 *
 * @param state
 * @return
 */
static int png_media_test_group_setup (void** state)
{
    *state = &png_test_media_resource;

    return 0;
}

void test_no_error_on_null_data(void** state)
{
    test_media_state_t* teststate = *state;
    unsigned char *media_data = NULL;
    size_t media_len = 0;

    for (int idx = 0; idx < 3; idx++) {

        teststate->find_media_func(NULL, 0, &media_data, &media_len);

        assert_null(media_data);
        assert_int_equal(0, media_len);
    }
}

unsigned char *find_media(char *file_path, unsigned char **jpegdata, size_t *jpeglen,
        unsigned char *(*find_media_func)(const unsigned char *, const size_t, unsigned char **, size_t *))
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

    return find_media_func(image_data, file_len, jpegdata, jpeglen);
}

void test_parse_images(void** state)
{
    test_media_state_t* teststate = *state;
    unsigned char *jpegdata = NULL;
    size_t jpeglen = 0;
    unsigned char *ret;

    for (int idx = 0; idx < 3; idx++) {

        for (int file_idx = 0; file_idx < 3; file_idx++) {

            ret = find_media(teststate->image_list[file_idx], &jpegdata, &jpeglen, teststate->find_media_func);

            assert_non_null(ret);
            assert_non_null(jpegdata);
            assert_int_equal(ret - jpegdata, jpeglen);

        }
    }


}

void test_dont_parse_corrupt_data(void** state)
{
    test_media_state_t* teststate = *state;
    unsigned char *jpegdata = NULL;
    size_t jpeglen = 0;
    const unsigned char *data = (unsigned char*)"\xff\xd8\x01\x03\x02\x00";
    const size_t len = 6;
    unsigned char *ret;

    ret = teststate->find_media_func(data, len, &jpegdata, &jpeglen);
    ret = teststate->find_media_func(ret, len - 2, &jpegdata, &jpeglen);

    assert_non_null(ret);
    assert_null(jpegdata);
    assert_int_equal(0, jpeglen);
}

void test_dont_parse_other_formats(void** state)
{
    test_media_state_t* teststate = *state;
    unsigned char *jpegdata = NULL;
    size_t jpeglen = 0;
    unsigned char *ret;

    for (int file_idx = 0; file_idx < 1; file_idx++) {

        ret = find_media(ico_image_list[file_idx], &jpegdata, &jpeglen, teststate->find_media_func);

        assert_non_null(ret);
        /* XXX: */
        //assert_null(jpegdata);
        //assert_int_equal(0, jpeglen);
    }
}

void test_correct_media_drivers_for_mediatype_count()
{
    drivers_t* image_drivers = NULL;
    drivers_t* audio_drivers = NULL;
    drivers_t* text_drivers = NULL;

    /* test image drivers count */
    image_drivers = get_drivers_for_mediatype(MEDIATYPE_IMAGE);

    assert_non_null(image_drivers);
    assert_int_equal(3, image_drivers->count);

    close_media_drivers(image_drivers);

    /* test audio drivers count */
    audio_drivers = get_drivers_for_mediatype(MEDIATYPE_AUDIO);

    assert_non_null(audio_drivers);
    assert_int_equal(1, audio_drivers->count);

    close_media_drivers(audio_drivers);

    /* test text drivers count */
    text_drivers = get_drivers_for_mediatype(MEDIATYPE_TEXT);

    assert_non_null(text_drivers);
    assert_int_equal(1, text_drivers->count);

    close_media_drivers(text_drivers);
}

int main(void)
{
    const struct CMUnitTest image_tests[] = {
            cmocka_unit_test(test_no_error_on_null_data),
            cmocka_unit_test(test_parse_images),
            cmocka_unit_test(test_dont_parse_corrupt_data),
            cmocka_unit_test(test_dont_parse_other_formats)
    };

    int ret = 0;

    ret += cmocka_run_group_tests_name("gif media tests", image_tests, gif_media_test_group_setup, NULL);
    ret += cmocka_run_group_tests_name("jpeg media tests", image_tests, jpeg_media_test_group_setup, NULL);
    ret += cmocka_run_group_tests_name("png media tests", image_tests, png_media_test_group_setup, NULL);

    const struct CMUnitTest media_tests[] = {
            cmocka_unit_test(test_correct_media_drivers_for_mediatype_count)
    };

    ret += cmocka_run_group_tests(media_tests, NULL, NULL);

    return ret;
}
