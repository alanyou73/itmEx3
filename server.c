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
#include <sys/stat.h>

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define USAGE "server <port> <pool-size> <max-number-of-request>"

enum contentType
{
    HTML, JPEG, GIF, PNG, CSS, BASIC, WAV, MSVIDEO, V_MPEG, A_MPEG
};

typedef struct response
{
    char** filesNames;
    char* path;
    char* body;
    int typefile;
    int filesCount;
    int statusCode;
    int fileFound;
    size_t bodySize;
    FILE *file;

}response;



response *init()
{
    response *r = (response*)malloc(sizeof(response));
    r->filesNames=NULL;
    r->path = NULL;
    r->body = NULL;
    r->typefile= 0;
    r->statusCode = 0;
    r->filesCount = 0;
    r->fileFound = 0;
    r->file=NULL;
    r->bodySize = 0;
    return r;
}
char* buidTable(response *h);
char* statusCode(int status);
char* getDate();
int checkDir(response *h,int sock);
void error(char *msg);
int get_mime_type(char *name);
int handleRequest(int socket);
char* responseMsg(response *h, int socket);
char* getStatusMsg(int status);
int getLength();
void getLocation();
char* buildErrorMessage(response* h);
char* buildSuccessMessage(response* h, int socket);
void getFileContent(response* h,int sock);
void responseMessage(response* h,int sock);
char* printRows(response *h, int socket);
char* lastTimeModified(response *h , char *path);
int getFileSize(response *h , char *name);
char* getContentType(int type);

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

int checkDir(response *h, int sock)
{

    DIR *d;
    struct dirent *dir;
    char* temp = NULL;

    if(h->path[strlen(h->path)-1] == ' ')
    {
        h->path[strlen(h->path)-1] = 0;
    }

    d = opendir(h->path);
    if (d)
    {
        //printf("%c",path[strlen(path)-1]);
        if(h->path[strlen(h->path)-1] != '/')
        {
            h->statusCode=302;
            h->typefile = HTML;
            getLocation(h);
        }
        else
        {
            h->statusCode = 200;
            while ((dir = readdir(d)) != NULL && h->fileFound == 0) {
                if (strcmp(dir->d_name, "index.html") == 0)
                {
                    temp = (char*) malloc(sizeof(char)*(strlen(h->path)+strlen("index.html/")+1));
                    temp = strcpy(temp, h->path);
                    temp = strcat(temp, "index.html/");
                    h->path = temp;
                    getFileContent(h,sock);
                }
            }
        }


        closedir(d);
    }

    else
    {
        getFileContent(h,sock);
    }
}

void getFileContent(response* h,int sock)
{
    FILE* file = NULL;
    size_t size = 1;
    char get_char = 0;
     char buffer[512] = ""; // TODO!!!! CHeck if it falls on another compiler
    h->fileFound = 1; // Index found - rise flag
    h->typefile = get_mime_type(h->path);
    puts((h->typefile == HTML ||h->typefile == CSS) ?"r":"rb");
    file=fopen(h->path, (h->typefile == HTML ||h->typefile == CSS) ?"r":"rb");
    if(file==NULL)
    {
        h->statusCode = 404;
        return; // stands for ERROR
    }

    h->statusCode = 200;
//    fseek(file, 0, SEEK_SET );
//
//    do {
//        get_char = fgetc(file);
//        if( feof(file) ) {
//            break ;
//        }
//        size++;
//        h->body = (char*) realloc(h->body, sizeof(char) * (size));
//        h->body[size-2] = get_char;
//        printf("%c", c);
//    } while(1);
//    while((get_char=fgetc(file))!= EOF)
//    {
//        size++;
//        h->body = (char*) realloc(h->body, sizeof(char) * (size));
//        h->body[size-2] = get_char;
//    }
//    h->body[size-1] = 0;

    struct stat buf;
    stat(h->path, &buf);
    h->bodySize = buf.st_size;

//    h->bodySize = size;


//    fseek(file, 0, SEEK_END);
//    size = ftell(file);
    fseek(file, 0, SEEK_SET);
//    printf("%d, and %d \n", ftell(file), ftell(file));
//    h->body = (char*)malloc(sizeof(char)*(500));
//    sprintf(h->body, "HTTP/1.0 %s\r\n"
//                     "Server: webserver/1.0\r\n"
//                     "Date: %s\r\n"
//                     "Content-Type: %s\r\n"
//                     "Content-Length: %d\r\n"
//                     "Last-Modified: Wed, 27 Oct 2010 06:50:29 GMT"
//                     "Connection: close\r\n"
//                     "\n", statusCode(h->statusCode), getDate(),
//            getContentType(h->typefile), strlen(bodyList));
    h->body = (char*) malloc(sizeof(char) * (h->bodySize + 501));
    sprintf(h->body, "HTTP/1.0 %s\r\n"
                     "Server: webserver/1.0\r\n"
                     "Date: %s\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %d\r\n"
                     "Last-Modified: %s"
                     "Connection: close\r\n"
                     "\n", statusCode(h->statusCode), getDate(),
            getContentType(h->typefile), h->bodySize, lastTimeModified(h, h->path));

//    fread(h->body,h->bodySize+1, 1, file);
//    fgets(h->body, size+1, file);

//
     while(fgets(buffer, 512, file) != NULL)
     {


        fread(buffer,512, 1, file);

//     write(sock,&buffer,strlen(buffer));
//     write(sock,buffer,512);
//        h->body = (char*) realloc(h->body, sizeof(char) * (strlen(buffer)+strlen(h->body)));
        strcat(h->body, buffer);
    }
    puts("image is: %s\n");
    puts(h->body);
    fclose(file);

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

    response* h = init();
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

    if( strcmp(line,"GET")!=0)
    {
        h->statusCode = 501;
    }

    if(h->statusCode == 0)
    {
        checkDir(h,socket);
    }

    // TODO check the path if exists and respon ddue to it.
    printf("Here is the message: %s\n", buffer);
    //n = write(socket, "I got your message", 18);
    char* msg = responseMsg(h, socket);
    write(socket,msg,strlen(msg));
//    responseMessage(h, socket);
    close(socket);

    if (n < 0) error("ERROR writing to socket");
    return 0;

}

void responseMessage(response* h, int socket)
{}

char* responseMsg(response *h, int socket )
{
    return h->statusCode == 200 ? buildSuccessMessage(h, socket) : buildErrorMessage(h);
//}
//    if(h->statusCode!=200) {
//        return buildErrorMessage(h);
//    }else{

//    return arr;
}

char* buildErrorMessage(response* h)
{
    int bodyLen = (getLength()+strlen(statusCode(h->statusCode))*2+strlen(getStatusMsg(h->statusCode)));
    char* arr= (char*)malloc(sizeof(char)*(bodyLen+500)+strlen(statusCode(h->statusCode)));
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
                 "</BODY></HTML>", statusCode(h->statusCode), getDate(), h->statusCode == 302 ? h->path : "",
            getContentType(h->typefile), bodyLen, statusCode(h->statusCode), statusCode(h->statusCode),
            getStatusMsg(h->statusCode));

    return arr;
}

char* buildSuccessMessage(response* h, int socket)
{
    //int bodyLen = 0;
    char* arr = NULL;
    struct dirent *dir;
    char* wrapper;
    //(char*)malloc(sizeof(char)*(bodyLen+500));

    if(!h->fileFound)
    {
//         ;

        arr = printRows(h, socket);
        return arr;
    }
    return h->body;


}

char* printRows(response *h, int socket)
{
    DIR *d;
    struct dirent *dir;
    char* path = NULL;
    char * lastModified = NULL;
    char buffer[1024]="";

    char* bodyList =  (char*)malloc(sizeof(char)*(strlen(h->path)*2+154));
     sprintf(bodyList,"<HTML>\n"
                "<HEAD><TITLE>Index of %s</TITLE></HEAD>\n"
                "\n"
                "<BODY>\n"
                "<H4>Index of %s</H4>\n"
                "\n"
                "<table CELLSPACING=8>\n"
                "<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n"
                "\n"
                "\n", h->path, h->path);
    d = opendir(h->path);
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, "..") != 0 && strcmp(dir->d_name, ".") != 0) {
//            h->filesCount++;
//            h->filesNames = (char **) realloc(h->filesNames, sizeof(char *) * (h->filesCount));
//            h->filesNames[h->filesCount - 1] = (char *) malloc(sizeof(char) * (strlen(dir->d_name) + 1));
//            strcpy(h->filesNames[h->filesCount - 1], dir->d_name);
            bzero(buffer,1024);
            path = (char*) malloc(sizeof(char) * (strlen(dir->d_name)+strlen(h->path) + 1));

            path = strcpy(path, h->path);
            path = strcat(path, dir->d_name);
            lastModified = lastTimeModified(h, path);
            sprintf( buffer,"<tr>\n"
                            "<td><A HREF=\"%s\">%s</A></td><td>%s</td>\n"
                            "<td>%d</td>\n"
                            "</tr>\n"
                            , dir->d_name, dir->d_name, lastModified, getFileSize(h, dir->d_name));

            bodyList=(char*)realloc(bodyList,sizeof(char)*(strlen(bodyList)+strlen(buffer)+1));
            strcat(bodyList,buffer);
            free(lastModified);
            free(path);
            path=NULL;
            lastModified = NULL;
        }
    }
    bodyList=(char*)realloc(bodyList,sizeof(char)*strlen(bodyList)+62);

    strcat(bodyList,"</table>\n"
                    "<HR>\n"
                    "<ADDRESS>webserver/1.0</ADDRESS>\n"
                    "</BODY></HTML>");

    h->body = (char*)malloc(sizeof(char)*(500));
    sprintf(h->body, "HTTP/1.0 %s\r\n"
                      "Server: webserver/1.0\r\n"
                      "Date: %s\r\n"
                      "Content-Type: %s\r\n"
                      "Content-Length: %d\r\n"
                      "Last-Modified: Wed, 27 Oct 2010 06:50:29 GMT"
                      "Connection: close\r\n"
                      "\n", statusCode(h->statusCode), getDate(),
            getContentType(h->typefile), strlen(bodyList));
//    write(socket, h->body, strlen(h->body), )
    h->body = (char*) realloc(h->body, sizeof(char) * (strlen(h->body) + strlen(bodyList) + 1));
//
    h->body = strcat(h->body, bodyList);
    return h->body;

    //return bodyList;

//     int i = 0;
//     while(h->filesCount>0)
//     {
//        d = opendir(h->path);
//
//
//        sprintf( buffer,"<tr>\n"
//         "<td><A HREF=\"%s\"><%s></A></td><td>%s></td>\n"
//         "<td><if entity is a file, add file size, otherwise, leave empty></td>\n"
//         "</tr>\n"
//         "</table>\n"
//         "<HR>\n"
//         "<ADDRESS>webserver/1.0</ADDRESS>\n"
//         "</BODY></HTML>",h->filesNames[i],h->filesNames[i]);
//     }
}

void getLocation(response *h)
{
    char* temp = (char*) malloc(sizeof(char) * (strlen(h->path) + strlen("Location: /\r\n") + 1));
    sprintf(temp, "Location: %s/\r\n", h->path);
    h->path = temp;
    puts(temp);
    puts(h->path);
    //? "Location: " + h->path + "\\" : ""
//    if(h->statusCode==302)
//    {
//
//        sprintf(statusCode(302),"%s\nLocation:%s\\",(statusCode(302),h->path));
//        return statusCode(302);
//    }
//
//    return"";
}

char* buildBody(response* h)
{

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
        case 200:
        {
            return "200 OK";
        }
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

/*char* createResponse()
{

}*/

char* lastTimeModified(response *h , char * path) {

    char *res = (char*) malloc(sizeof(char) * 1024);
    struct stat sb;

    if (stat(path, &sb)!= -1 ) {
        bzero(res, 128);
        strftime(res, 128, RFC1123FMT, gmtime(&(sb.st_mtime)));
    }

    return res;
}

int getFileSize(response *h , char *name)
{
    int size = 0;
    struct stat sb;
    char* path = (char*) malloc(sizeof(char) * (strlen(name)+strlen(h->path) + 1));
    path = strcpy(path, h->path);
    path = strcat(path, name);
//    int stat = ;
//    S_ISREG(path_stat.st_moded)
    if (stat(path, &sb)!= -1 && S_ISREG(sb.st_mode)) {
        size = sb.st_size;
    }
    free(path);
    path = NULL;
    return size;
}
