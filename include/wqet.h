#ifndef wqet_h__
#define wqet_h__

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "picotls.h"

#define wqet_alloc(sz_) calloc(1, sz_)

struct wqet_long_header {
    uint8_t type;
    uint64_t connection_id;
    uint32_t packet_number;
    uint32_t version;
};

struct wqet_short_header {
    unsigned int has_connection_id : 1;
    unsigned int key_phase_bit : 1;
    uint8_t type;
    uint64_t connection_id;
    uint32_t packet_number;
};

struct wqet_version_negotiation_packet {
    size_t nr_packets;
    uint32_t versions[];
};

struct wqet_iov_reader {
    size_t idx;
    ptls_iovec_t *iov;
};

static inline struct wqet_iov_reader wqet_iov_reader_init(ptls_iovec_t *buf)
{
    struct wqet_iov_reader r = {0, buf};
    return r;
}

#define WQET_IOV_READER_READ_FN(type_)                                                                                 \
    static inline int wqet_iov_reader_read_##type_(struct wqet_iov_reader *r, type_ *val)                              \
    {                                                                                                                  \
        if (r->idx + sizeof(*val) >= r->iov->len)                                                                      \
            return 0;                                                                                                  \
        *val = *(type_ *)&r->iov->base[r->idx];                                                                        \
        r->idx += sizeof(*val);                                                                                        \
        return 1;                                                                                                      \
    }
WQET_IOV_READER_READ_FN(uint8_t)
WQET_IOV_READER_READ_FN(uint16_t)
WQET_IOV_READER_READ_FN(uint32_t)
WQET_IOV_READER_READ_FN(uint64_t)

#define WQET_IOV_READER_NTOH(r_, dst_, type_, ntoh_, err_label_)                                                       \
    {                                                                                                                  \
        type_ val_;                                                                                                    \
        if (wqet_iov_reader_read_##type_(r_, &val_) == 0)                                                              \
            goto err_label_;                                                                                           \
        dst_ = ntoh_(val_);                                                                                            \
    }
#define WQET_IOV_READER(r_, dst_, type_, err_label_)                                                                   \
    {                                                                                                                  \
        type_ val_;                                                                                                    \
        if (wqet_iov_reader_read_##type_(r_, &val_) == 0)                                                              \
            goto err_label_;                                                                                           \
        dst_ = val_;                                                                                                   \
    }

#define WQET_MAX_LONG_HEADER_LEN 17

enum {
    WQET_QUIC_PACKET_TYPE_VERSION_NEGOTIATION = 0x01,
    WQET_QUIC_PACKET_TYPE_CLIENT_INITIAL = 0x02,
    WQET_QUIC_PACKET_TYPE_SERVER_STATELESS_RETRY = 0x03,
    WQET_QUIC_PACKET_TYPE_SERVER_CLEARTEXT = 0x04,
    WQET_QUIC_PACKET_TYPE_CLIENT_CLEARTEXT = 0x05,
    WQET_QUIC_PACKET_TYPE_ZERO_RTT_PROTECTED = 0x06,
    WQET_QUIC_PACKET_TYPE_ONE_RTT_PROTECTED_KEY_PHASE_0 = 0x07,
    WQET_QUIC_PACKET_TYPE_ONE_RTT_PROTECTED_KEY_PHASE_1 = 0x08,
};

static inline ssize_t parse_long_header(struct wqet_long_header *lh, ptls_iovec_t *buf)
{
    struct wqet_iov_reader r = wqet_iov_reader_init(buf);

    WQET_IOV_READER(&r, lh->type, uint8_t, err);
    assert(lh->type & 0x80);
    WQET_IOV_READER_NTOH(&r, lh->connection_id, uint64_t, htonll, err);
    WQET_IOV_READER_NTOH(&r, lh->packet_number, uint32_t, htonl, err);
    WQET_IOV_READER_NTOH(&r, lh->version, uint32_t, htonl, err);
    return r.idx;
err:
    return -1;
}

static inline ssize_t parse_short_header(struct wqet_short_header *sh, ptls_iovec_t *buf)
{
    struct wqet_iov_reader r = wqet_iov_reader_init(buf);
    uint8_t b;

    WQET_IOV_READER(&r, b, uint8_t, err);
    assert(!(b & 0x80));
    sh->has_connection_id = !!(b & 0x40);
    sh->key_phase_bit = !!(b & 0x20);
    sh->type = b & 0x1f;
    if (sh->has_connection_id)
        WQET_IOV_READER_NTOH(&r, sh->connection_id, uint64_t, htonll, err);
    switch (sh->type) {
    case 1:
        WQET_IOV_READER(&r, sh->packet_number, uint8_t, err);
        break;
    case 2:
        WQET_IOV_READER_NTOH(&r, sh->packet_number, uint16_t, htons, err);
        break;
    case 3:
        WQET_IOV_READER_NTOH(&r, sh->packet_number, uint32_t, htonl, err);
        break;
    default:
        goto err;
    }
    return r.idx;
err:
    return -1;
}

static inline ssize_t parse_version_negotiation_packet(struct wqet_version_negotiation_packet *vnp, ptls_iovec_t *iov)
{
    struct wqet_iov_reader r = wqet_iov_reader_init(iov);
    size_t i, nr_entries;
    if ((iov->len % sizeof(uint32_t)) != 0)
        return -1;
    nr_entries = iov->len / sizeof(uint32_t);
    if (nr_entries == 0)
        return -1;
    vnp = wqet_alloc(sizeof(*vnp) + iov->len);
    for (i = 0; i < nr_entries; i++)
        WQET_IOV_READER_NTOH(&r, vnp->versions[i], uint32_t, htonl, err);

err:
    return -1;
}
#endif /* wqet_h__ */
