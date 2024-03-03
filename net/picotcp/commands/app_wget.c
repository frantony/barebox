#include <common.h>
#include <command.h>

#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_socket.h>
#include <pico_http_client.h>

/* https://github.com/tass-belgium/picotcp-modules/blob/master/docs/user_manual/chap_examples.tex */
static void wget_callback(uint16_t ev, uint16_t conn)
{
	char data[1024 * 1024]; // MAX: 1M
	static int _length = 0;
	int32_t ret = 0;

	if (ev & EV_HTTP_CON) {
		printf(">>> Connected to the client \n");
		/* you can let the client use the default generated header
		or you can create you own string header and send it
		via pico_http_client_send_raw(...)*/
		pico_http_client_send_get(conn, NULL, HTTP_CONN_CLOSE);
	}

	if (ev & EV_HTTP_WRITE_SUCCESS) {
		printf("The request write was successful ... \n");
	}

	if (ev & EV_HTTP_WRITE_PROGRESS_MADE) {
		uint32_t total_bytes_written = 0;
		uint32_t total_bytes_to_write = 0;

		printf("The request write has made progress ... \n");
		ret = pico_http_client_get_write_progress(conn, &total_bytes_written,
								&total_bytes_to_write);
		if (ret < 0) {
			printf("Nothing to write anymore\n");
		} else {
			printf("Total_bytes_written: %d, Total_bytes_to_write: %d\n",
						total_bytes_written, total_bytes_to_write);
		}
	}

	if (ev & EV_HTTP_WRITE_FAILED) {
		printf("The request write has failed ... \n");
	}

	if (ev & EV_HTTP_REQ) {
		struct pico_http_header * header = pico_http_client_read_header(conn);

		printf("Received header from server...\n");
		printf("Server response : %d\n", header->response_code);
		printf("Location : %s\n", header->location);
		printf("Transfer-Encoding : %d\n", header->transfer_coding);
		printf("Size/Chunk : %d\n", header->content_length_or_chunk);
	}

	if (ev & EV_HTTP_BODY) {
		uint32_t len;
		uint8_t body_read_done = 0;

		printf("Reading data...\n");
		/*
		Data is passed to you without you worrying if the transfer is
		chunked or the content-length was specified.
		*/
		while((len = pico_http_client_read_body(conn, data + _length, 1024,
							&body_read_done))) {
			_length += len;
		}
	}

	if (ev & EV_HTTP_CLOSE) {
		struct pico_http_header * header = pico_http_client_read_header(conn);
		int32_t len;
		uint8_t body_read_done = 0;

		printf("Connection was closed...\n");
		printf("Reading remaining data, if any ...\n");

		while ((len = pico_http_client_read_body(conn, data, 1000u,
							&body_read_done)) && len > 0) {
			_length += len;
		}
		printf("Read a total data of : %d bytes \n", _length);

		if (header->transfer_coding == HTTP_TRANSFER_CHUNKED) {
			if (header->content_length_or_chunk) {
				printf("Last chunk data not fully read !\n");
				//exit(1);
				return;
			} else {
				printf("Transfer ended with a zero chunk! OK !\n");
			}
		} else {
			if (header->content_length_or_chunk == _length) {
				printf("Received the full : %d \n", _length);
				memory_display(data, 0, _length, 1, 0);
			} else {
				printf("Received %d , waiting for %d\n", _length,
					header->content_length_or_chunk);
				//exit(1);
				return;
			}
		}

		pico_http_client_close(conn);

		return;
	}

	if (ev & EV_HTTP_ERROR) {
		printf("Connection error (probably dns failed : check the routing table), trying to close the connection\n");
		pico_http_client_close(conn);
		return;
	}

	if (ev & EV_HTTP_DNS) {
		printf("The DNS query was successful ... \n");
	}
}

static void app_wget(char * uri)
{
	if (!uri) {
		pr_err(" wget expects the uri to be received\n");
		return;
	}

	if (pico_http_client_open(uri, wget_callback) < 0) {
		pr_err(" error opening the uri : %s, please check the format\n", uri);
		return;
	}
}

static int do_app_wget(int argc, char *argv[])
{
	pr_err("http://64.233.161.105/index.html\n");
	pr_err("PING www.google.com (64.233.161.105) 56(84) bytes of data.\n");

	app_wget(argv[1]);

	return 0;
}

BAREBOX_CMD_START(app_wget)
	.cmd		= do_app_wget,
BAREBOX_CMD_END
