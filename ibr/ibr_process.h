#ifndef C_ATOM_IBR_PROCESS_H
#define C_ATOM_IBR_PROCESS_H

#include <pthread.h>
#include "ibr_msg.h"
#include "ibr_convert.h"

typedef ibr_rv_t (*irb_media_proclaim)(const char *path, ibr_msg_t *src_msg, ibr_msg_t *dst_msg, int *md);
typedef ibr_rv_t (*irb_media_connect)(const char *path, int *md);
typedef ibr_rv_t (*irb_media_disconnect)(int md);
typedef ibr_rv_t (*irb_media_send)(int md, void *d, int *bts);
typedef ibr_rv_t (*irb_media_recv)(int md, void *d, int *btr);

typedef struct {
    irb_media_proclaim      proclaim;
    irb_media_connect       connect;
    irb_media_disconnect    disconnect;
    irb_media_send          send;
    irb_media_recv          recv;
} irb_media_driver_t;

typedef enum {
    mdt_function,
    mdt_function_bridge,
    mdt_function_vector,
    mdt_file,
    mdt_serial,
    mdt_sdtl,
    mdt_udp
} irb_media_driver_type_t;

typedef struct {
    irb_media_driver_type_t mdt;
    const irb_media_driver_t *drv;
    const char *access_addr;
    int descr;
} irb_media_setup_t;


typedef struct msg_record {
    uint32_t id;
    ibr_msg_t *src_msg;

    ibr_msg_t *dst_msg;

    conv_instr_queue_t conv_queue;
    int descr; // connection descriptor
} msg_record_t;


typedef enum {
    ibr_process_type_frame,
    ibr_process_type_copy,
    ibr_process_type_invalid,
} ibr_process_type_t;

typedef struct {
    const char *name;

    ibr_process_type_t type;

    irb_media_setup_t src;
    irb_media_setup_t dst;

    frame_t *frame;

    unsigned msgs_num;
    msg_record_t *msgs_setup;

    pthread_t tid;

} irb_process_setup_t;


typedef struct {
    irb_process_setup_t *process_setups;
    int                 processes_num;
} irb_setup_t;


const irb_media_driver_t *ibr_get_driver (irb_media_driver_type_t mdt);
ibr_rv_t ibr_decode_addr(const char *addr, irb_media_driver_type_t *mdt, const char **addr_rv);
void *ibr_process_thread (void *setup);
ibr_rv_t ibr_process_start (irb_process_setup_t *setup);

#endif //C_ATOM_IBR_PROCESS_H
