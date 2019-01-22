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

enum contentType
{
    HTML, JPEG, GIF, PNG, CSS, BASIC, WAV, MSVIDEO, V_MPEG, A_MPEG
};

typedef struct response
{
    char * date;
    int contentLength;
    char* responseMsg;
    char** filesNames;
    int typefile;
    char* location;
    int filesCount;
    int statusCode;
    char* path;

}response;

response *h;

response *init()
{
    response *r = (response*)malloc(sizeof(response));
    r->date=NULL;
    r->contentLength=0;
    r->responseMsg=NULL;
    r->filesNames=NULL;
    r->typefile= 0;
    r->location=NULL;
    r->statusCode = 0;
    r->filesCount = 0;
    r->path = NULL;
    return r;
}
char* createResponse();
char* statusCode(int status);
char* getDate();
void checkDir(char*path);
void error(char *msg);
int get_mime_type(char *name);
int handleRequest(int socket);
char* responseMsg();
char* getStatusMsg(int status);
int getLength();
char* getLocation();



int main (int argc , char ** argv)
{

    int count=0;
    int numOfRequest=atoi(argv[3]);
    threadpool *tpool=create_threadpool(atoi(argv[2]));
    int sockfd, newsockfd, portno, clilen;

    struct sockaddr_in serv_addr, cli_addr;
    //  printf("/nIM LONG AS %d/n",strlen("<tr><td><A HREF=""><entity-name (file or sub-directory)></A></td><td><modification time></td><td><if entity is a file, add file size, otherwise, leave empty></td></tr>"));

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
        count++;
        printf("%d\n",count);
    }

    printf("number of request exeded\n");
    close(sockfd);
    destroy_threadpool(tpool);
    return 0;
}



char* getDate()
{
    time_t now;
    static char timebuf[128];
    now=time(NULL);
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));

    return timebuf;
}
void checkDir(char* path)
{
    DIR *d;
    struct dirent *dir;
    if(path[strlen(path)-1] == ' ')
    {
        path[strlen(path)-1] = 0;
    }
    d = opendir(path);
    if (d)
    {
        //printf("%c",path[strlen(path)-1]);
        if(path[strlen(path)-1] != '/')
        {
            h->location=path;
            h->statusCode=302;
            h->typefile = HTML;
        }
        else
        {

            while ((dir = readdir(d)) != NULL)
            {
                if(strcmp(dir->d_name,"index.html")==0)
                {

                }
                else if(strcmp(dir->d_name,"..") != 0 && strcmp(dir->d_name,".") != 0)
                {
//                    h->responseBody=(response*)realloc()
//                    dir->name
                    h->filesCount++;
                    h->filesNames = (char**) realloc(h->filesNames, sizeof(char*) * (h->filesCount));
                    h->filesNames[h->filesCount-1] = (char*) malloc(sizeof(char) * (strlen(dir->d_name) + 1));
                    strcpy(h->filesNames[h->filesCount-1], dir->d_name);
                }

            }
            printf("%s",&h->filesNames[0][0]);
            printf("%s",&h->filesNames[0][1]);
            closedir(d);
        }

    }

    else
    {
        h->typefile = HTML;
        h->statusCode = 404;
    }
}


void error(char *msg)
{
    perror(msg);
    exit(1);
}

/*char* getString() {
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
}*/
int get_mime_type(char *name) {
    char *ext = strrchr(name, '.');
    if (!ext) return NULL;
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return HTML;
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return JPEG;
    if (strcmp(ext, ".gif") == 0) return GIF;
    if (strcmp(ext, ".png") == 0) return PNG;
    if (strcmp(ext, ".css") == 0) return CSS;
    if (strcmp(ext, ".au") == 0) return BASIC;
    if (strcmp(ext, ".wav") == 0) return WAV;
    if (strcmp(ext, ".avi") == 0) return MSVIDEO;
    if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return V_MPEG;
    if (strcmp(ext, ".mp3") == 0) return A_MPEG;
    return NULL;
}

char* getContentType(int type)
{
    switch(type)
    {
        case HTML:
            return "text/html";
        case JPEG:
            return "image/jpeg";
        case GIF:
            return "image/gif";
        case PNG:
            return "image/png";
        case CSS:
            return "text/css";
        case BASIC:
            return "audio/basic";
        case WAV:
            return "audio/wav";
        case MSVIDEO:
            return "video/x-msvideo";
        case V_MPEG:
            return "video/mpeg";
        case A_MPEG:
            return "audio/mpeg";
        default:
            return "text/html";
    }
}

int handleRequest(int socket)
{
    char buffer[256];
    int n = 0, length = 0;

    char* req = NULL;
    char *line;
    char *method = NULL;
    char* path;
    char* http;
    int i=0;
    const char s[2] = " ";
    char *token;

    if (socket < 0)
        error("ERROR on accept");

    while(1)
    {
        n = read(socket, buffer, 255);
        printf("this is n %d", n);

        if(!n) {
            break;
        }
        length = req ? strlen(req) : 0;
        req=(char*)realloc(req, sizeof(char)*(strlen(buffer)+length+1));

        req = strcat(req, buffer);
        if(n < 255) break;
        if (n < 0) error("ERROR reading from socket");
        bzero(buffer, 256);
        // printf("HI :)");
    }

    //write(socket,getString(),strlen(getString()));

    length = strchr(req, '\r')-req;
    line = (char*) malloc(sizeof(char)*(length+1)); //freeIt
    line = strncat(line, req, length);
    path = strchr(line,' ');
    *path = 0;
    path+=2;
    h->path = path;
    http = path+strlen(path)-8;
    if(*http != 'H')
    {
        h->statusCode = 400;
    }// all good
    *http = 0;
    http++;
    http = strchr(http,'/');
    http++;
    if(h->statusCode != 0 && strcmp(http,"1.1")!=0)
    {
        h->statusCode = 400;
    }

    if(h->statusCode != 0 && strcmp(line,"GET")!=0)
    {
        h->statusCode = 501;
    }

    if(h->statusCode == 0)
    {
        checkDir(path);
    }

    // TODO check the path if exists and respon ddue to it.
    printf("Here is the message: %s\n", buffer);
    //n = write(socket, "I got your message", 18);
    char* msg = responseMsg();
     write(socket,msg,strlen(msg));
    close(socket);

    if (n < 0) error("ERROR writing to socket");
    return 0;

}

char* responseMsg( )
{
    int body = (getLength()+strlen(statusCode(h->statusCode))*2+strlen(getStatusMsg(h->statusCode)));
    char* arr= (char*)malloc(sizeof(char)*(body+500));
    if(h->statusCode!=302) {
        sprintf(arr, "HTTP/1.0 %s\r\n"
                     "Server: webserver/1.0\r\n"
                     "Date:%s\r\n"
                     "%s"
                     "Content-Type:%s\r\n"
                     "Content-Length: %d\r\n"
                     "Connection: close\r\n"
                     "\n"
                     "<HTML><HEAD><TITLE>%s</TITLE></HEAD>\r\n"
                     "<BODY><H4>%s</H4>\r\n"
                     "%s.\r\n"
                     "</BODY></HTML>", statusCode(h->statusCode), getDate(), statusCode(h->statusCode),
                getContentType(h->typefile), body, statusCode(h->statusCode), statusCode(h->statusCode),
                getStatusMsg(h->statusCode));
    }else{
        sprintf(arr, "HTTP/1.0 %s\r\n"
                     "Server: webserver/1.0\r\n"
                     "Date:%s\r\n"
                     "%s"
                     "Location:%s/"
                     "Content-Type:%s\r\n"
                     "Content-Length: %d\r\n"
                     "Connection: close\r\n"
                     "\n"
                     "<HTML><HEAD><TITLE>%s</TITLE></HEAD>\r\n"
                     "<BODY><H4>%s</H4>\r\n"
                     "%s.\r\n"
                     "</BODY></HTML>", statusCode(h->statusCode), getDate(), h->path, statusCode(h->statusCode),
                getContentType(h->typefile), body, statusCode(h->statusCode), statusCode(h->statusCode),
                getStatusMsg(h->statusCode));}
    return arr;
}




char* getLocation()
{
    //? "Location: " + h->path + "\\" : ""
    if(h->statusCode==302)
    {

        sprintf(statusCode(302),"%s\nLocation:%s\\",(statusCode(302),h->path));
        return statusCode(302);
    }

    return"";
}


int getLength(){
    return strlen("<HTML><HEAD><TITLE></TITLE></HEAD>\r\n"
                                  "<BODY><H4></H4>\r\n"
                                  ".\r\n"
                                  "</BODY></HTML>");
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

char* getStatusMsg(int status)
{
    switch(status){
        case 302:
        {
            return "Directories must end with a slash";
        }
        case 400:
        {
            return "Bad Request";
        }
        case 403:
        {
            return "Forbidden";
        }
        case 404:
        {
            return "Not Found";
        }
        case 500:
        {
            return "Internal Server Error";
        }
        case 501:
        {
            return "Not supported";
        }
        default:
        {
            perror("code not found");
            return NULL;
        }
    }
}

char* createResponse()
{

}
