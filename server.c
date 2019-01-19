#include "threadpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <asm/errno.h>
#include <errno.h>
#include <time.h>

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define USAGE "server <port> <pool-size> <max-number-of-request>"

typedef struct response
{
   char * date;
   int contentLength;
   char* responseMsg;
   char *typefile;
   char* location;


}response;

response *h;


response *init()
{
   response *r = (response*)malloc(sizeof(response));
    r->date=NULL;
    r->contentLength=0;
    r->responseMsg=NULL;
    r->typefile=NULL;
    r->location=NULL;
   return r;
}
char* statusCode(int status);
char* getDate();
void checkDir(char*path);
void error(char *msg);
char *get_mime_type(char *name);
int handleRequest(int socket);
char* responseMsg(char *status , char*date );


int main (int argc , char ** argv)
{

    threadpool *tpool=create_threadpool(atoi(argv[2]));
    int sockfd, newsockfd, portno, clilen;

    struct sockaddr_in serv_addr, cli_addr;

    if(argc!=4)
    {
        error(USAGE);
    }


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        perror("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    h = init();

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    while(1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        printf("GOT from PORT");
        dispatch(tpool, handleRequest, newsockfd);
    }

    close(newsockfd);
    return 0;
}



char* getDate()
{
    char*timeStr=NULL;
    time_t now;
    char timebuf[128];
    now=time(NULL);
    h->date= (char *) strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
    strcpy(timeStr,timebuf);
    return  h->date;
}
void checkDir(char* path)
{
    DIR* dir = NULL;
    if(path[strlen(path)-1] == ' ')
    {
        path[strlen(path)-1] = 0;
    }
    dir = opendir(path);
    if (dir)
    {
        //printf("%c",path[strlen(path)-1]);
        if(path[strlen(path)-1] != '/')
        {
            h->location=path;
            statusCode(302);

        }
        else
        {
            printf("dir exist");
        }

        closedir(dir);
    }
    else
    {
        printf("404");
    }
}


void error(char *msg)
{
    perror(msg);
    exit(1);
}

char* getString() {
    return "HTTP/1.0 200 OK\n"
           "Server: webserver/1.0\n"
           "Date: Fri, 05 Nov 2010 06:53:58 GMT\n"
           "Content-Type: text/html\n"
           "Content-Length: <content-length>\n"
           "Last-Modified: Wed, 27 Oct 2010 06:50:29 GMT\n"
           "Connection: close\n"
           "\n"
           "<HTML>\n"
           "<HEAD><TITLE>Index of <path-of-directory></TITLE></HEAD>\n"
           "\n"
           "<BODY>\n"
           "<H4>Index of <path-of-directory></H4>\n"
           "\n"
           "<table CELLSPACING=8>\n"
           "<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n"
           "\n"
           "\n"
           "<!-- for each entity in the path add the following -->\n"
           "<tr>\n"
           "<td><A HREF=\"<entity-name>\"><entity-name (file or sub-directory)></A></td><td><modification time></td>\n"
           "<td><if entity is a file, add file size, otherwise, leave empty></td>\n"
           "</tr>\n"
           "</table>\n"
           "<HR>\n"
           "<ADDRESS>webserver/1.0</ADDRESS>\n"
           "</BODY></HTML>";
}
char *get_mime_type(char *name) {
    char *ext = strrchr(name, '.');
    if (!ext) return NULL;
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".au") == 0) return "audio/basic";
    if (strcmp(ext, ".wav") == 0) return "audio/wav";
    if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
    if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
    return NULL;
}

int handleRequest(int socket)
{
    char buffer[256];
    int n = 0, length = 0;

    char* req = NULL;
    char *line;
    char* method;
    char* path;
    char* http;
    int i=0;
    const char s[2] = " ";
    char *token;

    if (socket < 0)
        error("ERROR on accept");
    write(socket,getString(),strlen(getString()));

    while(n = read(socket, buffer, 255) != 0)
    {

        length = req ? strlen(req) : 0;
        req=(char*)realloc(req, sizeof(char)*(strlen(buffer)+length+1));

        req = strcat(req, buffer);
        if (n < 0) error("ERROR reading from socket");
        bzero(buffer, 256);
    }


    length = strchr(req, '\r')-req;
    line = (char*) malloc(sizeof(char)*(length+1)); //freeIt
    line = strncat(line, req, length);
    path = strchr(line,'/');
    *path = 0;
    path++;

    http = path+strlen(path)-8;
    if(*http != 'H')
    {
        statusCode(400);
    }// all good
    *http = 0;
    http++;
    http = strchr(http,'/');
    http++;
    if(strcmp(http,"1.1")!=0)
    {
        statusCode(400);
    }

    if(strcmp(line,"GET")!=0)
    {
        statusCode(501);
    }

    checkDir(path);

   /* method = (char*) malloc(sizeof(char)*(strlen(line))+1);
    path=(char*) malloc(sizeof(char)*(strlen(line))+1);
    http=(char*) malloc(sizeof(char)*(strlen(line))+1);
    strncat(line, req, length);
    token = strtok(line, s);
 //  next = strchr(line,' ');//"GET f.fdssfs"
    path = strchr(req, ' ');
    *path = 0;
    path++;

    path = &req[length];
    *path = 0;
    path++;*/



    while(token!=NULL)
    {
        if(i==0)
        {
            strcpy(method, token);
            token = strtok(NULL, s);
            i++;
        }
        else if(i==1)
        {
            strcpy(path, token);
            token = strtok(NULL, s);
            i++;
        }
        else
        {
            strcpy(http, token);
            token = strtok(NULL, s);
        }
    }




       if(method!=NULL && path!=NULL && http!=NULL)
       {
        if(strcmp(http,"HTTP/1.0")!=0)
        {
            statusCode(400);
        }
       }
        else
        {
        statusCode(400);
        }

        if(strcmp(method,"GET")!=0)
        {
            statusCode(501);
        }
        checkDir(path);


    // TODO check the path if exists and respon ddue to it.
    printf("Here is the message: %s\n", buffer);
    n = write(socket, "I got your message", 18);
    if (n < 0) error("ERROR writing to socket");

}

char* responseMsg(char *status , char*date )
{
    char*arr= (char*)malloc(sizeof(char));
    sprintf(arr,"HTTP/1.0 %s\r\n"
    "Server: webserver/1.0\r\n"
    "Date:%s\r\n"
    "Content-Type:%s\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n"
    "\n"
    "<HTML><HEAD><TITLE>%s</TITLE></HEAD>\r\n"
    "<BODY><H4>404 Not Found</H4>\r\n"
    "File not found.\r\n"
    "</BODY></HTML>");
    return arr;
}

char* statusCode(int status)
{
    switch(status)
    {
        case 302:
        {
            return "302 FOUND";
        }
        case 400:
        {
            return "400 Bad Request";
        }
        case 403:
        {
            return "403 Forbidden";
        }
        case 404:
        {
            return "404 Not Found";
        }
        case 500:
        {
            return "500 Internal Server Error";
        }
        case 501:
        {
            return "501 Not supported";
        }
        default:
        {
            perror("Status code not found");
            return NULL;
        }
    }
}

