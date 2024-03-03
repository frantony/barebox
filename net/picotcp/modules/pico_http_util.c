/*********************************************************************
   PicoTCP. Copyright (c) 2012 TASS Belgium NV. Some rights reserved.
   See LICENSE and COPYING for usage.

   Author: Andrei Carp <andrei.carp@tass.be>
 *********************************************************************/

#include <stdint.h>
#include "pico_config.h"
#include "pico_stack.h"
#include "pico_protocol.h"
#include "pico_http_util.h"

struct pico_mime_map supported_mime_types[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".shtm", "text/html"},
    {".shtml", "text/html"},
    {".css", "text/css"},
    {".js", "application/x-javascript"},
    {".ico", "image/x-icon"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".svg", "image/svg+xml"},
    {".txt", "text/plain"},
    {".torrent", "application/x-bittorrent"},
    {".wav", "audio/x-wav"},
    {".mp3", "audio/x-mp3"},
    {".mid", "audio/mid"},
    {".m3u", "audio/x-mpegurl"},
    {".ogg", "application/ogg"},
    {".ram", "audio/x-pn-realaudio"},
    {".xml", "text/xml"},
    {".json", "application/json"},
    {".xslt", "application/xml"},
    {".xsl", "application/xml"},
    {".ra", "audio/x-pn-realaudio"},
    {".doc", "application/msword"},
    {".exe", "application/octet-stream"},
    {".zip", "application/x-zip-compressed"},
    {".xls", "application/excel"},
    {".tgz", "application/x-tar-gz"},
    {".tar", "application/x-tar"},
    {".gz", "application/x-gunzip"},
    {".arj", "application/x-arj-compressed"},
    {".rar", "application/x-rar-compressed"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".swf", "application/x-shockwave-flash"},
    {".mpg", "video/mpeg"},
    {".webm", "video/webm"},
    {".mpeg", "video/mpeg"},
    {".mov", "video/quicktime"},
    {".mp4", "video/mp4"},
    {".m4v", "video/x-m4v"},
    {".asf", "video/x-ms-asf"},
    {".avi", "video/x-msvideo"},
    {".bmp", "image/bmp"},
    {".ttf", "application/x-font-ttf"}
};

int pico_itoaHex(uint16_t port, char *ptr)
{
    int size = 0;
    int index;

    /* transform to from number to string [ in backwards ] */
    while(port)
    {
        ptr[size] = (char)(((port & 0xF) < 10) ? ((port & 0xF) + '0') : ((port & 0xF) - 10 + 'a'));
        port = port >> 4u; /* divide by 16 */
        size++;
    }
    /* invert positions */
    for(index = 0; index < (size / 2); index++)
    {
        char c = ptr[index];
        ptr[index] = ptr[size - index - 1];
        ptr[size - index - 1] = c;
    }
    ptr[size] = '\0';
    return size;
}

uint32_t pico_itoa(uint32_t port, char *ptr)
{
    uint32_t size = 0;
    uint32_t index;

    /* transform to from number to string [ in backwards ] */
    while(port)
    {
        ptr[size] = (char)(port % 10 + '0');
        port = port / 10;
        size++;
    }
    /* invert positions */
    for(index = 0; index < (size >> 1u); index++)
    {
        char c = ptr[index];
        ptr[index] = ptr[size - index - 1];
        ptr[size - index - 1] = c;
    }
    ptr[size] = '\0';
    return size;
}

/*
 * The function decodes a percent-encoded url (src).
 * The result is saved to dst.
 */
void pico_http_url_decode(char *dst, const char *src)
{
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (pico_is_hex(a) && pico_is_hex(b)))
        {
            if (a >= 'a')
                a = (char)(a - 'a' - 'A');

            if (a >= 'A')
                a = (char)(a - 'A' - 10);
            else
                a = (char)(a - '0');

            if (b >= 'a')
                b = (char)(b - 'a' - 'A');

            if (b >= 'A')
                b = (char)(b - ('A' - 10));
            else
                b = (char)(b - '0');

            *dst++ = (char)(16 * a + b);
            src += 3;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}
/*
    Function for guessing the mimetype based on the last part of the filename supplied (the file extension).
    If no good guess can be made (none of the supported extensions is found as a substring of the filename), NULL is returned. Otherwise the MIME-type string is returned.
*/
const char* pico_http_get_mimetype(char* resourcename)
{
    int loopcounter = 0;
    for(loopcounter = 0; loopcounter < sizeof(supported_mime_types) / sizeof(struct pico_mime_map); loopcounter++)
    {
        struct pico_mime_map mime_map_element = supported_mime_types[loopcounter];
        if ( strstr(resourcename, mime_map_element.extension) != NULL )
        {
            return mime_map_element.mimetype;
        }
    }
    return NULL;
}
