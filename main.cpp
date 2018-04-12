#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/un.h>

#include "util.h"

int pids[4];
int processIndex = 0;

sig_atomic_t clientSocket[4];

bool hasRequest = false;

struct msghdr m;
struct cmsghdr *cm;
struct iovec iov;
char buf[CMSG_SPACE(sizeof(int))];
char dummy[2];
ssize_t readlen;
int *fdlist;

char *socket_path = "\0hidden";
int fd,cl,rc;

void handle(int signal_number) {
    hasRequest = true;
}

void handleParen(int signal_number) {

}

int main () {
    int i = 0;
    pid_t child_pid;
    int fd;

    char buffer[BUFF_SIZE + 1];


    int sock = create_server_socket(false);

    for(i = 0; i < 4; i++) {
        child_pid = fork();
        processIndex = i;

        if(child_pid == 0) {
            break;
        }
        else {
            pids[i] = child_pid;
        }
    }

    if(child_pid == 0) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = &handle;
        sigaction(SIGUSR1, &sa, NULL);

        while(true) {
            if(hasRequest) {
                sig_atomic_t client;

                struct sockaddr_un addr;
                int fd,rc;

                if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
                    perror("socket error");
                    exit(-1);
                }

                memset(&addr, 0, sizeof(addr));
                addr.sun_family = AF_UNIX;
                if (*socket_path == '\0') {
                    *addr.sun_path = '\0';
                    strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
                } else {
                    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
                }

                if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
                    perror("connect error");
                    exit(-1);
                }

                iov.iov_base = dummy;
                iov.iov_len = sizeof(dummy);
                memset(&m, 0, sizeof(m));
                m.msg_iov = &iov;
                m.msg_iovlen = 1;
                m.msg_controllen = CMSG_SPACE(sizeof(int));
                m.msg_control = buf;
                readlen = recvmsg(fd, &m, 0);
                client = -1; /* Default: none was received */
                for (cm = CMSG_FIRSTHDR(&m); cm; cm = CMSG_NXTHDR(&m, cm)) {
                    if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SCM_RIGHTS) {
                        fdlist = (int *)CMSG_DATA(cm);
                        client = *fdlist;
                        break;
                    }
                }

                printf("I'm %d\n", getpid());

                bzero(buffer, sizeof(buffer));
                int bytes_read = recv(client, buffer, sizeof(buffer), 0);

                char response[BUFF_SIZE + 2];
                bzero(response, sizeof(response));
                generate_echo_response(buffer, response);
                //puts(buffer);

                int bytes_written = send(client, response, strlen(response), 0);
                if(bytes_written < 0) {
                    error_msg("Problem with send call", false);
                }

                bzero(response, sizeof(response));
                strcpy(response, "Please wait 5 sec for connection to be closed\n");
                bytes_written = send(client, response, strlen(response), 0);
                if(bytes_written < 0) {
                    error_msg("Problem with send call", false);
                }
                fsync(client);

                sleep(5);

                close(client);
                close(fd);
            }

            pause();
        }
    }
    else {
        if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
            perror("socket error");
            exit(-1);
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;

        if (*socket_path == '\0') {
            *addr.sun_path = '\0';
            strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
        } else {
            strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
            unlink(socket_path);
        }

        if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            perror("bind error");
            exit(-1);
        }

        if (listen(fd, 5) == -1) {
            perror("listen error");
            exit(-1);
        }
    }

    int pidIndex = 0;

    while(true) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);

        sig_atomic_t client = accept(sock, (struct sockaddr*)&client_addr, &len);
        announce_client(&client_addr.sin_addr);

        printf("waking up a process, pid index %d\n", pids[pidIndex]);
        kill(pids[pidIndex], SIGUSR1);

        if ( (cl = accept(fd, NULL, NULL)) == -1) {
              perror("accept error");
              continue;
        }

        memset(&m, 0, sizeof(m));
        m.msg_controllen = CMSG_SPACE(sizeof(int));
        m.msg_control = &buf;
        memset(m.msg_control, 0, m.msg_controllen);
        cm = CMSG_FIRSTHDR(&m);
        cm->cmsg_level = SOL_SOCKET;
        cm->cmsg_type = SCM_RIGHTS;
        cm->cmsg_len = CMSG_LEN(sizeof(int));
        *((int *)CMSG_DATA(cm)) = client;
        m.msg_iov = &iov;
        m.msg_iovlen = 1;
        iov.iov_base = dummy;
        iov.iov_len = 1;
        dummy[0] = 0;
        sendmsg(cl, &m, 0);
        //pause();

        close(client);

        pidIndex = (pidIndex + 1) % 4;
    }

    return 0;
}
