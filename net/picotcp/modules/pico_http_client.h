/*********************************************************************
   PicoTCP. Copyright (c) 2012 TASS Belgium NV. Some rights reserved.
   See LICENSE and COPYING for usage.

   Authors:
   - Daniele Lacamera <daniele.lacamera@altran.com>
   - Robbin Van Damme <robbin.vandamme@altran.com>
   - Maxime Vincent <maxime.vincent@altran.com>
   - Andrei Carp <andrei.carp@tass.be>
 *********************************************************************/


#ifndef PICO_HTTP_CLIENT_H_
#define PICO_HTTP_CLIENT_H_

#include "pico_http_util.h"

/*
 * Transfer encodings
 */
#define HTTP_TRANSFER_CHUNKED  1u
#define HTTP_TRANSFER_FULL     0u

/*
 * Parameters for the send request functions
 */
#define HTTP_CONN_CLOSE         0u
#define HTTP_CONN_KEEP_ALIVE    1u

/*
 * Data types
 */

struct pico_http_header
{
    uint16_t response_code;                  /* http response */
    char *location;                          /* if redirect is reported */
    uint32_t content_length_or_chunk;        /* size of the message */
    uint8_t transfer_coding;                 /* chunked or full */
};

struct pico_http_client;

struct multipart_chunk
{
    unsigned char *data;
    uint32_t length_data;
    char *name;
    uint16_t length_name;
    char *filename;
    uint16_t length_filename;
    char *content_disposition;
    uint16_t length_content_disposition;
    char *content_type;
    uint16_t length_content_type;
};

struct multipart_chunk *multipart_chunk_create(unsigned char *data, uint64_t length_data, char *name, char *filename, char *content_disposition, char *content_type);
int8_t multipart_chunk_destroy(struct multipart_chunk *chunk);
int8_t pico_http_client_get_write_progress(uint16_t conn, uint32_t *total_bytes_written, uint32_t *total_bytes_to_write);
int32_t pico_http_client_open_with_usr_pwd_encoding(char *uri, void (*wakeup)(uint16_t ev, uint16_t conn), int (*encoding)(char *out_buffer, char *in_buffer));
int32_t pico_http_client_open(char *uri, void (*wakeup)(uint16_t ev, uint16_t conn));
int8_t pico_http_client_send_raw(uint16_t conn, char *resource);
int8_t pico_http_client_send_get(uint16_t conn, char *resource, uint8_t connection_type);
int8_t pico_http_client_long_poll_send_get(uint16_t conn, char *resource, uint8_t connection_type);
int8_t pico_http_client_long_poll_cancel(uint16_t conn);
int8_t pico_http_client_send_post(uint16_t conn, char *resource, uint8_t *post_data, uint32_t post_data_len, uint8_t connection_type, char *content_type, char *cache_control);
int8_t pico_http_client_send_delete(uint16_t conn, char *resource, uint8_t connection_type);
int8_t pico_http_client_send_post_multipart(uint16_t conn, char *resource, struct multipart_chunk **post_data, uint16_t post_data_len, uint8_t connection_type);

struct pico_http_header *pico_http_client_read_header(uint16_t conn);
struct pico_http_uri *pico_http_client_read_uri_data(uint16_t conn);

int32_t pico_http_client_read_body(uint16_t conn, unsigned char *data, uint16_t size, uint8_t *body_read_done);
int8_t pico_http_client_close(uint16_t conn);

int8_t pico_http_set_close_ev(uint16_t conn);

#endif /* PICO_HTTP_CLIENT_H_ */
