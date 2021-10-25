/* $KAME: inet_pton.c,v 1.5 2001/08/20 02:32:40 itojun Exp $	*/

/* Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/* Author: Paul Vixie, 1996 */
/* Copied from Linux, modified for Phoenix-RTOS. */

#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <psp2/net/net.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define SPRINTF(x) ((socklen_t) sprintf x)

#define inet_ntop4(src, dst, size) sceNetInetNtop(SCE_NET_AF_INET, src, dst, size)
#define inet_pton4(src, dst) sceNetInetPton(SCE_NET_AF_INET, src, dst)

static int inet_pton6(const char *src, u_char *dst)
{
    static const char xdigits_l[] = "0123456789abcdef";
    static const char xdigits_u[] = "0123456789ABCDEF";
    u_char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
    const char *xdigits, *curtok;
    int ch, saw_xdigit;
    u_int val;

    memset((tp = tmp), '\0', NS_IN6ADDRSZ);
    endp = tp + NS_IN6ADDRSZ;
    colonp = NULL;

    /* Leading :: requires some special handling. */
    if (*src == ':')
        if (*++src != ':')
            return 0;

    curtok = src;
    saw_xdigit = 0;
    val = 0;

    while ((ch = *src++) != '\0') {
        const char *pch;

        if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
            pch = strchr((xdigits = xdigits_u), ch);

        if (pch != NULL) {
            val <<= 4;
            val |= (pch - xdigits);

            if (val > 0xffff)
                return 0;

            saw_xdigit = 1;
            continue;
        }

        if (ch == ':') {
            curtok = src;

            if (!saw_xdigit) {
                if (colonp)
                    return 0;

                colonp = tp;
                continue;
            }

            if (tp + NS_INT16SZ > endp)
                return 0;

            *tp++ = (u_char) (val >> 8) & 0xff;
            *tp++ = (u_char) val & 0xff;
            saw_xdigit = 0;
            val = 0;
            continue;
        }

        if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
            inet_pton4(curtok, tp) > 0) {
            tp += NS_INADDRSZ;
            saw_xdigit = 0;
            break;	/* '\0' was seen by inet_pton4(). */
        }

        return 0;
    }

    if (saw_xdigit) {
        if (tp + NS_INT16SZ > endp)
            return 0;

        *tp++ = (u_char) (val >> 8) & 0xff;
        *tp++ = (u_char) val & 0xff;
    }

    if (colonp != NULL) {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        const int n = tp - colonp;
        int i;

        for (i = 1; i <= n; i++) {
            endp[- i] = colonp[n - i];
            colonp[n - i] = 0;
        }

        tp = endp;
    }

    if (tp != endp)
        return 0;

    memcpy(dst, tmp, NS_IN6ADDRSZ);
    return 1;
}

int inet_pton(int af, const char *src, void *dst)
{
    switch (af) {
        case AF_INET:
            const int ret = sceNetInetPton(SCE_NET_AF_INET, src, dst);
            return ret;

        case AF_INET6:
            return inet_pton6((const char*)src, (u_char*)dst);

        default:
            errno = EAFNOSUPPORT;
            return -1;
    }

    /* NOTREACHED */
}

static const char *inet_ntop6(const u_char *src, char *dst, socklen_t size)
{
    /*
     * Note that int32_t and int16_t need only be "at least" large enough
     * to contain a value of the specified size.  On some systems, like
     * Crays, there is no such thing as an integer variable with 16 bits.
     * Keep this in mind if you think this function should have been coded
     * to use pointer overlays.  All the world's not a VAX.
     */
    char tmp[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")], *tp;
    struct {
        int base, len;
    } best, cur;
    u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
    int i;
    /*
     * Preprocess:
     *	Copy the input (bytewise) array into a wordwise array.
     *	Find the longest run of 0x00's in src[] for :: shorthanding.
     */
    memset(words, '\0', sizeof words);

    for (i = 0; i < NS_IN6ADDRSZ; i++)
        words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));

    best.base = -1;
    cur.base = -1;

    for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
        if (words[i] == 0) {
            if (cur.base == -1)
                cur.base = i, cur.len = 1;

            else
                cur.len++;
        }

        else {
            if (cur.base != -1) {
                if (best.base == -1 || cur.len > best.len)
                    best = cur;

                cur.base = -1;
            }
        }
    }

    if (cur.base != -1) {
        if (best.base == -1 || cur.len > best.len)
            best = cur;
    }

    if (best.base != -1 && best.len < 2)
        best.base = -1;

    /*
     * Format the result.
     */
    tp = tmp;

    for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
        /* Are we inside the best run of 0x00's? */
        if (best.base != -1 && i >= best.base &&
            i < (best.base + best.len)) {
            if (i == best.base)
                *tp++ = ':';

            continue;
        }

        /* Are we following an initial run of 0x00s or any real hex? */
        if (i != 0)
            *tp++ = ':';

        /* Is this address an encapsulated IPv4? */
        if (i == 6 && best.base == 0 &&
            (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
            if (!inet_ntop4(src + 12, tp, sizeof tmp - (tp - tmp)))
                return NULL;

            tp += strlen(tp);
            break;
        }

        tp += SPRINTF((tp, "%x", words[i]));
    }

    /* Was it a trailing run of 0x00's? */
    if (best.base != -1 && (best.base + best.len) ==
                           (NS_IN6ADDRSZ / NS_INT16SZ))
        *tp++ = ':';

    *tp++ = '\0';

    /*
     * Check for overflow, copy, and we're done.
     */
    if ((socklen_t)(tp - tmp) > size) {
        errno = ENOSPC;
        return NULL;
    }

    strcpy(dst, tmp);
    return dst;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
  switch (af) {
    case AF_INET:
      sceNetInetNtop(SCE_NET_AF_INET, src, dst, size);
      return dst;

    case AF_INET6:
      return inet_ntop6(src, dst, size);

    default:
      errno = EAFNOSUPPORT;
      return NULL;
  }

  /* NOTREACHED */
}
int inet_aton(const char *cp, struct in_addr *inp)
{
    u_long parts[4];
    in_addr_t val;
    char *c;
    char *endptr;
    int gotend, n;
    c = (char *)cp;
    n = 0;
    /*
     * Run through the string, grabbing numbers until
     * the end of the string, or some error
     */
    gotend = 0;

    while (!gotend) {
        errno = 0;
        val = strtoul(c, &endptr, 0);

        if (errno == ERANGE)	/* Fail completely if it overflowed. */
            return 0;

        /*
         * If the whole string is invalid, endptr will equal
         * c.. this way we can make sure someone hasn't
         * gone '.12' or something which would get past
         * the next check.
         */
        if (endptr == c)
            return 0;

        parts[n] = val;
        c = endptr;

        /* Check the next character past the previous number's end */
        switch (*c) {
            case '.':

                /* Make sure we only do 3 dots .. */
                if (n == 3)	/* Whoops. Quit. */
                    return 0;

                n++;
                c++;
                break;

            case '\0':
                gotend = 1;
                break;

            default:
                if (isspace((unsigned char)*c)) {
                    gotend = 1;
                    break;
                }

                else
                    return 0;	/* Invalid character, so fail */
        }
    }

    /*
     * Concoct the address according to
     * the number of parts specified.
     */

    switch (n) {
        case 0:				/* a -- 32 bits */
            /*
             * Nothing is necessary here.  Overflow checking was
             * already done in strtoul().
             */
            break;

        case 1:				/* a.b -- 8.24 bits */
            if (val > 0xffffff || parts[0] > 0xff)
                return 0;

            val |= parts[0] << 24;
            break;

        case 2:				/* a.b.c -- 8.8.16 bits */
            if (val > 0xffff || parts[0] > 0xff || parts[1] > 0xff)
                return 0;

            val |= (parts[0] << 24) | (parts[1] << 16);
            break;

        case 3:				/* a.b.c.d -- 8.8.8.8 bits */
            if (val > 0xff || parts[0] > 0xff || parts[1] > 0xff ||
                parts[2] > 0xff)
                return 0;

            val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
            break;
    }

    if (inp != NULL)
        inp->s_addr = htonl(val);

    return 1;
}

in_addr_t inet_addr(const char *cp)
{
    struct in_addr val;

    if (inet_aton(cp, &val))
        return val.s_addr;

    return INADDR_NONE;
}
