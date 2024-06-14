// SPDX-License-Identifier: GPL-2.0-only
/*
 *  linux/lib/string.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * stupid library routines.. The optimized versions should generally be found
 * as inline code in <asm-xx/string.h>
 *
 * These are buggy as well..
 *
 * * Fri Jun 25 1999, Ingo Oeser <ioe@informatik.tu-chemnitz.de>
 * -  Added strsep() which will replace strtok() soon (because strsep() is
 *    reentrant and should be faster). Use only strsep() in new code, please.
 * * Mon Sep 14 2020, Ahmad Fatoum <a.fatoum@pengutronix.de>
 * -  Kissed strtok() goodbye
 *
 */

#include <linux/types.h>
#include <string.h>
#include <linux/ctype.h>
#include <asm/word-at-a-time.h>
#include <malloc.h>

#ifndef __HAVE_ARCH_STRCASECMP
int strcasecmp(const char *s1, const char *s2)
{
	int c1, c2;

	do {
		c1 = tolower(*s1++);
		c2 = tolower(*s2++);
	} while (c1 == c2 && c1 != 0);
	return c1 - c2;
}
EXPORT_SYMBOL(strcasecmp);
#endif

#ifndef __HAVE_ARCH_STRNCASECMP
/**
 * strncasecmp - Case insensitive, length-limited string comparison
 * @s1: One string
 * @s2: The other string
 * @len: the maximum number of characters to compare
 */
int strncasecmp(const char *s1, const char *s2, size_t len)
{
	/* Yes, Virginia, it had better be unsigned */
	unsigned char c1, c2;

	if (!len)
		return 0;

	do {
		c1 = *s1++;
		c2 = *s2++;
		if (!c1 || !c2)
			break;
		if (c1 == c2)
			continue;
		c1 = tolower(c1);
		c2 = tolower(c2);
		if (c1 != c2)
			break;
	} while (--len);
	return (int)c1 - (int)c2;
}
EXPORT_SYMBOL(strncasecmp);
#endif

#ifndef __HAVE_ARCH_STRCPY
/**
 * strcpy - Copy a %NUL terminated string
 * @dest: Where to copy the string to
 * @src: Where to copy the string from
 */
char * strcpy(char * dest,const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return tmp;
}
#endif
EXPORT_SYMBOL(strcpy);

#ifndef __HAVE_ARCH_STRSCPY
ssize_t strscpy(char *dest, const char *src, size_t count)
{
	const struct word_at_a_time constants = WORD_AT_A_TIME_CONSTANTS;
	size_t max = count;
	long res = 0;

	if (count == 0 || WARN_ON_ONCE(count > INT_MAX))
		return -E2BIG;

#ifdef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
	/*
	 * If src is unaligned, don't cross a page boundary,
	 * since we don't know if the next page is mapped.
	 */
	if ((long)src & (sizeof(long) - 1)) {
		size_t limit = PAGE_SIZE - ((long)src & (PAGE_SIZE - 1));
		if (limit < max)
			max = limit;
	}
#else
	/* If src or dest is unaligned, don't do word-at-a-time. */
	if (((long) dest | (long) src) & (sizeof(long) - 1))
		max = 0;
#endif

	/*
	 * read_word_at_a_time() below may read uninitialized bytes after the
	 * trailing zero and use them in comparisons. Disable this optimization
	 * under KMSAN to prevent false positive reports.
	 */
	if (IS_ENABLED(CONFIG_KMSAN))
		max = 0;

	while (max >= sizeof(unsigned long)) {
		unsigned long c, data;

		c = read_word_at_a_time(src+res);
		if (has_zero(c, &data, &constants)) {
			data = prep_zero_mask(c, data, &constants);
			data = create_zero_mask(data);
			*(unsigned long *)(dest+res) = c & zero_bytemask(data);
			return res + find_zero(data);
		}
		*(unsigned long *)(dest+res) = c;
		res += sizeof(unsigned long);
		count -= sizeof(unsigned long);
		max -= sizeof(unsigned long);
	}

	while (count) {
		char c;

		c = src[res];
		dest[res] = c;
		if (!c)
			return res;
		res++;
		count--;
	}

	/* Hit buffer length without finding a NUL; force NUL-termination. */
	if (res)
		dest[res-1] = '\0';

	return -E2BIG;
}
EXPORT_SYMBOL(strscpy);
#endif

/**
 * stpcpy - Copy a %NUL terminated string, but return pointer to %NUL
 * @dest: Where to copy the string to
 * @src: Where to copy the string from
 */
char *stpcpy(char *dest, const char *src)
{
	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return dest - 1;
}
EXPORT_SYMBOL(stpcpy);

#ifndef __HAVE_ARCH_STRNCPY
/**
 * strncpy - Copy a length-limited, %NUL-terminated string
 * @dest: Where to copy the string to
 * @src: Where to copy the string from
 * @count: The maximum number of bytes to copy
 *
 * Note that unlike userspace strncpy, this does not %NUL-pad the buffer.
 * However, the result is not %NUL-terminated if the source exceeds
 * @count bytes.
 */
char * strncpy(char * dest,const char *src,size_t count)
{
	char *tmp = dest;

	while (count-- && (*dest++ = *src++) != '\0')
		/* nothing */;

	return tmp;
}
#endif
EXPORT_SYMBOL(strncpy);

#ifndef __HAVE_ARCH_STRLCPY
/**
 * strlcpy - Copy a %NUL terminated string into a sized buffer
 * @dest: Where to copy the string to
 * @src: Where to copy the string from
 * @size: size of destination buffer
 *
 * Compatible with *BSD: the result is always a valid
 * NUL-terminated string that fits in the buffer (unless,
 * of course, the buffer size is zero). It does not pad
 * out the result like strncpy() does.
 */
size_t strlcpy(char *dest, const char *src, size_t size)
{
	size_t ret = strlen(src);

	if (size) {
		size_t len = (ret >= size) ? size - 1 : ret;
		memcpy(dest, src, len);
		dest[len] = '\0';
	}
	return ret;
}
EXPORT_SYMBOL(strlcpy);
#endif

#ifndef __HAVE_ARCH_STRCAT
/**
 * strcat - Append one %NUL-terminated string to another
 * @dest: The string to be appended to
 * @src: The string to append to it
 */
char * strcat(char * dest, const char * src)
{
	char *tmp = dest;

	while (*dest)
		dest++;
	while ((*dest++ = *src++) != '\0')
		;

	return tmp;
}
#endif
EXPORT_SYMBOL(strcat);

#ifndef __HAVE_ARCH_STRNCAT
/**
 * strncat - Append a length-limited, %NUL-terminated string to another
 * @dest: The string to be appended to
 * @src: The string to append to it
 * @count: The maximum numbers of bytes to copy
 *
 * Note that in contrast to strncpy, strncat ensures the result is
 * terminated.
 */
char * strncat(char *dest, const char *src, size_t count)
{
	char *tmp = dest;

	if (count) {
		while (*dest)
			dest++;
		while ((*dest++ = *src++)) {
			if (--count == 0) {
				*dest = '\0';
				break;
			}
		}
	}

	return tmp;
}
#endif
EXPORT_SYMBOL(strncat);

#ifndef __HAVE_ARCH_STRCMP
/**
 * strcmp - Compare two strings
 * @cs: One string
 * @ct: Another string
 */
int strcmp(const char * cs,const char * ct)
{
	register signed char __res;

	BUG_ON(!cs || !ct);

	while (1) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
	}

	return __res;
}
#endif
EXPORT_SYMBOL(strcmp);

#ifndef __HAVE_ARCH_STRNCMP
/**
 * strncmp - Compare two length-limited strings
 * @cs: One string
 * @ct: Another string
 * @count: The maximum number of bytes to compare
 */
int strncmp(const char * cs, const char * ct, size_t count)
{
	register signed char __res = 0;

	BUG_ON(!cs || !ct);

	while (count) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
		count--;
	}

	return __res;
}
#endif
EXPORT_SYMBOL(strncmp);

#ifndef __HAVE_ARCH_STRCHR
/**
 * strchr - Find the first occurrence of a character in a string
 * @s: The string to be searched
 * @c: The character to search for
 */
char * _strchr(const char * s, int c)
{
	for(; *s != (char) c; ++s)
		if (*s == '\0')
			return NULL;
	return (char *) s;
}
#endif
EXPORT_SYMBOL(_strchr);

#ifndef __HAVE_ARCH_STRCHRNUL
/**
 * strchrnul - Find and return a character in a string, or end of string
 * @s: The string to be searched
 * @c: The character to search for
 *
 * Returns pointer to first occurrence of 'c' in s. If c is not found, then
 * return a pointer to the null byte at the end of s.
 */
char *strchrnul(const char *s, int c)
{
	while (*s && *s != (char)c)
		s++;
	return (char *)s;
}
EXPORT_SYMBOL(strchrnul);
#endif

#ifndef __HAVE_ARCH_STRRCHR
/**
 * strrchr - Find the last occurrence of a character in a string
 * @s: The string to be searched
 * @c: The character to search for
 */
char * _strrchr(const char * s, int c)
{
       const char *p = s + strlen(s);
       do {
	   if (*p == (char)c)
	       return (char *)p;
       } while (--p >= s);
       return NULL;
}
#endif
EXPORT_SYMBOL(_strrchr);

#ifndef __HAVE_ARCH_STRLEN
/**
 * strlen - Find the length of a string
 * @s: The string to be sized
 */
size_t strlen(const char * s)
{
	const char *sc;

	for (sc = s; *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}
#endif
EXPORT_SYMBOL(strlen);

#ifndef __HAVE_ARCH_STRNLEN
/**
 * strnlen - Find the length of a length-limited string
 * @s: The string to be sized
 * @count: The maximum number of bytes to search
 */
size_t strnlen(const char * s, size_t count)
{
	const char *sc;

	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}
#endif
EXPORT_SYMBOL(strnlen);

#ifndef __HAVE_ARCH_STRDUP
char * strdup(const char *s)
{
	char *new;

	if ((s == NULL)	||
	    ((new = malloc (strlen(s) + 1)) == NULL) ) {
		return NULL;
	}

	strcpy (new, s);
	return new;
}
#endif
EXPORT_SYMBOL(strdup);

#ifndef __HAVE_ARCH_STRNDUP
char *strndup(const char *s, size_t n)
{
	char *new;
	size_t len = strnlen(s, n);

	if ((s == NULL) ||
	    ((new = malloc(len + 1)) == NULL)) {
		return NULL;
	}

	memcpy(new, s, len);
	new[len] = '\0';

	return new;
}

#endif
EXPORT_SYMBOL(strndup);

#ifndef __HAVE_ARCH_STRSPN
/**
 * strspn - Calculate the length of the initial substring of @s which only
 * 	contain letters in @accept
 * @s: The string to be searched
 * @accept: The string to search for
 */
size_t strspn(const char *s, const char *accept)
{
	const char *p;
	const char *a;
	size_t count = 0;

	for (p = s; *p != '\0'; ++p) {
		for (a = accept; *a != '\0'; ++a) {
			if (*p == *a)
				break;
		}
		if (*a == '\0')
			return count;
		++count;
	}

	return count;
}
#endif
EXPORT_SYMBOL(strspn);

#ifndef __HAVE_ARCH_STRPBRK
/**
 * strpbrk - Find the first occurrence of a set of characters
 * @cs: The string to be searched
 * @ct: The characters to search for
 */
char * strpbrk(const char * cs,const char * ct)
{
	const char *sc1, *sc2;

	for( sc1 = cs; *sc1 != '\0'; ++sc1) {
		for( sc2 = ct; *sc2 != '\0'; ++sc2) {
			if (*sc1 == *sc2)
				return (char *) sc1;
		}
	}
	return NULL;
}
#endif
EXPORT_SYMBOL(strpbrk);

#ifndef __HAVE_ARCH_STRSEP
/**
 * strsep - Split a string into tokens
 * @s: The string to be searched
 * @ct: The characters to search for
 *
 * strsep() updates @s to point after the token, ready for the next call.
 *
 * It returns empty tokens, too, behaving exactly like the libc function
 * of that name. In fact, it was stolen from glibc2 and de-fancy-fied.
 * Same semantics, slimmer shape. ;)
 */
char * strsep(char **s, const char *ct)
{
	char *sbegin = *s, *end;

	if (sbegin == NULL)
		return NULL;

	end = strpbrk(sbegin, ct);
	if (end)
		*end++ = '\0';
	*s = end;

	return sbegin;
}
#endif
EXPORT_SYMBOL(strsep);

/**
 * strsep_unescaped - Split a string into tokens, while ignoring escaped delimiters
 * @s: The string to be searched
 * @ct: The delimiter characters to search for
 *
 * strsep_unescaped() behaves like strsep unless it meets an escaped delimiter.
 * In that case, it shifts the string back in memory to overwrite the escape's
 * backslash then continues the search until an unescaped delimiter is found.
 */
char *strsep_unescaped(char **s, const char *ct)
{
        char *sbegin = *s, *hay;
        const char *needle;
        size_t shift = 0;

        if (sbegin == NULL)
                return NULL;

        for (hay = sbegin; *hay != '\0'; ++hay) {
                *hay = hay[shift];

                if (*hay == '\\') {
                        *hay = hay[++shift];
                        if (*hay != '\\')
                                continue;
                }

                for (needle = ct; *needle != '\0'; ++needle) {
                        if (*hay == *needle)
                                goto match;
                }
        }

        *s = NULL;
        return sbegin;

match:
        *hay = '\0';
        *s = &hay[shift + 1];

        return sbegin;
}

#ifndef __HAVE_ARCH_STRSWAB
/**
 * strswab - swap adjacent even and odd bytes in %NUL-terminated string
 * s: address of the string
 *
 * returns the address of the swapped string or NULL on error. If
 * string length is odd, last byte is untouched.
 */
char *strswab(const char *s)
{
	char *p, *q;

	if ((NULL == s) || ('\0' == *s)) {
		return (NULL);
	}

	for (p=(char *)s, q=p+1; (*p != '\0') && (*q != '\0'); p+=2, q+=2) {
		char  tmp;

		tmp = *p;
		*p  = *q;
		*q  = tmp;
	}

	return (char *) s;
}
#endif

/**
 * memset - Fill a region of memory with the given value
 * @s: Pointer to the start of the area.
 * @c: The byte to fill the area with
 * @count: The size of the area.
 *
 * Do not use memset() to access IO space, use memset_io() instead.
 */
void *__default_memset(void * s, int c, size_t count)
{
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}
EXPORT_SYMBOL(__default_memset);

void __prereloc __no_sanitize_address *__nokasan_default_memset(void * s, int c, size_t count)
{
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}
EXPORT_SYMBOL(__nokasan_default_memset);

#ifndef __HAVE_ARCH_MEMSET
void *memset(void *s, int c, size_t count) __alias(__default_memset);
void *__memset(void *s, int c, size_t count) __alias(__default_memset);
#endif

/**
 * memcpy - Copy one area of memory to another
 * @dest: Where to copy to
 * @src: Where to copy from
 * @count: The size of the area.
 *
 * You should not use this function to access IO space, use memcpy_toio()
 * or memcpy_fromio() instead.
 */
void *__default_memcpy(void * dest,const void *src, size_t count)
{
	char *tmp = (char *) dest, *s = (char *) src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}
EXPORT_SYMBOL(__default_memcpy);

void __no_sanitize_address *__nokasan_default_memcpy(void * dest,
						     const void *src, size_t count)
{
	char *tmp = (char *) dest, *s = (char *) src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}
EXPORT_SYMBOL(__nokasan_default_memcpy);

#ifndef __HAVE_ARCH_MEMCPY
void *memcpy(void * dest, const void *src, size_t count)
	__alias(__default_memcpy);
void *__memcpy(void * dest, const void *src, size_t count)
	__alias(__default_memcpy);
#endif

void *mempcpy(void *dest, const void *src, size_t count)
{
	return memcpy(dest, src, count) + count;
}
EXPORT_SYMBOL(mempcpy);

#ifndef __HAVE_ARCH_MEMMOVE
/**
 * memmove - Copy one area of memory to another
 * @dest: Where to copy to
 * @src: Where to copy from
 * @count: The size of the area.
 *
 * Unlike memcpy(), memmove() copes with overlapping areas.
 */
void * memmove(void * dest,const void *src,size_t count)
{
	char *tmp, *s;

	if (dest <= src) {
		tmp = (char *) dest;
		s = (char *) src;
		while (count--)
			*tmp++ = *s++;
		}
	else {
		tmp = (char *) dest + count;
		s = (char *) src + count;
		while (count--)
			*--tmp = *--s;
		}

	return dest;
}
#endif
EXPORT_SYMBOL(memmove);

#ifndef __HAVE_ARCH_MEMCMP
/**
 * memcmp - Compare two areas of memory
 * @cs: One area of memory
 * @ct: Another area of memory
 * @count: The size of the area.
 */
int memcmp(const void * cs,const void * ct,size_t count)
{
	const unsigned char *su1, *su2;
	int res = 0;

	for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}
#endif
EXPORT_SYMBOL(memcmp);

#ifndef __HAVE_ARCH_MEMSCAN
/**
 * memscan - Find a character in an area of memory.
 * @addr: The memory area
 * @c: The byte to search for
 * @size: The size of the area.
 *
 * returns the address of the first occurrence of @c, or 1 byte past
 * the area if @c is not found
 */
void * memscan(void * addr, int c, size_t size)
{
	unsigned char * p = (unsigned char *) addr;

	while (size) {
		if (*p == c)
			return (void *) p;
		p++;
		size--;
	}
	return (void *) p;
}
#endif
EXPORT_SYMBOL(memscan);

#ifndef __HAVE_ARCH_STRSTR
/**
 * strstr - Find the first substring in a %NUL terminated string
 * @s1: The string to be searched
 * @s2: The string to search for
 */
char * _strstr(const char * s1,const char * s2)
{
	int l1, l2;

	l2 = strlen(s2);
	if (!l2)
		return (char *) s1;
	l1 = strlen(s1);
	while (l1 >= l2) {
		l1--;
		if (!memcmp(s1,s2,l2))
			return (char *) s1;
		s1++;
	}
	return NULL;
}
#endif
EXPORT_SYMBOL(_strstr);

#ifndef __HAVE_ARCH_MEMCHR
/**
 * memchr - Find a character in an area of memory.
 * @s: The memory area
 * @c: The byte to search for
 * @n: The size of the area.
 *
 * returns the address of the first occurrence of @c, or %NULL
 * if @c is not found
 */
void *memchr(const void *s, int c, size_t n)
{
	const unsigned char *p = s;
	while (n-- != 0) {
		if ((unsigned char)c == *p++) {
			return (void *)(p-1);
		}
	}
	return NULL;
}

#endif
EXPORT_SYMBOL(memchr);

/**
 * skip_spaces - Removes leading whitespace from @str.
 * @str: The string to be stripped.
 *
 * Returns a pointer to the first non-whitespace character in @str.
 */
char *skip_spaces(const char *str)
{
	while (isspace(*str))
		++str;
	return (char *)str;
}

/**
 * strim - Removes trailing whitespace from @s.
 * @s: The string to be stripped.
 *
 * Note that the first trailing whitespace is replaced with a %NUL-terminator
 * in the given string @s. Returns a pointer to the first non-whitespace
 * character in @s.
 */
char *strim(char *s)
{
	size_t size;
	char *end;

	s = skip_spaces(s);
	size = strlen(s);
	if (!size)
		return s;

	end = s + size - 1;
	while (end >= s && isspace(*end))
		end--;
	*(end + 1) = '\0';

	return s;
}
EXPORT_SYMBOL(strim);

static void *check_bytes8(const u8 *start, u8 value, unsigned int bytes)
{
	while (bytes) {
		if (*start != value)
			return (void *)start;
		start++;
		bytes--;
	}
	return NULL;
}

/**
 * memchr_inv - Find an unmatching character in an area of memory.
 * @start: The memory area
 * @c: Find a character other than c
 * @bytes: The size of the area.
 *
 * returns the address of the first character other than @c, or %NULL
 * if the whole buffer contains just @c.
 */
void *memchr_inv(const void *start, int c, size_t bytes)
{
	u8 value = c;
	u64 value64;
	unsigned int words, prefix;

	if (bytes <= 16)
		return check_bytes8(start, value, bytes);

	value64 = value;
	value64 |= value64 << 8;
	value64 |= value64 << 16;
	value64 |= value64 << 32;

	prefix = (unsigned long)start % 8;
	if (prefix) {
		u8 *r;

		prefix = 8 - prefix;
		r = check_bytes8(start, value, prefix);
		if (r)
			return r;
		start += prefix;
		bytes -= prefix;
	}

	words = bytes / 8;

	while (words) {
		if (*(u64 *)start != value64)
			return check_bytes8(start, value, 8);
		start += 8;
		words--;
	}

	return check_bytes8(start, value, bytes % 8);
}
EXPORT_SYMBOL(memchr_inv);

void *memdup(const void *orig, size_t size)
{
	void *buf;

	buf = malloc(size);
	if (!buf)
		return NULL;

	memcpy(buf, orig, size);

	return buf;
}
EXPORT_SYMBOL(memdup);

/**
 * strtobool - convert a string to a boolean value
 * @str - The string
 * @val - The boolean value returned.
 *
 * This function treats
 * - any positive (nonzero) number as true
 * - "0" as false
 * - "true" (case insensitive) as true
 * - "false" (case insensitive) as false
 *
 * Every other value results in an error and the @val is not
 * modified. The caller is expected to initialize @val with the
 * correct default before calling strtobool.
 *
 * Returns 0 for success or negative error code if the variable does
 * not exist or contains something this function does not recognize
 * as true or false.
 */
int strtobool(const char *str, int *val)
{
	if (!str || !*str)
		return -EINVAL;

	if (simple_strtoul(str, NULL, 0) > 0) {
		*val = true;
		return 0;
	}

	if (!strcmp(str, "0") || !strcasecmp(str, "false")) {
		*val = false;
		return 0;
	}

	if (!strcasecmp(str, "true")) {
		*val = true;
		return 0;
	}

	return -EINVAL;
}
EXPORT_SYMBOL(strtobool);

bool strends(const char *str, const char *postfix)
{
	if (strlen(str) < strlen(postfix))
		return false;

	return strcmp(str + strlen(str) - strlen(postfix), postfix) == 0;
}
EXPORT_SYMBOL(strends);

/**
 * match_string - matches given string in an array
 * @array:	array of strings
 * @n:		number of strings in the array or -1 for NULL terminated arrays
 * @string:	string to match with
 *
 * This routine will look for a string in an array of strings up to the
 * n-th element in the array or until the first NULL element.
 *
 * Historically the value of -1 for @n, was used to search in arrays that
 * are NULL terminated. However, the function does not make a distinction
 * when finishing the search: either @n elements have been compared OR
 * the first NULL element was found.
 *
 * Return:
 * index of a @string in the @array if matches, or %-EINVAL otherwise.
 */
int match_string(const char * const *array, size_t n, const char *string)
{
	int index;
	const char *item;

	for (index = 0; index < n; index++) {
		item = array[index];
		if (!item)
			break;
		if (!strcmp(item, string))
			return index;
	}

	return -EINVAL;
}
EXPORT_SYMBOL(match_string);

char *parse_assignment(char *str)
{
	char *value;

	value = strchr(str, '=');
	if (value)
		*value++ = '\0';

	return value;
}

char *strjoin(const char *separator, char **arr, size_t arrlen)
{
	size_t separatorlen;
	int len = 1; /* '\0' */
	char *buf, *p;
	int i;

	separatorlen = strlen(separator);

	for (i = 0; i < arrlen; i++)
		len += strlen(arr[i]) + separatorlen;

	if (!arrlen)
		return xzalloc(1);

	p = buf = xmalloc(len);

	for (i = 0; i < arrlen - 1; i++) {
		p = stpcpy(p, arr[i]);
		p = mempcpy(p, separator, separatorlen);
	}

	stpcpy(p, arr[i]);

	return buf;
}
EXPORT_SYMBOL(strjoin);

/**
 * strreplace - Replace all occurrences of character in string.
 * @str: The string to operate on.
 * @old: The character being replaced.
 * @new: The character @old is replaced with.
 *
 * Replaces the each @old character with a @new one in the given string @str.
 *
 * Return: pointer to the string @str itself.
 */
char *strreplace(char *str, char old, char new)
{
	char *s = str;

	for (; *s; ++s)
		if (*s == old)
			*s = new;
	return str;
}
EXPORT_SYMBOL(strreplace);
