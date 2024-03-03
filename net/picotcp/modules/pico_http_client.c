/*********************************************************************
   PicoTCP. Copyright (c) 2012 TASS Belgium NV. Some rights reserved.
   See LICENSE and COPYING for usage.

   Authors:
   - Daniele Lacamera <daniele.lacamera@altran.com>
   - Robbin Van Damme <robbin.vandamme@altran.com>
   - Maxime Vincent <maxime.vincent@altran.com>
   - Andrei Carp <andrei.carp@tass.be>
 *********************************************************************/
#include <string.h>
#ifndef __BAREBOX__
#include <stdint.h>
#endif
#include <stdio.h>
#if defined __BAREBOX__
#include <linux/ctype.h>
#else
#include <ctype.h>
#endif

#include "pico_tree.h"
#include "pico_config.h"
#include "pico_socket.h"
#include "pico_tcp.h"
#include "pico_dns_client.h"
#include "pico_http_client.h"
#include "pico_http_util.h"
#include "pico_ipv4.h"
#include "pico_stack.h"

/*
 * This is the size of the following header
 *
 * GET <resource> HTTP/1.1<CRLF>
 * Host: <host>:<port><CRLF>
 * User-Agent: picoTCP<CRLF>
 * Connection: Close/Keep-Alive<CRLF>
 * <CRLF>
 *
 * where <resource>,<host> and <port> will be added later.
 */

#define HTTP_GET_BASIC_SIZE                 100u   //70u
#define HTTP_POST_BASIC_SIZE                256u
#define HTTP_POST_MULTIPART_BASIC_SIZE      80u
#define HTTP_POST_HEADER_BASIC_SIZE         160u
#define HTTP_DELETE_BASIC_SIZE              70u
#define HTTP_HEADER_LINE_SIZE               50u
#define HTTP_MAX_FIXED_POST_MULTIPART_CHUNK 100u
#define RESPONSE_INDEX                      9u

#define HTTP_CHUNK_ERROR    0xFFFFFFFFu

#ifdef dbg
    #undef dbg
#endif
#define dbg printf
//#define dbg(...) do {} while(0)

#define nop() do {} while(0)

#define consume_char(c)                          (pico_socket_read(client->sck, &c, 1u))
#define is_location(line)                        (memcmp(line, "Location", 8u) == 0)
#define is_content_length(line)           (memcmp(line, "Content-Length", 14u) == 0u)
#define is_transfer_encoding(line)        (memcmp(line, "Transfer-Encoding", 17u) == 0u)
#define is_chunked(line)                         (memcmp(line, " chunked", 8u) == 0u)
#define is_not_HTTPv1(line)                       (memcmp(line, "HTTP/1.", 7u))
#define is_hex_digit(x) ((('0' <= x) && (x <= '9')) || (('a' <= x) && (x <= 'f')))
#define hex_digit_to_dec(x) ((('0' <= x) && (x <= '9')) ? (x - '0') : ((('a' <= x) && (x <= 'f')) ? (x - 'a' + 10) : (-1)))

static uint16_t global_client_conn_ID = 0;

struct request_part
{
    char *buf;
    uint32_t buf_len;
    uint32_t buf_len_done;
    uint8_t copy;
    uint8_t mem;
};

struct pico_http_client
{
    uint16_t connectionID;
    uint8_t state;
    uint32_t body_read;
    uint8_t body_read_done;
    struct pico_socket *sck;
    void (*wakeup)(uint16_t ev, uint16_t conn);
    struct pico_ip4 ip;
    struct pico_http_uri *urikey;
    struct pico_http_header *header;
    struct request_part **request_parts;
    uint32_t request_parts_len_done;
    uint32_t request_parts_len;
    uint32_t request_parts_idx;
    uint8_t long_polling_state;
    uint8_t conn_state;
    uint8_t connection_type;
};

/* HTTP Client internal states */
#define HTTP_CONN_IDLE                  0
#define HTTP_START_READING_HEADER       1
#define HTTP_READING_HEADER             2
#define HTTP_READING_BODY               3
#define HTTP_READING_CHUNK_VALUE        4
#define HTTP_READING_CHUNK_TRAIL        5
#define HTTP_WRITING_REQUEST            6

/* HTTP Long Polling States */
#define HTTP_LONG_POLL_CONN_CLOSE       1
#define HTTP_LONG_POLL_CONN_KEEP_ALIVE  2
#define HTTP_LONG_POLL_CANCEL           3

/* HTTP Conn States */
#define HTTP_CONNECTION_CONNECTED               1
#define HTTP_CONNECTION_NOT_CONNECTED           2
#define HTTP_CONNECTION_WAITING_FOR_NEW_CONN    3


/* MISC */
#define HTTP_NO_COPY_TO_HEAP    0
#define HTTP_COPY_TO_HEAP       1
#define HTTP_USER_MEM           2
#define HTTP_NO_USER_MEM        3

/* HTTP URI string parsing */
#define HTTP_PROTO_TOK      "http://"
#define HTTP_PROTO_LEN      7u

static int8_t free_uri(struct pico_http_client *to_be_removed);
static int32_t client_open(char *uri, void (*wakeup)(uint16_t ev, uint16_t conn), int32_t connID, int (*encoding)(char *out_buffer, char *in_buffer));
static void free_header(struct pico_http_client *to_be_removed);

struct request_part *request_part_create(char *buf, uint32_t buf_len, uint8_t copy, uint8_t mem)
{
    dbg("request_part_create: Before pico_zalloc\n");
    struct request_part *part = PICO_ZALLOC(sizeof(struct request_part));
    dbg("request_part_create: After pico_zalloc\n");
    if (!part)
    {
        /* not enough memory */
        pico_err = PICO_ERR_ENOMEM;
        return NULL;
    }

    if (buf)
    {
        if (copy == HTTP_COPY_TO_HEAP)
        {
            part->buf = PICO_ZALLOC(buf_len);
            if (!part->buf)
            {
                PICO_FREE(part);
                pico_err = PICO_ERR_ENOMEM;
                return NULL;
            }
            memcpy(part->buf, buf, buf_len);
        }
        else
        {
            part->buf = buf;
        }
        part->copy = copy;
        part->buf_len = buf_len;
        part->buf_len_done = 0;
        part->mem = mem;
        return part;
    }
    else
    {
        pico_err = PICO_ERR_EINVAL;
        PICO_FREE(part);
        return NULL;
    }
}
/*
static void print_request_part_info(struct pico_http_client *client)
{
    uint32_t i = 0;
    dbg("request_parts_len: %d request_parts_len_done: %d\n", client->request_parts_len, client->request_parts_len_done);
    for (i=0; i<client->request_parts_len; i++)
    {
        dbg("i=%d buf_len: %d buf_len_done: %d buf: %p\n", i, client->request_parts[i]->buf_len, client->request_parts[i]->buf_len_done, client->request_parts[i]->buf);
    }
}
*/
//User memory buffers will not be freed
static int8_t request_parts_destroy(struct pico_http_client *client)
{
    uint32_t i = 0;
    if (!client)
    {
        return HTTP_RETURN_ERROR;
    }

    for (i=0; i<client->request_parts_len; i++)
    {
        if (client->request_parts[i]->copy == HTTP_COPY_TO_HEAP || client->request_parts[i]->mem == HTTP_NO_USER_MEM)
        {
            PICO_FREE(client->request_parts[i]->buf);
        }
        PICO_FREE(client->request_parts[i]);
    }
    PICO_FREE(client->request_parts);
    client->request_parts = NULL;
    client->request_parts_len_done = 0;
    client->request_parts_len = 0;
    client->request_parts_idx = 0;
    return HTTP_RETURN_OK;
}

static int32_t socket_write_request_parts(struct pico_http_client *client)
{
    uint32_t bytes_written = 0;
    uint32_t i = 0;
    uint32_t bytes_to_write = 0;
    uint32_t idx = 0;

    client->state = HTTP_WRITING_REQUEST;
    for (i = client->request_parts_len_done; i<client->request_parts_len; i++)
    {
        bytes_to_write = client->request_parts[i]->buf_len - client->request_parts[i]->buf_len_done;
        idx = client->request_parts[i]->buf_len_done;
        bytes_written = pico_socket_write(client->sck, (void *)&client->request_parts[i]->buf[idx], bytes_to_write);
        client->request_parts[i]->buf_len_done += bytes_written;
        /*uint32_t x = 0;
        for (x=0; x<bytes_to_write; x++)
        {
            printf("client->request_parts[i]->buf[%d] = %c\n", x, client->request_parts[i]->buf[x]);
        }
*/
        dbg("Bytes written: %d bytes_to_write: %d\n", bytes_written, bytes_to_write);
        if (bytes_written < 0)
        {
            request_parts_destroy(client);
            client->state = HTTP_CONN_IDLE;
            client->wakeup(EV_HTTP_WRITE_FAILED, client->connectionID);
        }
        else if (bytes_written == bytes_to_write)
        {
            client->request_parts_len_done += 1;
        }
        else
        {
            dbg("Could not fully write complete request.\n");
            break;
        }
        if (client->request_parts_len_done==client->request_parts_len)
        {
            dbg("Write success\n");
            request_parts_destroy(client);
            client->state = HTTP_START_READING_HEADER;
            if (client->long_polling_state == HTTP_LONG_POLL_CONN_CLOSE)
            {
                client->conn_state = HTTP_CONNECTION_WAITING_FOR_NEW_CONN;
            }
            client->wakeup(EV_HTTP_WRITE_SUCCESS, client->connectionID);
        }
    }
    return bytes_written;
}

static int8_t pico_http_uri_destroy(struct pico_http_uri *urikey)
{
    if (urikey)
    {
        if (urikey->resource)
        {
            PICO_FREE(urikey->resource);
            urikey->resource = NULL;
        }
        if (urikey->raw_uri)
        {
            PICO_FREE(urikey->raw_uri);
            urikey->raw_uri = NULL;
        }
        if (urikey->host)
        {
            PICO_FREE(urikey->host);
            urikey->host = NULL;
        }
        if (urikey->user_pass)
        {
            PICO_FREE(urikey->user_pass);
            urikey->user_pass = NULL;
        }
        PICO_FREE(urikey);
    }
    return HTTP_RETURN_OK;
}

static int8_t pico_process_resource(const char *resource, struct pico_http_uri *urikey)
{
    dbg("Start pico_process_resource(..) %s\n", resource);
    if (!resource || !urikey || resource[0] != '/')
    {
        pico_err = PICO_ERR_EINVAL;
        pico_http_uri_destroy(urikey);
        return HTTP_RETURN_ERROR;
    }
    if (urikey->resource && strcmp(urikey->resource, resource) == 0)
    {
        return HTTP_RETURN_OK;
    }
    if (urikey->resource)
    {
        PICO_FREE(urikey->resource);
        urikey->resource = NULL;
    }

    /* cpy the resource */
    urikey->resource = PICO_ZALLOC(strlen(resource)+1);

    if (!urikey->resource)
    {
        /* no memory */
        pico_err = PICO_ERR_ENOMEM;
        pico_http_uri_destroy(urikey);
        return HTTP_RETURN_ERROR;
    }

    //strcpy(urikey->resource, resource);
    memcpy(urikey->resource, resource, strlen(resource)+1);
    dbg("Stop pico_process_resource(..) %s\n", urikey->resource);
    return HTTP_RETURN_OK;
}

static int8_t pico_process_uri(const char *uri, struct pico_http_uri *urikey, int (*encoding)(char *out_buffer, char *in_buffer))
{
    uint16_t last_index = 0, index;
    uint16_t credentials_index = 0;
    uint8_t userpass_flag = 0;
    char buffin[128];
    char buffout[256];
    uint32_t outLen = sizeof(buffout);

    dbg("Start: pico_process_uri(..) %s\n", uri);
    if (!uri || !urikey || uri[0] == '/')
    {
        if (urikey)
            pico_http_uri_destroy(urikey);
        pico_err = PICO_ERR_EINVAL;
        return HTTP_RETURN_ERROR;
    }
    urikey->raw_uri = (char *)PICO_ZALLOC(strlen(uri)+1);
    if (!urikey->raw_uri)
    {
        pico_err = PICO_ERR_ENOMEM;
        pico_http_uri_destroy(urikey);
        return HTTP_RETURN_ERROR;
    }
    memcpy(urikey->raw_uri, uri, strlen(uri)+1);

    /* detect protocol => search for  "colon-slash-slash" */
    if (memcmp(uri, HTTP_PROTO_TOK, HTTP_PROTO_LEN) == 0) /* could be optimized */
    { /* protocol identified, it is http */
        urikey->protoHttp = 1;
        last_index = HTTP_PROTO_LEN;
    }
    else
    {
        if (strstr(uri, "://")) /* different protocol specified */
        {
            urikey->protoHttp = 0;
            pico_http_uri_destroy(urikey);
            return HTTP_RETURN_ERROR;
        }
        /* no protocol specified, assuming by default it's http */
        urikey->protoHttp = 1;
    }

    /* detect hostname */
    /*<user>:<password>@<host>:<port>/<url-path>*/
    /* Check if the user (and password) are specified. */
    dbg("detecting hostname\n");
    index = last_index;
    while ( uri[index] && uri[index] != '\0' )
    {
    	if ( uri[index] == '@') {
    		/* Username and password are specified */
    		userpass_flag = 1;
    		break;
    	} else if ( uri[index] == '/' ) {
    		/* End of <host>:<port> specification */
    		userpass_flag = 0;
    		break;
    	}

    	index++;
    }

    if (userpass_flag){
        dbg("skipping user name and password to find host\n");
        // Skip the @
        index++;
        credentials_index = last_index;
        last_index = index;
    }
    else
        index = last_index;

    dbg("Check hostname format\n");
    while (uri[index] && uri[index] != '/' && uri[index] != ':') index++;
    if (index == last_index)
    {
        /* wrong format */
    	dbg("wrong format\n");
        urikey->port = urikey->protoHttp = 0u;
        pico_err = PICO_ERR_EINVAL;
        pico_http_uri_destroy(urikey);
        return HTTP_RETURN_ERROR;
    }
    else
    {
        /* extract host */
    	dbg("extract host\n");
        urikey->host = PICO_ZALLOC((uint32_t)(index - last_index + 1));

        if(!urikey->host)
        {
            /* no memory */
            pico_err = PICO_ERR_ENOMEM;
            pico_http_uri_destroy(urikey);
            return HTTP_RETURN_ERROR;
        }

        memcpy(urikey->host, uri + last_index, (size_t)(index - last_index));

        /* extract user credentials */
	if ( userpass_flag ){
		dbg("extract login and password\n");
        	int inLen = last_index - credentials_index - 1;
        	strncpy(buffin, uri + credentials_index, (size_t) inLen);
		buffin[inLen]='\0';

		//Clearing the memory of the out buffer
            	memset(buffout, 0, outLen);

		//if encoding is specified then the encoding of the username and password can be applied
		if (encoding != NULL) {
			if (encoding(buffout, buffin) < 0) {
				dbg("error happened while encoding");
			}
		} else {
			memcpy(buffout, buffin, (size_t)strlen(buffin));
		}

		urikey->user_pass = PICO_ZALLOC((uint32_t)(strlen(buffout)+1));
		if(!urikey->user_pass) {
			/* no memory */
			pico_err = PICO_ERR_ENOMEM;
			pico_http_uri_destroy(urikey);
			return HTTP_RETURN_ERROR;
		}
		memcpy(urikey->user_pass, buffout, (size_t)(strlen(buffout)+1));
		dbg("processed userpass = %s and lenght = %i\n", urikey->user_pass, (int) strlen(urikey->user_pass));
	}

    }

    if (!uri[index] || uri[index] == '/')
    {
        /* nothing specified */
        urikey->port = 80u;
    }
    else if (uri[index] == ':' && uri[index+1] && uri[index+1] == '/') {
        // No port after ':'
        dbg("No port after ':' \n");
        urikey->port = urikey->protoHttp = 0u;
        pico_err = PICO_ERR_EINVAL;
        pico_http_uri_destroy(urikey);
        return HTTP_RETURN_ERROR;
    }
    else if (uri[index] == ':')
    {
        urikey->port = 0u;
        index++;
        while (uri[index] && uri[index] != '/')
        {
            if (!isdigit(uri[index])){
                dbg("Port is not a number\n");
                urikey->host = urikey->resource = urikey->user_pass = NULL;
                urikey->port = urikey->protoHttp = 0u;
                pico_http_uri_destroy(urikey);
                return HTTP_RETURN_ERROR;
            }
            urikey->port = (uint16_t)(urikey->port * 10 + (uri[index] - '0'));
            index++;
        }
    }

    dbg("End: pico_process_uri(..) index: %d\n", index);
    return pico_process_resource(&uri[index], urikey);
 }

static int32_t compare_clients(void *ka, void *kb)
{
    return ((struct pico_http_client *)ka)->connectionID - ((struct pico_http_client *)kb)->connectionID;
}

PICO_TREE_DECLARE(pico_client_list, compare_clients);

/* Local functions */
static int8_t parse_header_from_server(struct pico_http_client *client, struct pico_http_header *header);
static int8_t read_chunk_line(struct pico_http_client *client);
/*  */
/*
void print_header(struct pico_http_header * header)
{
    printf("In lib: Received header from server...\n");
    printf("In lib: Server response : %d\n",header->response_code);
    printf("In lib: Location : %s\n",header->location);
    printf("In lib: Transfer-Encoding : %d\n",header->transfer_coding);
    printf("In lib: Size/Chunk : %d\n",header->content_length_or_chunk);
}
*/
static inline void wait_for_header(struct pico_http_client *client)
{
    /* wait for header */
    int8_t http_ret;

    http_ret = parse_header_from_server(client, client->header);
    if (http_ret < 0)
    {
        client->wakeup(EV_HTTP_ERROR, client->connectionID);
    }
    else if (http_ret == HTTP_RETURN_BUSY)
    {
        client->state = HTTP_READING_HEADER;
    }
    /*else if (http_ret == HTTP_RETURN_NOT_FOUND)
    {
        print_header(client->header);
        client->state = HTTP_CONN_IDLE;
        client->body_read = 0;
        client->wakeup(EV_HTTP_REQ, client->connectionID);
    }
    else*/
    {
        /* call wakeup */
        if (client->header->response_code != HTTP_CONTINUE)
        {
            /*if (client->header->response_code == HTTP_OK)
            {
                client->wakeup((EV_HTTP_REQ | EV_HTTP_BODY), client->connectionID);
            }
            else
            {
                client->state = HTTP_CONN_IDLE;
                client->body_read = 0;
                client->wakeup(EV_HTTP_REQ, client->connectionID);
            }*/
            if (client->header->content_length_or_chunk)
            {
                client->wakeup((EV_HTTP_REQ | EV_HTTP_BODY), client->connectionID);
            }
            else
            {
                client->wakeup(EV_HTTP_REQ, client->connectionID);
            }
        }
    }
}

static void treat_write_event(struct pico_http_client *client)
{
    /* write request parts if not everything has been written allready */
    dbg("treat write event, client state: %d\n", client->state);
    uint32_t bytes_written = 0;
    if (client->request_parts_len_done != client->request_parts_len)
    {
        bytes_written = socket_write_request_parts(client);
        dbg("Bytes written: %d\n", bytes_written);
        if (bytes_written && !client->long_polling_state)
        {
            client->wakeup(EV_HTTP_WRITE_PROGRESS_MADE, client->connectionID);
        }
    }
    else
    {
        //dbg("No request parts to write.\n");
    }
}

static void treat_read_event(struct pico_http_client *client)
{
    /* read the header, if not read */
    dbg("treat read event, client state: %d\n", client->state);
    if (client->state == HTTP_START_READING_HEADER)
    {
        /* wait for header */
        dbg("Wait for header\n");
        free_header(client); //when using keep alive, we create a new one
        client->header = PICO_ZALLOC(sizeof(struct pico_http_header));
        if (!client->header)
        {
            pico_err = PICO_ERR_ENOMEM;
            dbg("Client not found!\n");
            return;
        }

        wait_for_header(client);
    }
    else if (client->state == HTTP_READING_HEADER)
    {
        wait_for_header(client);
    }
    else
    {
        /* just let the user know that data has arrived, if chunked data comes,
         * will be treated in the  read api. */
        client->wakeup(EV_HTTP_BODY, client->connectionID);
    }
}

static void treat_long_polling(struct pico_http_client *client, uint16_t ev)
{
    uint32_t conn = 0;
    char *raw_uri = NULL;
    char *resource = NULL;
    //void *wakeup = NULL;
    void (*wakeup)(uint16_t ev, uint16_t conn) = NULL;
    uint8_t cpy_long_polling_state = 0;
    uint8_t cpy_body_read_done = 0;
    dbg("TREAT LONG POLLING\n");
    conn = client->connectionID;

    dbg("client->body_read_done: %d client->conn_state: %d\n", client->body_read_done, client->conn_state);

    if (client->body_read_done && client->conn_state == HTTP_CONNECTION_CONNECTED)
    {
        client->body_read_done = 0;
        if(client->long_polling_state == HTTP_LONG_POLL_CONN_KEEP_ALIVE)
        {
            pico_http_client_long_poll_send_get(conn, client->urikey->resource, HTTP_CONN_KEEP_ALIVE);
        }
        else
        {
            pico_http_client_long_poll_send_get(conn, client->urikey->resource, HTTP_CONN_CLOSE);
        }
    }
    else if(client->body_read_done && client->conn_state == HTTP_CONNECTION_NOT_CONNECTED)
    {
        if (client->long_polling_state != HTTP_LONG_POLL_CONN_CLOSE)
        {
            dbg("Connection: Keep-Alive, but still got close ev, setup new connection.");
        }
        raw_uri = PICO_ZALLOC(strlen(client->urikey->raw_uri)+1);
        if (!raw_uri)
        {
            pico_err = PICO_ERR_ENOMEM;
            client->wakeup(EV_HTTP_ERROR | EV_HTTP_CLOSE, client->connectionID);
            return;
        }
        strcpy(raw_uri, client->urikey->raw_uri);
        resource = PICO_ZALLOC(strlen(client->urikey->resource) + 1);
        if (!resource)
        {
            PICO_FREE(raw_uri);
            pico_err = PICO_ERR_ENOMEM;
            client->wakeup(EV_HTTP_ERROR | EV_HTTP_CLOSE, client->connectionID);
            return;
        }
        strcpy(resource, client->urikey->resource);
        wakeup = client->wakeup;
        cpy_long_polling_state = client->long_polling_state;
        cpy_body_read_done = client->body_read_done;
        pico_http_client_close(client->connectionID);
        dbg("treat_long_polling before client_open\n");
        conn = client_open(raw_uri, wakeup, conn, NULL);
        dbg("treat_long_polling after client_open\n");
        if (conn < 0)
        {
            wakeup(EV_HTTP_ERROR | EV_HTTP_CLOSE, conn);
            PICO_FREE(raw_uri);
            PICO_FREE(resource);
            return;
        }
        //refresh client with the new one but same connectionID
        struct pico_http_client search = {
            .connectionID = conn
        };
        client = pico_tree_findKey(&pico_client_list, &search);
        if (!client)
        {
            PICO_FREE(raw_uri);
            PICO_FREE(resource);
            dbg("treat_long_polling Client not found!\n");
            wakeup(EV_HTTP_ERROR | EV_HTTP_CLOSE, conn);
            return;
        }
        //
        client->body_read_done = cpy_body_read_done;
        //put back the long polling state
        client->long_polling_state = cpy_long_polling_state;
        //put the correct resource back
        pico_process_resource(resource, client->urikey);
        PICO_FREE(raw_uri);
        PICO_FREE(resource);
    }
}

static void tcp_callback(uint16_t ev, struct pico_socket *s)
{
    struct pico_http_client *client = NULL;
    struct pico_tree_node *index;
    int16_t r_ev = 0;
    dbg("tcp callback (%d)\n", ev);
    /* find http_client */
    pico_tree_foreach(index, &pico_client_list)
    {
        if (((struct pico_http_client *)index->keyValue)->sck == s )
        {
            client = (struct pico_http_client *)index->keyValue;
            break;
        }
    }
    dbg("Client_ptr: %p\n", client);
    if (!client)
    {
        dbg("Client not found...Something went wrong !\n");
        return;
    }

    if (ev & PICO_SOCK_EV_CONN)
    {
        client->conn_state = HTTP_CONNECTION_CONNECTED;
        if (client->long_polling_state)
        {
            treat_long_polling(client, 0);
        }
        client->wakeup(EV_HTTP_CON, client->connectionID);
    }

    if (ev & PICO_SOCK_EV_ERR)
    {
        r_ev = EV_HTTP_ERROR;
        client->conn_state = HTTP_CONNECTION_NOT_CONNECTED;
        if (client->long_polling_state)
        {
            treat_long_polling(client, 0);
        }
        if (client->request_parts)
        {
            request_parts_destroy(client);
            client->wakeup(EV_HTTP_WRITE_FAILED, client->connectionID);
            r_ev = r_ev | EV_HTTP_WRITE_FAILED;
        }
        client->state = HTTP_CONN_IDLE;
        client->wakeup(r_ev, client->connectionID);
    }

    if ((ev & PICO_SOCK_EV_CLOSE) || (ev & PICO_SOCK_EV_FIN))
    {
        client->conn_state = HTTP_CONNECTION_NOT_CONNECTED;
        r_ev = EV_HTTP_CLOSE;
        if (client->long_polling_state)
        {
            treat_long_polling(client, 0);
        }
        if (client->request_parts)
        {
            request_parts_destroy(client);
            client->wakeup(EV_HTTP_WRITE_FAILED, client->connectionID);
            r_ev = r_ev | EV_HTTP_WRITE_FAILED;
        }
        client->state = HTTP_CONN_IDLE;
        dbg("long polling state %d\n", client->long_polling_state);
        client->wakeup(r_ev, client->connectionID);
    }

    if (ev & PICO_SOCK_EV_WR)
    {
        treat_write_event(client);
    }

    if (ev & PICO_SOCK_EV_RD)
    {
        treat_read_event(client);
    }
}

/* used for getting a response from DNS servers */
static void dns_callback(char *ip, void *ptr)
{
    struct pico_http_client *client = (struct pico_http_client *)ptr;
    uint32_t val = 0;
    if (!client)
    {
        dbg("Who made the request ?!\n");
        return;
    }

    if (ip)
    {
        client->wakeup(EV_HTTP_DNS, client->connectionID);

        /* add the ip address to the client, and start a tcp connection socket */
        pico_string_to_ipv4(ip, &client->ip.addr);
        client->sck = pico_socket_open(PICO_PROTO_IPV4, PICO_PROTO_TCP, &tcp_callback);
        if (!client->sck)
        {
            client->wakeup(EV_HTTP_ERROR, client->connectionID);
            return;
        }
        val = 60000;
        pico_socket_setoption(client->sck, PICO_SOCKET_OPT_KEEPIDLE, &val);
        pico_socket_setoption(client->sck, PICO_SOCKET_OPT_KEEPINTVL, &val);
        val = 7;
        pico_socket_setoption(client->sck, PICO_SOCKET_OPT_KEEPCNT, &val);
        dbg("client->sck: %p\n", client->sck);
        if (pico_socket_connect(client->sck, &client->ip, short_be(client->urikey->port)) < 0)
        {
            client->wakeup(EV_HTTP_ERROR, client->connectionID);
            return;
        }
    }
    else
    {
        /* wakeup client and let know error occured */
        client->wakeup(EV_HTTP_ERROR, client->connectionID);

        /* close the client (free used heap) */
        pico_http_client_close(client->connectionID);
    }
}

/*
 * API for reading received data.
 *
 * Builds a GET request based on the fields on the uri.
 */
char *pico_http_client_build_get(const struct pico_http_uri *uri_data, uint8_t connection_type)
{
    char *header;
    char port[6u]; /* 6 = max length of a uint16 + \0 */
    uint64_t header_size = HTTP_GET_BASIC_SIZE;

    if (!uri_data->host || !uri_data->resource || !uri_data->port)
    {
        pico_err = PICO_ERR_EINVAL;
        return NULL;
    }

    if (!uri_data->user_pass){
    	dbg("USING NO USERPASS\n");
    	header_size += (header_size + strlen(uri_data->host));
		header_size += (header_size + strlen(uri_data->resource));
		header_size += (header_size + pico_itoa(uri_data->port, port) + 4u); /* 3 = size(CRLF + \0) */
		header = PICO_ZALLOC(header_size);

		if (!header)
		{
			/* not enough memory */
			pico_err = PICO_ERR_ENOMEM;
			return NULL;
		}

		/* build the actual header */
		sprintf(header, "GET %s HTTP/1.1\r\nHost: %s:%s\r\nUser-Agent: picoTCP\r\nConnection: %s\r\n\r\n",
				uri_data->resource,
				uri_data->host,
				port,
				connection_type == HTTP_CONN_CLOSE ? "Close":"Keep-Alive");
    } else {
    	dbg("USING USERPASS\n");
		printf("user_pass: %s\n",uri_data->user_pass);
    	 /* build the header with authorization*/

    	dbg("creating header length\n");
    	header_size += (header_size + strlen(uri_data->host));
    	header_size += (header_size + strlen(uri_data->user_pass));
		header_size += (header_size + strlen(uri_data->resource));
		header_size += (header_size + pico_itoa(uri_data->port, port) + 4u); /* 3 = size(CRLF + \0) */
		header = PICO_ZALLOC(header_size);

		if (!header)
		{
			/* not enough memory */
			dbg("error header not created\n");
			pico_err = PICO_ERR_ENOMEM;
			return NULL;
		}

		/* build the actual header */
		dbg("building the header/n");
		sprintf(header, "GET %s HTTP/1.1\r\nAuthorization: Basic %s\r\nHost: %s:%s\r\nUser-Agent: picoTCP\r\nConnection: %s\r\n\r\n",
						uri_data->resource,
						uri_data->user_pass,
		                uri_data->host,
		                port,
		                connection_type == HTTP_CONN_CLOSE ? "Close":"Keep-Alive");
    }




    return header;
}


/*
 * Builds a DELETE header based on the fields of the uri
 */
static char *pico_http_client_build_delete(const struct pico_http_uri *uri_data, uint8_t connection_type)
{
    char *header;
    char port[6u]; /* 6 = max length of a uint16 + \0 */
    uint64_t header_size = HTTP_DELETE_BASIC_SIZE;

    if (!uri_data->host || !uri_data->resource || !uri_data->port)
    {
        pico_err = PICO_ERR_EINVAL;
        return NULL;
    }

    /*  */
    header_size += (header_size + strlen(uri_data->host));
    header_size += (header_size + strlen(uri_data->resource));
    header_size += (header_size + pico_itoa(uri_data->port, port) + 4u); /* 3 = size(CRLF + \0) */
    header = PICO_ZALLOC(header_size);

    if (!header)
    {
        /* not enough memory */
        pico_err = PICO_ERR_ENOMEM;
        return NULL;
    }

    /* build the actual header */
    sprintf(header, "DELETE %s HTTP/1.1\r\nUser-Agent: picoTCP\r\nAccept: */*\r\nHost: %s:%s\r\nConnection: %s\r\n\r\n",
            uri_data->resource,
            uri_data->host,
            port,
            connection_type ==  HTTP_CONN_CLOSE ? "Close":"Keep-Alive");
    return header;
}


struct multipart_chunk *multipart_chunk_create(unsigned char *data, uint64_t length_data, char *name, char *filename, char *content_disposition, char *content_type)
{
    if (length_data <= 0 || data == NULL)
    {
        return NULL;
    }

    struct multipart_chunk *chunk = PICO_ZALLOC(sizeof(struct multipart_chunk));

    if (!chunk)
    {
        /* not enough memory */
        pico_err = PICO_ERR_ENOMEM;
        return NULL;
    }
    if (data)
    {
        chunk->data = PICO_ZALLOC(length_data);
        memcpy(chunk->data, data, length_data);
        chunk->length_data = length_data;
    }
    if (name)
    {
        chunk->name = strdup(name);
        chunk->length_name = strlen(name);
    }
    if (content_disposition)
    {
        chunk->content_disposition = strdup(content_disposition);
        chunk->length_content_disposition = strlen(content_disposition);
    }
    if (filename)
    {
        chunk->filename = strdup(filename);
        chunk->length_filename = strlen(filename);
    }
    if (content_type)
    {
        chunk->content_type = strdup(content_type);
        chunk->length_content_type = strlen(content_type);
    }
    return chunk;
}

int8_t multipart_chunk_destroy(struct multipart_chunk *chunk)
{
    if (!chunk)
    {
        return -1;
    }
    if (chunk->data)
    {
        PICO_FREE(chunk->data);
    }
    if (chunk->name)
    {
        PICO_FREE(chunk->name);
    }
    if (chunk->filename)
    {
        PICO_FREE(chunk->filename);
    }
    if (chunk->content_type)
    {
        PICO_FREE(chunk->content_type);
    }
    if (chunk->content_disposition)
    {
        PICO_FREE(chunk->content_disposition);
    }
    PICO_FREE(chunk);
    return 0;
}

static int32_t get_content_length(struct multipart_chunk **post_data, uint16_t post_data_elements, const char *boundary)
{
    uint32_t i;
    uint64_t total_content_length = 0;
    for (i=0; i<post_data_elements; i++)
    {
        if (post_data[i]->data != NULL)
        {
            total_content_length += 2; // "--"
            total_content_length += strlen(boundary);
            total_content_length += 2; // "\r\n"
            if (post_data[i]->content_disposition != NULL)
            {
                total_content_length += 21 ; // "Content-Disposition: "
                total_content_length += post_data[i]->length_content_disposition;
                if (post_data[i]->name != NULL)
                {
                    total_content_length += 8; // "; name=\""
                    total_content_length += post_data[i]->length_name;
                    total_content_length += 1; // "\""
                }
                if (post_data[i]->filename != NULL)
                {
                    total_content_length += 12; // "; filename=\""
                    total_content_length += post_data[i]->length_filename;
                    total_content_length += 1; // "\""
                }
            }
            if (post_data[i]->content_type != NULL)
            {
                total_content_length += 2; // "\r\n"
                total_content_length += 14; // "Content-type: "
                total_content_length += post_data[i]->length_content_type;
            }
            total_content_length += 4; // "\r\n\r\n"
            total_content_length += post_data[i]->length_data;
            total_content_length += 2; // "\r\n"
        }
    }
    total_content_length += 2; // "--"
    total_content_length += strlen(boundary);
    total_content_length += 4; // "--\r\n"
    return total_content_length;
}

static int32_t get_max_multipart_header_size(struct multipart_chunk **post_data, uint16_t length)
{
    uint32_t max = 0;
    uint32_t len = 0;
    uint32_t i = 0;
    for (i=0; i<length; i++)
    {
        len = 0;
        len += post_data[i]->length_content_disposition;
        len += post_data[i]->length_name;
        len += post_data[i]->length_filename;
        len += post_data[i]->length_content_type;
        if (max < len)
        {
            max = len;
        }
    }
    return max;
}

/*
 * post_data: list with the multipart chunk that need to be added to the http client struct.
 * post_data_length: number of elements in post_data.
 * boundary: the const boundary used to separate the chunks.
 * http: the http client struct where we need to add the chunks to.
 */
static int8_t add_multipart_chunks(struct multipart_chunk **post_data, uint16_t post_data_length, const char *boundary, struct pico_http_client *http)
{
    uint32_t i = 0;
    char *buf = NULL;
    uint32_t buf_size = HTTP_POST_MULTIPART_BASIC_SIZE;

    buf_size += get_max_multipart_header_size(post_data, post_data_length);
    buf_size += strlen(boundary);
    buf = PICO_ZALLOC(buf_size);

    if(!buf)
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    for (i=0; i<post_data_length; i++)
    {
        if (post_data[i]->data != NULL)
        {
            if (i == 0)
                strcpy(buf,"--");
            else
                strcat(buf,"--");
            strcat(buf, boundary);
            strcat(buf, "\r\n");
            if (post_data[i]->content_disposition != NULL)
            {
                strcat(buf, "Content-Disposition: ");
                strcat(buf, post_data[i]->content_disposition);
                if (post_data[i]->name != NULL)
                {
                    strcat(buf, "; name=\"");
                    strcat(buf, post_data[i]->name);
                    strcat(buf, "\"");
                }
                if (post_data[i]->filename != NULL)
                {
                    strcat(buf, "; filename=\"");
                    strcat(buf, post_data[i]->filename);
                    strcat(buf, "\"");
                }
            }
            if (post_data[i]->content_type != NULL)
            {
                strcat(buf, "\r\n");
                strcat(buf, "Content-type: ");
                strcat(buf, post_data[i]->content_type);
            }
            strcat(buf, "\r\n\r\n");

            http->request_parts[http->request_parts_len] = request_part_create(buf, strlen(buf), HTTP_COPY_TO_HEAP, HTTP_NO_USER_MEM);
            if (!http->request_parts[http->request_parts_len])
            {
                PICO_FREE(buf);
                pico_err = PICO_ERR_ENOMEM;
                return HTTP_RETURN_ERROR;
            }
            http->request_parts_len += 1;

            http->request_parts[http->request_parts_len] = request_part_create((char *)post_data[i]->data, post_data[i]->length_data, HTTP_NO_COPY_TO_HEAP, HTTP_USER_MEM);
            if (!http->request_parts[http->request_parts_len])
            {
                PICO_FREE(buf);
                pico_err = PICO_ERR_ENOMEM;
                return HTTP_RETURN_ERROR;
            }
            http->request_parts_len += 1;
            strcpy(buf, "\r\n");
        }
    }
    strcat(buf,"--");
    strcat(buf, boundary);
    strcat(buf, "--\r\n");

    http->request_parts[http->request_parts_len] = request_part_create(buf, strlen(buf), HTTP_COPY_TO_HEAP, HTTP_USER_MEM);
    if (!http->request_parts[http->request_parts_len])
    {
        PICO_FREE(buf);
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts_len += 1;
    PICO_FREE(buf);
    return HTTP_RETURN_OK;
}

static int8_t pico_http_client_build_post_multipart_request(const struct pico_http_uri *uri_data, struct multipart_chunk **post_data, uint16_t len, struct pico_http_client *http, uint8_t connection_type)
{
    char *header = NULL;
    uint32_t content_length = 0;
    uint32_t data_length = 0;
    char port[6u]; /* 6 = max length of a uint16 + \0 */
    uint32_t header_size = HTTP_POST_HEADER_BASIC_SIZE;
    const char *boundary = "--------------------------c6b5ca0828dmx010";
    char str_content_length[6u];
    uint16_t i = 0;
    uint8_t rv = HTTP_RETURN_OK;

    if (!uri_data->host || !uri_data->resource || !uri_data->port)
    {
        pico_err = PICO_ERR_EINVAL;
        return -1;
    }
    content_length = get_content_length(post_data, len, boundary);
    sprintf(str_content_length, "%d", content_length);
    for (i=0; i<len; i++)
    {
        if (post_data[i]->data)
        {
            data_length += post_data[i]->length_data;
        }
    }

    header_size = (header_size + strlen(uri_data->host));
    header_size = (header_size + strlen(uri_data->resource));
    header_size = (header_size + pico_itoa(uri_data->port, port) + 4u); /* 3 = size(CRLF + \0) */
    header_size = (header_size + strlen(boundary));
    header = PICO_ZALLOC(header_size);
    if (!header)
    {
        /* not enough memory */
        pico_err = PICO_ERR_ENOMEM;
        return -1;
    }

    /* build the actual header */
    sprintf(header, "POST %s HTTP/1.1\r\nUser-Agent: picoTCP\r\nAccept: */*\r\nHost: %s:%s\r\nConnection: %s\r\nContent-Length: %s\r\nContent-Type: multipart/mixed; boundary=%s\r\n\r\n",
            uri_data->resource,
            uri_data->host,
            port,
            connection_type == HTTP_CONN_CLOSE ? "Close":"Keep-Alive",
            str_content_length,
            boundary);

    http->request_parts[http->request_parts_len] = request_part_create(header, strlen(header), HTTP_NO_COPY_TO_HEAP, HTTP_NO_USER_MEM);
    if (!http->request_parts[0])
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts_len += 1;
    rv = add_multipart_chunks(post_data, len, boundary, http);
    return rv;
}

/*
 * Builds a POST header based on the fields of the uri provided.
 */
static char *pico_http_client_build_post_header(const struct pico_http_uri *uri_data, uint32_t post_data_len, uint8_t connection_type, char *content_type, char *cache_control)
{
    char *header;
    char port[6u]; /* 6 = max length of a uint16 + \0 */
    uint64_t header_size = HTTP_POST_BASIC_SIZE;
    char str_post_data_len[6u];

    if (!uri_data->host || !uri_data->resource || !uri_data->port)
    {
        pico_err = PICO_ERR_EINVAL;
        return NULL;
    }
    sprintf(str_post_data_len, "%d", post_data_len);
    /*  */
    header_size += (header_size + strlen(uri_data->host));
    header_size += (header_size + strlen(uri_data->resource));
    header_size += (header_size + pico_itoa(uri_data->port, port) + 4u); /* 3 = size(CRLF + \0) */
    header_size += 6u; //str_post_data_len
    header = PICO_ZALLOC(header_size);

    if (!header)
    {
        /* not enough memory */
        pico_err = PICO_ERR_ENOMEM;
        return NULL;
    }

    /* build the actual header */
    sprintf(header, "POST %s HTTP/1.1\r\nUser-Agent: picoTCP\r\nAccept: */*\r\nHost: %s:%s\r\nConnection: %s\r\nContent-Type: %s\r\nCache-Controle: %s\r\nContent-Length: %s\r\n\r\n",
            uri_data->resource,
            uri_data->host,
            port,
            connection_type == HTTP_CONN_CLOSE ? "Close":"Keep-Alive",
            content_type == NULL ? "application/x-www-form-urlencoded":content_type,
            cache_control == NULL ? "private, max-age=0, no-cache":cache_control,
            str_post_data_len);
    return header;
}
/*  */

/*
 * API used to check how many bytes are allready written.
 *
 * The function acceptes the connectionID and 2 pointers to store the number of written bytes and total bytes to write.
 *
 * The function returns -1 if connectionID is not found or there is nothing to write anymore.
 * 'Total_bytes_to_write' can be NULL
 */
int8_t MOCKABLE pico_http_client_get_write_progress(uint16_t conn, uint32_t *total_bytes_written, uint32_t *total_bytes_to_write)
{
    uint32_t i = 0;
    struct pico_http_client search = {
        .connectionID = conn
    };
    struct pico_http_client *client = pico_tree_findKey(&pico_client_list, &search);
    if (!client)
    {
        dbg("Client not found !\n");
        return HTTP_RETURN_ERROR;
    }
    if (!total_bytes_written)
    {
        return HTTP_RETURN_ERROR;
    }

    *total_bytes_written = 0;
    if (total_bytes_to_write)
    {
        *total_bytes_to_write = 0;
    }

    if (client->request_parts)
    {
        for (i=0; i<client->request_parts_len; i++)
        {
            *total_bytes_written += client->request_parts[i]->buf_len_done;
            if (total_bytes_to_write)
            {
                *total_bytes_to_write += client->request_parts[i]->buf_len;
            }
        }
        return HTTP_RETURN_OK;
    }
    else
    {
        return HTTP_RETURN_ERROR;
    }

}

static int32_t client_open(char *uri, void (*wakeup)(uint16_t ev, uint16_t conn), int32_t connID, int (*encoding)(char *out_buffer, char *in_buffer))
{
    struct pico_http_client *client;
    uint32_t ip = 0;

    if (!wakeup || !uri)
    {
        return HTTP_RETURN_ERROR;
    }

    client = PICO_ZALLOC(sizeof(struct pico_http_client));
    if (!client)
    {
        /* memory error */
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }

    client->wakeup = wakeup;
    client->connectionID = connID >= 0 ? connID : global_client_conn_ID++;

    client->urikey = PICO_ZALLOC(sizeof(struct pico_http_uri));

    if (!client->urikey)
    {
        pico_err = PICO_ERR_ENOMEM;
        PICO_FREE(client);
        return HTTP_RETURN_ERROR;
    }

    if (pico_process_uri(uri, client->urikey, encoding) < 0)
    {
        PICO_FREE(client);
        return HTTP_RETURN_ERROR;
    }

    if (pico_tree_insert(&pico_client_list, client))
    {
        /* already in */
        pico_err = PICO_ERR_EEXIST;
        free_uri(client);
        PICO_FREE(client);
        return HTTP_RETURN_ALREADYIN;
    }

    /* dns query */
    if (pico_string_to_ipv4(client->urikey->host, &ip) == -1)
    {
        dbg("Querying : %s \n", client->urikey->host);
        pico_dns_client_getaddr(client->urikey->host, (void *)dns_callback, client);
    }
    else
    {
        dbg("host already and ip address, no dns required\n");
        dns_callback(client->urikey->host, client);
    }

    /* return the connection ID */
    return client->connectionID;
}


/*
 * API used for opening a new HTTP Client.
 *
 * The accepted uri's are [http:]['hostname'][:port]/['resource']
 *
 * The function returns a connection ID >= 0 if successful
 * -1 if an error occured.
 */
int32_t MOCKABLE pico_http_client_open_with_usr_pwd_encoding(char *uri, void (*wakeup)(uint16_t ev, uint16_t conn), int (*encoding)(char *out_buffer, char *in_buffer))
{
    return client_open(uri, wakeup, -1, encoding);
}


int32_t MOCKABLE pico_http_client_open(char *uri, void (*wakeup)(uint16_t ev, uint16_t conn))
{
    return client_open(uri, wakeup, -1, NULL);
}

/*
 * API for sending a header POST multipart to the client.
 *
 * The library will build the response request
 * based on the uri/post_data passed when opening the client.
 *
 * POST request:
 *  post_data: pointer to multipart_chunk
 *  length_post_data: length of the multipart_chunk array
 */
int8_t MOCKABLE pico_http_client_send_post_multipart(uint16_t conn, char *resource, struct multipart_chunk **post_data, uint16_t length_post_data, uint8_t connection_type)
{
    struct pico_http_client search = {
        .connectionID = conn
    };
    struct pico_http_client *http = NULL;
    int32_t bytes_written;
    int8_t rv = 0;

    dbg("POST MULTIPART request\n");

    if (!post_data || !length_post_data)
    {
        return HTTP_RETURN_ERROR;
    }

    http = pico_tree_findKey(&pico_client_list, &search);
    if (!http)
    {
        dbg("Client not found !\n");
        return HTTP_RETURN_ERROR;
    }
    if (http->state != HTTP_CONN_IDLE)
    {
        return HTTP_RETURN_CONN_BUSY;
    }
    if (connection_type != HTTP_CONN_CLOSE && connection_type != HTTP_CONN_KEEP_ALIVE)
    {
        return HTTP_RETURN_ERROR;
    }
    if (resource)
    {
        if(pico_process_resource(resource, http->urikey) < 0)
        {
            PICO_FREE(http);
            return HTTP_RETURN_ERROR;
        }
    }
    http->request_parts = PICO_ZALLOC((2+length_post_data*2) * sizeof(struct request_part *)); //header + end_boundary + 2*length_post_data
    if (!http->request_parts)
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts_len = 0;
    http->request_parts_len_done = 0;

    /* the api gives the possibility to the user to build the POST multipart header */
    /* based on the uri passed when opening the client, less headache for the user */
    rv = pico_http_client_build_post_multipart_request(http->urikey, post_data, length_post_data, http, connection_type);
    if (rv == HTTP_RETURN_ERROR)
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    bytes_written = socket_write_request_parts(http);
    if (bytes_written < 0)
    {
        return HTTP_RETURN_ERROR;
    }
    return HTTP_RETURN_OK;
}

/*
 * API for sending a DELETE request.
 *
 * The library will build the request
 * based on the uri passed when opening the client.
 * Retruns HTTP_RETURN_CONN_BUSY (-3) if the conn is still writing or reading data
 */
int8_t MOCKABLE pico_http_client_send_delete(uint16_t conn, char *resource, uint8_t connection_type)
{
    char *request = NULL;
    struct pico_http_client search = {
        .connectionID = conn
    };
    struct pico_http_client *http = pico_tree_findKey(&pico_client_list, &search);
    int32_t bytes_written = 0;

    dbg("DELETE request\n");

    if (!http)
    {
        dbg("Client not found !\n");
        return HTTP_RETURN_ERROR;
    }
    if (http->state != HTTP_CONN_IDLE)
    {
        return HTTP_RETURN_CONN_BUSY;
    }
    if (connection_type != HTTP_CONN_CLOSE && connection_type != HTTP_CONN_KEEP_ALIVE)
    {
        return HTTP_RETURN_ERROR;
    }
    if (resource)
    {
        if (pico_process_resource(resource, http->urikey) < 0)
        {
            PICO_FREE(http);
            return HTTP_RETURN_ERROR;
        }
    }
    http->request_parts = PICO_ZALLOC(1 * sizeof(struct request_part *));
    if (!http->request_parts)
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts_len = 0;
    http->request_parts_len_done = 0;

    request = pico_http_client_build_delete(http->urikey, connection_type);
    dbg("DELETE: request: \n%s\n", request);
    if (!request)
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts[http->request_parts_len] = request_part_create(request, strlen(request), HTTP_NO_COPY_TO_HEAP, HTTP_NO_USER_MEM);
    if (!http->request_parts[http->request_parts_len])
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts_len += 1;
    bytes_written = socket_write_request_parts(http);
    if (bytes_written < 0)
    {
        return HTTP_RETURN_ERROR;
    }
    return HTTP_RETURN_OK;
}

/*
 * API for sending a POST request.
 *
 * The library will build the request
 * based on the uri passed when opening the client.
 *
 * User should not FREE the post_data until it has been written to the http_socket.
 * Callback will indicate this.
 *
 * connection_type: HTTP_CONN_CLOSE/HTTP_CONN_KEEP_ALIVE
 * content_type: if NULL --> default value is passed
 * cache_control: HTTP_CACHE_CONTROL/HTTP_NO_CACHE_CONTROL
 * post_data example: "var_1=value_1&var_2=value_2"
 *
 */
int8_t MOCKABLE pico_http_client_send_post(uint16_t conn, char *resource, uint8_t *post_data, uint32_t post_data_len, uint8_t connection_type, char *content_type, char *cache_control)
{
    char *header = NULL;
    struct pico_http_client search = {
        .connectionID = conn
    };
    struct pico_http_client *http = NULL;
    int32_t bytes_written = 0;

    dbg("POST request\n");

    if(!post_data || !post_data_len)
    {
        return HTTP_RETURN_ERROR;
    }
    http = pico_tree_findKey(&pico_client_list, &search);
    if (!http)
    {
        dbg("Client not found !\n");
        return HTTP_RETURN_ERROR;
    }

    if (http->state != HTTP_CONN_IDLE)
    {
        return HTTP_RETURN_CONN_BUSY;
    }

    if (connection_type != HTTP_CONN_CLOSE && connection_type != HTTP_CONN_KEEP_ALIVE)
    {
        return HTTP_RETURN_ERROR;
    }

    if (resource)
    {
        if(pico_process_resource(resource, http->urikey) < 0)
        {
            PICO_FREE(http);
            return HTTP_RETURN_ERROR;
        }
    }

    http->request_parts = PICO_ZALLOC(2 * sizeof(struct request_part *));
    if (!http->request_parts)
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts_len = 0;
    http->request_parts_len_done = 0;
    header = pico_http_client_build_post_header(http->urikey, post_data_len, connection_type, content_type, cache_control);
    if (!header)
    {
        request_parts_destroy(http);
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts[http->request_parts_len] = request_part_create(header, strlen(header), HTTP_NO_COPY_TO_HEAP, HTTP_NO_USER_MEM);
    if (!http->request_parts[http->request_parts_len])
    {
        request_parts_destroy(http);
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts_len += 1;

    http->request_parts[http->request_parts_len] = request_part_create((char *)post_data, post_data_len, HTTP_NO_COPY_TO_HEAP, HTTP_USER_MEM);
    if (!http->request_parts[http->request_parts_len])
    {
        request_parts_destroy(http);
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts_len += 1;

    bytes_written = socket_write_request_parts(http);
    if (bytes_written < 0)
    {
        return HTTP_RETURN_ERROR;
    }
    return HTTP_RETURN_OK;
}

/*
 * API for sending a raw request.
 * User should not FREE the request until it has been written to the http_socket.
 * Callback will indicate this.
 */
int8_t MOCKABLE pico_http_client_send_raw(uint16_t conn, char *request)
{
    struct pico_http_client search = {
        .connectionID = conn
    };
    struct pico_http_client *http = pico_tree_findKey(&pico_client_list, &search);
    int32_t bytes_written = 0;

    dbg("RAW request\n");

    if (!http)
    {
        dbg("Client not found!\n");
        return HTTP_RETURN_ERROR;
    }
    if (!request)
    {
        dbg("Request is empty!\n");
        return HTTP_RETURN_ERROR;
    }

    if (http->state != HTTP_CONN_IDLE)
    {
        return HTTP_RETURN_CONN_BUSY;
    }

    http->request_parts = PICO_ZALLOC(1 * sizeof(struct request_part *));
    if (!http->request_parts)
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts_len = 0;
    http->request_parts_len_done = 0;
    http->request_parts[http->request_parts_len] = request_part_create(request, strlen(request), HTTP_NO_COPY_TO_HEAP, HTTP_USER_MEM);

    if (!http->request_parts[http->request_parts_len])
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }

    http->request_parts_len += 1;
    bytes_written = socket_write_request_parts(http);
    if (bytes_written < 0)
    {
        return HTTP_RETURN_ERROR;
    }
    return HTTP_RETURN_OK;
}

/*
 * API for sending a long polling GET request to the client.
 * Can be used in combination with keep-alive.
 * The library will build the request
 * based on the uri passed when opening the client.
 */
int8_t MOCKABLE pico_http_client_long_poll_send_get(uint16_t conn, char *resource, uint8_t connection_type)
{
    struct pico_http_client search = {
        .connectionID = conn
    };
    struct pico_http_client *client = pico_tree_findKey(&pico_client_list, &search);
    if (!client)
    {
        dbg("pico_http_client_long_poll_send_get | Client not found!\n");
        return HTTP_RETURN_ERROR;
    }
    if (connection_type == HTTP_CONN_CLOSE)
    {
        client->long_polling_state = HTTP_LONG_POLL_CONN_CLOSE;
    }
    else if (connection_type == HTTP_CONN_KEEP_ALIVE)
    {
        client->long_polling_state = HTTP_LONG_POLL_CONN_KEEP_ALIVE;
    }
    else
    {
        return HTTP_RETURN_ERROR;
    }
    return pico_http_client_send_get(conn, resource, connection_type);
}

/*
 * API to cancel a long polling GET request.
 */
int8_t MOCKABLE pico_http_client_long_poll_cancel(uint16_t conn)
{
    struct pico_http_client search = {
        .connectionID = conn
    };
    struct pico_http_client *client = pico_tree_findKey(&pico_client_list, &search);
    if (!client)
    {
        dbg("pico_http_client_long_poll_cancel | Client not found! \n");
        return HTTP_RETURN_ERROR;
    }
    client->long_polling_state = 0;
    return pico_http_client_close(conn);
}


/*
 * API for sending a GET request to a HTTP-server.
 *
 * The library will build the request
 * based on the resource and the hostname that was passed on opening the connection to the HTTP-server.
 */
int8_t MOCKABLE pico_http_client_send_get(uint16_t conn, char *resource, uint8_t connection_type)
{
    char *request = NULL;
    struct pico_http_client search = {
        .connectionID = conn
    };
    struct pico_http_client *http = pico_tree_findKey(&pico_client_list, &search);
    int32_t bytes_written = 0;

    if (!http)
    {
        dbg("Client not found !\n");
        return HTTP_RETURN_ERROR;
    }

    if (resource)
    {
        if (pico_process_resource(resource, http->urikey) < 0)
        {
            return HTTP_RETURN_ERROR;
        }
    }

    if (http->state != HTTP_CONN_IDLE)
    {
        return HTTP_RETURN_CONN_BUSY;
    }

    if (connection_type != HTTP_CONN_CLOSE && connection_type != HTTP_CONN_KEEP_ALIVE)
    {
        return HTTP_RETURN_ERROR;
    }

    http->request_parts = PICO_ZALLOC(1 * sizeof(struct request_part *));
    if (!http->request_parts)
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts_len = 0;
    http->request_parts_len_done = 0;

    request = pico_http_client_build_get(http->urikey, connection_type);
    if (!request)
    {
        request_parts_destroy(http);
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    dbg("GET HEADER: %s\n", request);
    http->request_parts[http->request_parts_len] = request_part_create(request, strlen(request), HTTP_NO_COPY_TO_HEAP, HTTP_NO_USER_MEM);
    if (!http->request_parts[http->request_parts_len])
    {
        request_parts_destroy(http);
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }
    http->request_parts_len += 1;
    bytes_written = socket_write_request_parts(http);
    if (bytes_written < 0)
    {
        return HTTP_RETURN_ERROR;
    }
    http->connection_type = connection_type;
    return HTTP_RETURN_OK;
}
/* / */

static inline int check_chunk_line(struct pico_http_client *client, int tmp_len_read)
{
    if (read_chunk_line(client) == HTTP_RETURN_ERROR)
    {
        dbg("Probably the chunk is malformed or parsed wrong...\n");
        client->wakeup(EV_HTTP_ERROR, client->connectionID);
        return HTTP_RETURN_ERROR;
    }

    if (client->state != HTTP_READING_BODY || !tmp_len_read)
    {
        return 0; /* force out */
    }
    return 1;
}

static inline void update_content_length(struct pico_http_client *client, uint32_t tmp_len_read )
{
    if (tmp_len_read > 0)
    {
        client->header->content_length_or_chunk = client->header->content_length_or_chunk - (uint32_t)tmp_len_read;
    }
}

static inline int32_t read_body(struct pico_http_client *client, uint8_t *data, uint16_t size, uint32_t *len_read, uint32_t *tmp_len_read)
{
    *tmp_len_read = 0;

    if (client->state == HTTP_READING_BODY)
    {

        /* if needed truncate the data */
        *tmp_len_read = pico_socket_read(client->sck, data + (*len_read),
                                       (client->header->content_length_or_chunk < ((uint32_t)(size - (*len_read)))) ? ((uint32_t)client->header->content_length_or_chunk) : (size - (*len_read)));

        update_content_length(client, *tmp_len_read);
        if (*tmp_len_read < 0)
        {
            /* error on reading */
            dbg(">>> Error returned pico_socket_read\n");
            pico_err = PICO_ERR_EBUSY;
            /* return how much data was read until now */
            return (*len_read);
        }
    }

    *len_read += *tmp_len_read;
    return 0;
}

static inline uint32_t read_big_chunk(struct pico_http_client *client, uint8_t *data, uint16_t size, uint32_t *len_read)
{
    uint32_t value;
    /* check if we need more than one chunk */
    if (size >= client->header->content_length_or_chunk)
    {
        /* read the rest of the chunk, if chunk is done, proceed to the next chunk */
        while ((uint16_t)(*len_read) <= size)
        {
            uint32_t tmp_len_read = 0;
            if (read_body(client, data, size, len_read, &tmp_len_read))
            {
                return (*len_read);
            }
            if ((value = check_chunk_line(client, tmp_len_read)) <= 0)
            {
                return value;
            }
        }
    }
    return 0;
}

static inline void read_small_chunk(struct pico_http_client *client, uint8_t *data, uint16_t size, uint32_t *len_read)
{
    if (size < client->header->content_length_or_chunk)
    {
        /* read the data from the chunk */
        *len_read = pico_socket_read(client->sck, (void *)data, size);

        if (*len_read)
        {
            client->header->content_length_or_chunk = client->header->content_length_or_chunk - (uint32_t)(*len_read);
        }
    }
}
static inline int32_t read_chunked_data(struct pico_http_client *client, unsigned char *data, uint16_t size)
{
    uint32_t len_read = 0;
    int32_t value;
    // read the chunk line
    if (read_chunk_line(client) == HTTP_RETURN_ERROR)
    {
        dbg("Probably the chunk is malformed or parsed wrong...\n");
        client->wakeup(EV_HTTP_ERROR, client->connectionID);
        return HTTP_RETURN_ERROR;
    }

    // nothing to read, no use to try
    if (client->state != HTTP_READING_BODY)
    {
        pico_err = PICO_ERR_EAGAIN;
        return HTTP_RETURN_OK;
    }

    read_small_chunk(client, data, size, &len_read);
    value = read_big_chunk(client, data, size, &len_read);
    if (value)
    {
        return value;
    }
    return len_read;
}

/*
 * API for reading received body.
 *
 * This api hides from the user if the transfer-encoding
 * was chunked or a full length was provided, in case of
 * a chunked transfer encoding will "de-chunk" the data
 * and pass it to the user.
 * Body_read_done will be set to 1 if the body has been read completly.
 */
int32_t MOCKABLE pico_http_client_read_body(uint16_t conn, unsigned char *data, uint16_t size, uint8_t *body_read_done)
{
    uint32_t bytes_read = 0;
    struct pico_http_client dummy = {
        .connectionID = conn
    };
    struct pico_http_client *client = pico_tree_findKey(&pico_client_list, &dummy);

    if (!client)
    {
        dbg("Wrong connection id !\n");
        pico_err = PICO_ERR_EINVAL;
        return HTTP_RETURN_ERROR;
    }
    if (client->header->transfer_coding == HTTP_TRANSFER_FULL)
    {
        //check to make sure we don't read more than the header told us, content-length
        if ((client->header->content_length_or_chunk - client->body_read) < size)
        {
            size = client->header->content_length_or_chunk;
            dbg("client->header->content_length_or_chunk: %d\n", client->header->content_length_or_chunk);
        }
        bytes_read = pico_socket_read(client->sck, (void *)data, size);
        client->body_read += bytes_read;
        if (client->header->content_length_or_chunk == client->body_read)
        {
            client->body_read_done = 1;
        }
    }
    else
    {
        /*
         * client->state will be set to HTTP_READ_BODY_DONE if we reach the
         * ending '0' at the end of the body, read_chunked_data make sure
         * we don't read to mutch.
         */
        client->body_read += bytes_read;
        bytes_read = read_chunked_data(client, data, size);
    }

    if (client->body_read_done)
    {
        dbg("Body read finished! %d\n", client->body_read);
        client->state = HTTP_CONN_IDLE;
        client->body_read = 0;
        //client->body_read_done = 0;
        *body_read_done = 1;
        if (client->long_polling_state)
        {
            treat_long_polling(client, 0);
        }
        if (client->connection_type == HTTP_CONN_CLOSE)
        {
        	//client->wakeup(EV_HTTP_CLOSE, client->connectionID);
        	//pico_http_client_close(client->connectionID);
        }
    }
    return bytes_read;
}

/*
 * API for reading received data.
 *
 * Reads out the header struct received from server.
 */
struct pico_http_header * MOCKABLE pico_http_client_read_header(uint16_t conn)
{
    struct pico_http_client dummy = {
        .connectionID = conn
    };
    struct pico_http_client *client = pico_tree_findKey(&pico_client_list, &dummy);

    if (client)
    {
        return client->header;
    }
    else
    {
        /* not found */
        dbg("Wrong connection id !\n");
        pico_err = PICO_ERR_EINVAL;
        return NULL;
    }
}

/*
 * API for reading received data.
 *
 * Reads out the uri struct after was processed.
 */
struct pico_http_uri *pico_http_client_read_uri_data(uint16_t conn)
{
    struct pico_http_client dummy = {
        .connectionID = conn
    };
    struct pico_http_client *client = pico_tree_findKey(&pico_client_list, &dummy);
    /*  */
    if (client)
    {
        return client->urikey;
    }
    else
    {
        /* not found */
        dbg("Wrong connection id !\n");
        pico_err = PICO_ERR_EINVAL;
        return NULL;
    }
}

static void free_header(struct pico_http_client *to_be_removed)
{
    if (to_be_removed->header)
    {
        /* free space used */
        if (to_be_removed->header->location)
        {
            PICO_FREE(to_be_removed->header->location);
        }
        PICO_FREE(to_be_removed->header);
    }
}

static int8_t free_uri(struct pico_http_client *to_be_removed)
{
    if (!to_be_removed)
    {
        return HTTP_RETURN_ERROR;
    }

    if (to_be_removed->urikey)
    {
        if (to_be_removed->urikey->host)
        {
            PICO_FREE(to_be_removed->urikey->host);
        }
        if (to_be_removed->urikey->resource)
        {
            PICO_FREE(to_be_removed->urikey->resource);
        }
        if (to_be_removed->urikey->raw_uri)
        {
            PICO_FREE(to_be_removed->urikey->raw_uri);
        }
        if (to_be_removed->urikey->user_pass)
	    {
		   PICO_FREE(to_be_removed->urikey->user_pass);
	    }
        PICO_FREE(to_be_removed->urikey);
    }

    return HTTP_RETURN_OK;

}

int8_t MOCKABLE pico_http_client_close(uint16_t conn)
{
    struct pico_http_client *to_be_removed = NULL;
    struct pico_http_client dummy = {
        0
    };
    dummy.connectionID = conn;

    dbg("Closing the client %d...\n", conn);
    to_be_removed = pico_tree_delete(&pico_client_list, &dummy);
    if (!to_be_removed)
    {
        dbg("Warning ! Element not found...");
        return HTTP_RETURN_ERROR;
    }

    /* close socket */
    if (to_be_removed->sck)
    {
        pico_socket_close(to_be_removed->sck);
    }
    free_header(to_be_removed);
    free_uri(to_be_removed);

    PICO_FREE(to_be_removed);

    return 0;
}

static inline void read_first_line(struct pico_http_client *client, uint8_t *line, uint32_t *index)
{
    uint8_t c;
    /* read the first line of the header */
    while (consume_char(c) > 0 && c != '\r')
    {
        if (*index < HTTP_HEADER_LINE_SIZE) /* truncate if too long */
        {
            line[(*index)++] = c;
        }
    }
    consume_char(c); /* consume \n */
}

static inline void start_reading_body(struct pico_http_client *client, struct pico_http_header *header)
{

    if (header->transfer_coding == HTTP_TRANSFER_CHUNKED)
    {
        /* read the first chunk */
        header->content_length_or_chunk = 0;

        client->state = HTTP_READING_CHUNK_VALUE;
        read_chunk_line(client);
    }
    else
        client->state = HTTP_READING_BODY;
}

static inline int32_t parse_loc_and_cont(struct pico_http_client *client, struct pico_http_header *header, uint8_t *line, uint32_t *index)
{
    uint8_t c;
    /* Location: */
    if (is_location(line))
    {
        *index = 0;
        while (consume_char(c) > 0 && c != '\r')
        {
            dbg("parse_location: %c\n", c);
            line[(*index)++] = c;
        }
        /* allocate space for the field */
        header->location = PICO_ZALLOC((*index) + 1u);
        if (header->location)
        {
            memcpy(header->location, line, (*index));
            return 1;
        }
        else
        {
            return -1;
        }
    }    /* Content-Length: */
    else if (is_content_length(line))
    {
        header->content_length_or_chunk = 0u;
        header->transfer_coding = HTTP_TRANSFER_FULL;
        /* consume the first space */
        consume_char(c);
        while (consume_char(c) > 0 && c != '\r')
        {
            dbg("parse_content_length: %c\n", c);
            header->content_length_or_chunk = header->content_length_or_chunk * 10u + (uint32_t)(c - '0');
        }
        return 1;
    }    /* Transfer-Encoding: chunked */

    return 0;
}

static inline int32_t parse_transfer_encoding(struct pico_http_client *client, struct pico_http_header *header, uint8_t *line, uint32_t *index)
{
    uint8_t c;

    if (is_transfer_encoding(line))
    {
        (*index) = 0;
        while (consume_char(c) > 0 && c != '\r')
        {
            line[(*index)++] = c;
        }
        if (is_chunked(line))
        {
            header->content_length_or_chunk = 0u;
            header->transfer_coding = HTTP_TRANSFER_CHUNKED;
        }

        return 1;
    } /* just ignore the line */

    return 0;
}


static inline int32_t parse_fields(struct pico_http_client *client, struct pico_http_header *header, uint8_t *line, uint32_t *index)
{
    int8_t c;
    int32_t ret_val;

    ret_val = parse_loc_and_cont(client, header, line, index);
    if (ret_val == 0)
    {
        if (!parse_transfer_encoding(client, header, line, index))
        {
            while (consume_char(c) > 0 && c != '\r') nop();
        }
    }
    else if (ret_val == -1)
    {
        return -1;
    }

    /* consume the next one */
    consume_char(c);
    /* reset the index */
    (*index) = 0u;

    return 0;
}

static inline int32_t parse_rest_of_header(struct pico_http_client *client, struct pico_http_header *header, uint8_t *line, uint32_t *index)
{
    uint8_t c;
    uint32_t read_len = 0;
    (*index) = 0u;

    /* parse the rest of the header */
    read_len = consume_char(c);
    if (read_len == 0)
        return HTTP_RETURN_BUSY;
    dbg("parse_rest_of_header: client %p, header %p, line %p, index: %d\n", client, header, line, *index);
    while (read_len > 0)
    {
        if (c == ':')
        {
            if (parse_fields(client, header, line, index) == -1)
                return HTTP_RETURN_ERROR;
        }
        else if (c == '\r' && !(*index))
        {
            /* consume the \n */
            consume_char(c);
            break;
        }
        else
        {
            line[(*index)++] = c;
        }
        read_len = consume_char(c);
    }
    return HTTP_RETURN_OK;
}

static int8_t parse_header_from_server(struct pico_http_client *client, struct pico_http_header *header)
{
    uint8_t line[HTTP_HEADER_LINE_SIZE];
    uint32_t index = 0;
    dbg("Parse header from server\n");
    if (client->state == HTTP_START_READING_HEADER)
    {
        read_first_line(client, line, &index);
        /* check the integrity of the response */
        /* make sure we have enough characters to include the response code */
        /* make sure the server response starts with HTTP/1. */
        if ((index < RESPONSE_INDEX + 2u) || is_not_HTTPv1(line))
        {
            /* wrong format of the the response */
            pico_err = PICO_ERR_EINVAL;
            return HTTP_RETURN_ERROR;
        }

        /* extract response code */
        header->response_code = (uint16_t)((line[RESPONSE_INDEX] - '0') * 100 +
                                          (line[RESPONSE_INDEX + 1] - '0') * 10 +
                                          (line[RESPONSE_INDEX + 2] - '0'));

        /*if (header->response_code == HTTP_NOT_FOUND)
        {
            return HTTP_RETURN_NOT_FOUND;
        }
        else if (header->response_code >= HTTP_INTERNAL_SERVER_ERR)
        {
            // invalid response type
            header->response_code = 0;
            return HTTP_RETURN_ERROR;
        }*/
    }

    dbg("Server response : %d \n", header->response_code);

    if (parse_rest_of_header(client, header, line, &index) == HTTP_RETURN_BUSY)
        return HTTP_RETURN_BUSY;
    start_reading_body(client, header);
    dbg("End of header\n");
    return HTTP_RETURN_OK;

}

/* an async read of the chunk part, since in theory a chunk can be split in 2 packets */
static inline void set_client_chunk_state(struct pico_http_client *client)
{

    if (client->header->content_length_or_chunk == 0 && client->state == HTTP_READING_BODY)
    {
        client->state = HTTP_READING_CHUNK_VALUE;
    }
}
static inline void read_chunk_trail(struct pico_http_client *client)
{
    uint8_t c;

    if (client->state == HTTP_READING_CHUNK_TRAIL)
    {

        while (consume_char(c) > 0 && c != '\n')
        {
            nop();
        }
        if (c == '\n')
        {
            client->state = HTTP_READING_BODY;
        }
    }
}
static inline void read_chunk_value(struct pico_http_client *client)
{
    uint8_t c;

    while (consume_char(c) > 0 && c != '\r' && c != ';')
    {
        dbg("c: %c\n", c);
        if (is_hex_digit(c))
        {
            client->header->content_length_or_chunk = (client->header->content_length_or_chunk << 4u) + (uint32_t)hex_digit_to_dec(c);
        }
        if (c == '0' && client->header->content_length_or_chunk == 0)
        {
            dbg("End of chunked data\n");
            client->body_read_done = 1;
        }
    }

    if (c == '\r' || c == ';')
    {
        client->state = HTTP_READING_CHUNK_TRAIL;
    }
}

static int8_t read_chunk_line(struct pico_http_client *client)
{
    set_client_chunk_state(client);

    if (client->state == HTTP_READING_CHUNK_VALUE)
    {
        read_chunk_value(client);
    }

    read_chunk_trail(client);

    return HTTP_RETURN_OK;
}

int8_t pico_http_set_close_ev(uint16_t conn)
{
	struct pico_http_client dummy = {
		.connectionID = conn
	};
	struct pico_http_client *client = pico_tree_findKey(&pico_client_list, &dummy);

	if (!client)
	{
		dbg("Wrong connection id !\n");
		pico_err = PICO_ERR_EINVAL;
		return HTTP_RETURN_ERROR;
	}
	client->wakeup(EV_HTTP_CLOSE, client->connectionID);
    	return EV_HTTP_CLOSE;
}





