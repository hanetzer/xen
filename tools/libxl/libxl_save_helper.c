/*
 * Copyright (C) 2012      Citrix Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 only. with the special
 * exception on linking described in file LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

/*
 * The libxl-save-helper utility speaks a protocol to its caller for
 * the callbacks.  The protocol is as follows.
 *
 * The helper talks on stdin and stdout, in binary in machine
 * endianness.  The helper speaks first, and only when it has a
 * callback to make.  It writes a 16-bit number being the message
 * length, and then the message body.
 *
 * Each message starts with a 16-bit number indicating which of the
 * messages it is, and then some arguments in a binary marshalled form.
 * If the callback does not need a reply (it returns void), the helper
 * just continues.  Otherwise the helper waits for its caller to send a
 * single int which is to be the return value from the callback.
 *
 * Where feasible the stubs and callbacks have prototypes identical to
 * those required by xc_domain_save and xc_domain_restore, so that the
 * autogenerated functions can be used/provided directly.
 *
 * The actual messages are in the array @msgs in libxl_save_msgs_gen.pl
 */

#include "libxl_osdeps.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <fcntl.h>
#include <signal.h>

#include "libxl.h"
#include "libxl_utils.h"

#include "xenctrl.h"
#include "xenguest.h"
#include "_libxl_save_msgs_helper.h"

/*----- logger -----*/

static void tellparent_vmessage(xentoollog_logger *logger_in,
                                xentoollog_level level,
                                int errnoval,
                                const char *context,
                                const char *format,
                                va_list al)
{
    char *formatted;
    int r = vasprintf(&formatted, format, al);
    if (r < 0) { perror("memory allocation failed during logging"); exit(-1); }
    helper_stub_log(level, errnoval, context, formatted, 0);
    free(formatted);
}

static void tellparent_progress(struct xentoollog_logger *logger_in,
                                const char *context,
                                const char *doing_what, int percent,
                                unsigned long done, unsigned long total)
{
    helper_stub_progress(context, doing_what, done, total, 0);
}

static void tellparent_destroy(struct xentoollog_logger *logger_in)
{
    abort();
}

/*----- globals -----*/

static const char *program = "libxl-save-helper";
static xentoollog_logger logger = {
    tellparent_vmessage,
    tellparent_progress,
    tellparent_destroy,
};
static xc_interface *xch;
static int io_fd;

/*----- error handling -----*/

static void fail(int errnoval, const char *fmt, ...)
    __attribute__((noreturn,format(printf,2,3)));
static void fail(int errnoval, const char *fmt, ...)
{
    va_list al;
    va_start(al,fmt);
    xtl_logv(&logger,XTL_ERROR,errnoval,program,fmt,al);
    exit(-1);
}

static int read_exactly(int fd, void *buf, size_t len)
/* returns 0 if we get eof, even if we got it midway through; 1 if ok */
{
    while (len) {
        ssize_t r = read(fd, buf, len);
        if (r<=0) return r;
        assert(r <= len);
        len -= r;
        buf = (char*)buf + r;
    }
    return 1;
}

static void *xmalloc(size_t sz)
{
    if (!sz) return 0;
    void *r = malloc(sz);
    if (!r) { perror("memory allocation failed"); exit(-1); }
    return r;
}

/*----- signal handling -----*/

static int unwriteable_fd;

static void save_signal_handler(int num)
{
    /*
     * We want to be able to interrupt save.  But the code in libxc
     * which does the actual saving is straight-through, and we need
     * to execute its error path to put the guest back to sanity.
     *
     * So what we do is this: when we get the signal, we dup2
     * the result of open("/dev/null",O_RDONLY) onto the output fd.
     *
     * This is guaranteed to 1. interrupt libxc's write (causing it to
     * return short, or maybe EINTR); 2. make the next write give
     * EBADF, so that: 3. at latest, libxc will notice when it next
     * tries to write data and will then go into its cleanup path.
     *
     * We make no effort here to sanitise the resulting errors.
     * That's libxl's job.
     */
    int esave = errno;

    int r = dup2(unwriteable_fd, io_fd);
    assert(r == io_fd); /* if not we can't write an xtl message because we
                         * might end up interleaving on our control stream */

    errno = esave;
}

static void setup_signals(void (*handler)(int))
{
    struct sigaction sa;
    sigset_t spmask;
    int r;

    unwriteable_fd = open("/dev/null",O_RDONLY);
    if (unwriteable_fd < 0) fail(errno,"open /dev/null for reading");

    LIBXL_FILLZERO(sa);
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    r = sigaction(SIGTERM, &sa, 0);
    if (r) fail(errno,"sigaction SIGTERM failed");

    sigemptyset(&spmask);
    sigaddset(&spmask,SIGTERM);
    r = sigprocmask(SIG_UNBLOCK,&spmask,0);
    if (r) fail(errno,"sigprocmask unblock SIGTERM failed");
}

/*----- helper functions called by autogenerated stubs -----*/

unsigned char * helper_allocbuf(int len, void *user)
{
    return xmalloc(len);
}

static void transmit(const unsigned char *msg, int len, void *user)
{
    while (len) {
        int r = write(1, msg, len);
        if (r<0) { perror("write"); exit(-1); }
        assert(r >= 0);
        assert(r <= len);
        len -= r;
        msg += r;
    }
}

void helper_transmitmsg(unsigned char *msg_freed, int len_in, void *user)
{
    assert(len_in < 64*1024);
    uint16_t len = len_in;
    transmit((const void*)&len, sizeof(len), user);
    transmit(msg_freed, len, user);
    free(msg_freed);
}

int helper_getreply(void *user)
{
    int v;
    int r = read_exactly(0, &v, sizeof(v));
    if (r<=0) exit(-2);
    return v;
}

/*----- other callbacks -----*/

static struct save_callbacks helper_save_callbacks;

static void startup(const char *op) {
    xtl_log(&logger,XTL_DEBUG,0,program,"starting %s",op);

    xch = xc_interface_open(&logger,&logger,0);
    if (!xch) fail(errno,"xc_interface_open failed");
}

static void complete(int retval) {
    int errnoval = retval ? errno : 0; /* suppress irrelevant errnos */
    xtl_log(&logger,XTL_DEBUG,errnoval,program,"complete r=%d",retval);
    helper_stub_complete(retval,errnoval,0);
    xc_interface_close(xch);
    exit(0);
}

static struct restore_callbacks helper_restore_callbacks;

int main(int argc, char **argv)
{
    int r;

#define NEXTARG (++argv, assert(*argv), *argv)

    const char *mode = *++argv;
    assert(mode);

    if (!strcmp(mode,"--save-domain")) {

        io_fd =                    atoi(NEXTARG);
        uint32_t dom =             strtoul(NEXTARG,0,10);
        uint32_t max_iters =       strtoul(NEXTARG,0,10);
        uint32_t max_factor =      strtoul(NEXTARG,0,10);
        uint32_t flags =           strtoul(NEXTARG,0,10);
        int hvm =                  atoi(NEXTARG);
        unsigned cbflags =         strtoul(NEXTARG,0,10);
        assert(!*++argv);

        helper_setcallbacks_save(&helper_save_callbacks, cbflags);

        startup("save");
        setup_signals(save_signal_handler);

        r = xc_domain_save2(xch, io_fd, dom, max_iters, max_factor, flags,
                           &helper_save_callbacks, hvm);
        complete(r);

    } else if (!strcmp(mode,"--restore-domain")) {

        io_fd =                    atoi(NEXTARG);
        uint32_t dom =             strtoul(NEXTARG,0,10);
        unsigned store_evtchn =    strtoul(NEXTARG,0,10);
        domid_t store_domid =      strtoul(NEXTARG,0,10);
        unsigned console_evtchn =  strtoul(NEXTARG,0,10);
        domid_t console_domid =    strtoul(NEXTARG,0,10);
        unsigned int hvm =         strtoul(NEXTARG,0,10);
        unsigned int pae =         strtoul(NEXTARG,0,10);
        int superpages =           strtoul(NEXTARG,0,10);
        unsigned cbflags =         strtoul(NEXTARG,0,10);
        int checkpointed =         strtoul(NEXTARG,0,10);
        assert(!*++argv);

        helper_setcallbacks_restore(&helper_restore_callbacks, cbflags);

        unsigned long store_mfn = 0;
        unsigned long console_mfn = 0;

        startup("restore");
        setup_signals(SIG_DFL);

        r = xc_domain_restore2(xch, io_fd, dom, store_evtchn, &store_mfn,
                              store_domid, console_evtchn, &console_mfn,
                              console_domid, hvm, pae, superpages,
                              checkpointed,
                              &helper_restore_callbacks);
        helper_stub_restore_results(store_mfn,console_mfn,0);
        complete(r);

    } else {
        assert(!"unexpected mode argument");
    }
}

/*
 * Local variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
