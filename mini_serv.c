#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>


int client[4096] = {-1};
char *messages[4096];
fd_set activefd, writefd, readfd;
int serverfd;

void ft_error(char *str)
{
    write(2, str, strlen(str));
    if (serverfd > 0)
        close(serverfd);
    exit(1);
}

void ft_send(int fd, int maxfd, char *str)
{
    for (int i = maxfd; i > serverfd; i--)
        if (client[i] != -1 && i != fd && FD_ISSET(i, &writefd))
            send(i, str, strlen(str), 0);
}

int extract_message(char **buf, char **msg)
{
    char *newbuf;
    int i;

    *msg = 0;
    if (*buf == 0)
        return (0);
    i = 0;
    while ((*buf)[i])
    {
        if ((*buf)[i] == '\n')
        {
            newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
            if (newbuf == 0)
                return (-1);
            strcpy(newbuf, *buf + i + 1);
            *msg = *buf;
            (*msg)[i + 1] = 0;
            *buf = newbuf;
            return (1);
        }
        i++;
    }
    return (0);
}

char *str_join(char *buf, char *add)
{
    char *newbuf;
    int len;

    if (buf == 0)
        len = 0;
    else
        len = strlen(buf);
    newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
    if (newbuf == 0)
        return (0);
    newbuf[0] = 0;
    if (buf != 0)
        strcat(newbuf, buf);
    free(buf);
    strcat(newbuf, add);
    return (newbuf);
}


int main(int ac, char **av)
{
    if (ac != 2)
        ft_error("Wrong number of arguments\n");
    
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd == -1)
        ft_error("Fatal error\n");

    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
    servaddr.sin_port = htons(atoi(av[1]));
    if ((bind(serverfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        ft_error("Fatal error\n");
    if (listen(serverfd, 128) != 0)
        ft_error("Fatal error\n");
    FD_ZERO(&activefd);
    FD_SET(serverfd, &activefd);
    int maxfd = serverfd;
    int id = 0;
    while (1)
    {
        readfd = writefd = activefd;
        if (select(maxfd + 1, &readfd, &writefd, NULL, NULL) < 0)
            continue;
        if (FD_ISSET(serverfd, &readfd))
        {
            int clientfd = accept(serverfd, NULL, NULL);
            if (clientfd < 0)
                continue;
            FD_SET(clientfd, &activefd);
            char buf[64];
            sprintf(buf, "server: client %d just arrived\n", id);
            client[clientfd] = id++;
            messages[clientfd] = calloc(1,1);
            maxfd = (clientfd > maxfd) ? clientfd: maxfd;
            ft_send(clientfd, maxfd, buf);
        }
        for (int fd = maxfd; fd > serverfd; fd--)
        {
            if (FD_ISSET(fd, &readfd))
            {
                char buffer[4095];
                int bytes = recv(fd, buffer, 4094, 0);
                if (bytes <= 0)
                {
                    FD_CLR(fd, &activefd);
                    char buf[64];
                    sprintf(buf, "server: client %d just left\n", client[fd]);
                    ft_send(fd, maxfd, buf);
                    client[fd] = -1;
                    free(messages[fd]);
                    close(fd);
                }
                else
                {
                    buffer[bytes] = 0;
                    messages[fd] = str_join(messages[fd], buffer);
                    char *msg;
                    while (extract_message(&messages[fd], &msg))
                    {
                        char buf[64 + strlen(msg)];
                        sprintf(buf, "client %d: %s", client[fd], msg);
                        ft_send(fd, maxfd, buf);
                    }
                }
            }
        }
    }
    return 0;    
}
