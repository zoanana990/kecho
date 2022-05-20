#include <linux/errno.h>
#include <linux/in.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/signal.h>
#include <linux/tcp.h>
#include <linux/types.h>
#include <linux/version.h>
#include <net/sock.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
#define USE_SETSOCKET
#endif

#include "techo_server.h"

#define DEFAULT_PORT 12345
#define DEFAULT_BACKLOG 128

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fast echo server in kernel");
MODULE_VERSION("0.1");

static ushort port = DEFAULT_PORT;
static ushort backlog = DEFAULT_BACKLOG;
static bool bench = false;
module_param(port, ushort, S_IRUGO);
module_param(backlog, ushort, S_IRUGO);
module_param(bench, bool, S_IRUGO);

struct echo_server_param param;
struct socket *listen_sock;
struct task_struct *echo_server;

static int open_listen(struct socket **);
static void close_listen(struct socket *);

static int kecho_init_module(void)
{
    int error = open_listen(&listen_sock);
    if (error < 0) {
        printk(KERN_ERR MODULE_NAME ": listen socket open error\n");
        return error;
    }

    param.listen_sock = listen_sock;

    echo_server = kthread_run(echo_server_daemon, &param, MODULE_NAME);
    if (IS_ERR(echo_server)) {
        printk(KERN_ERR MODULE_NAME ": cannot start server daemon\n");
        close_listen(listen_sock);
    }

    return 0;
}

static void kecho_cleanup_module(void)
{
    send_sig(SIGTERM, echo_server, 1);
    kthread_stop(echo_server);
    close_listen(listen_sock);
    printk(MODULE_NAME ": module successfully removed \n");
}

static int open_listen(struct socket **result)
{
    struct socket *sock;
    struct sockaddr_in addr;
    int error;
    int opt = 1;
#ifdef USE_SETSOCKET
    sockptr_t kopt = {.kernel = (char *) &opt, .is_kernel = 1};
#endif

    /* using IPv4, TCP/IP */
    error = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (error < 0) {
        printk(KERN_ERR MODULE_NAME ": socket create error = %d\n", error);
        return error;
    }
    printk(MODULE_NAME ": socket create ok....\n");

/* set tcp_nodelay */
#ifdef USE_SETSOCKET
    error =
        sock->ops->setsockopt(sock, SOL_TCP, TCP_NODELAY, kopt, sizeof(opt));
#else
    error = kernel_setsockopt(sock, SOL_TCP, TCP_NODELAY, (char *) &opt,
                              sizeof(opt));
#endif

    if (error < 0) {
        printk(KERN_ERR MODULE_NAME
               ": setsockopt tcp_nodelay setting error = %d\n",
               error);
        sock_release(sock);
        return error;
    }

#ifdef USE_SETSOCKET
    error = sock_setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, kopt, sizeof(opt));
#else
    error = kernel_setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char *) &opt,
                              sizeof(opt));
#endif

    if (error < 0) {
        printk(KERN_ERR MODULE_NAME
               ": setsockopt SO_REUSEPORT setting error = %d\n",
               error);
        sock_release(sock);
        return error;
    }
    printk(MODULE_NAME ": setsockopt ok....\n");

    /* set sockaddr_in */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    /* bind */
    error = kernel_bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    if (error < 0) {
        printk(KERN_ERR MODULE_NAME ": socket bind error = %d\n", error);
        sock_release(sock);
        return error;
    }
    printk(MODULE_NAME ": socket bind ok....\n");

    /* listen */
    error = kernel_listen(sock, backlog);
    if (error < 0) {
        printk(KERN_ERR MODULE_NAME ": socket listen error = %d\n", error);
        sock_release(sock);
        return error;
    }
    printk(MODULE_NAME ": socket listen ok....\n");

    *result = sock;
    return 0;
}

static void close_listen(struct socket *sock)
{
    kernel_sock_shutdown(sock, SHUT_RDWR);
    sock_release(sock);
}

module_init(kecho_init_module);
module_exit(kecho_cleanup_module);
