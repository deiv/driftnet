/*
 * web_data.h:
 * static content files.
 *
 * Copyright (c) 2012-2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __WEB_DATA_H_
#define __WEB_DATA_H_

typedef struct web_static_file_t {
    char* name;
    unsigned char *data;
    unsigned int size;
} web_static_file_t;

extern web_static_file_t static_content[];

#endif /* __WEB_DATA_H_ */