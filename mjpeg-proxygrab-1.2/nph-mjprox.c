/*	Kenneth Lavrsen's mjpeg proxy for Motion: nph-mjprox
 *	This program is published under the GNU Public license
 *  Version 1.0 - 2003 November 19 - Initial Release
 *  Version 1.1 - 2003 November 22 - Greatly improved by Folkert van Heusden
 *    inspired by a suggestion from Torben Jensen
 *  Version 1.2 - 2004 April 02 - Fix for old versions of gcc provided by
 *    Christophe Grenier 
 *  
 *	The program opens a socket to a running Motion webcam server
 *	It then fetches the mjpeg stream from the Motion webcam server
 *	and sends it to standard out (browser).
 *	The idea is that the browser gets the stream from the standard http
 *	port 80 so that company firewalls does not block the stream.
 *	Additionally this program enables the webserver to be on one IP address
 *	and motion running on another IP address and an applet such as
 *	Cambozola will still work because it gets the stream from the webserver.
 *
 *	This program is made as a cgi program for for example an Apache server
 *	It runs as a nph (direct to browser) program. It takes one variable
 *	which is the camera (Motion thread number).
 *	The program must be copied to a directory from which cgi scripts can run.
 *	It must be have access rights set as executable for the web server.
 *	The program must be named with the prefix nph- .
 *	To use it with for example Cambozola add this to an HTML page
 *
 *	<applet code=com.charliemouse.cambozola.Viewer
 *		archive=cambozola6.jar width=325 height=245>
 *		<param name=url value="/cgi-bin/nph-mjprox?1">
 *		<param name="accessories" value="none"/>
 *	</applet> 
 *
 *  Note the number after the "?". This is the camera number (Motion thread no.)
 *
 *	Below the 3 #defines must be set before you build the program.
 *	You need to set IP address of the motion server, the portbase
 *	and the upper limit for the number of cameras you have.
 *	IP address 127.0.0.1 means local host (same machine).
 *	PORTBASE is the number from which the webcam port is calculated.
 *	The calculated port = PORTBASE + Camera Number (thread number).
 *	So if thread 1 has the webcam port set to 8081 the PORTBASE should be 8080
 *	Program can be built with this command:
 *	gcc -Wall -O3 -o nph-mjprox nph-mjprox.c
 */

#include <sys/ioctl.h>
#include <net/if.h> 
#include <unistd.h>
#include <errno.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/*  User defined parameters - EDIT HERE */
#define IPADDR "127.0.0.1" /* IP address of motion server, 127.0.0.1 = localhost */
#define PORTBASE 8080      /* If your camera 1 is on port 8081 set this to one lower = 8080 */
#define UPPERCAMERANUMBER 12  /* Set this to your upper limit of cameras */
#define MAXLEN 80
#define EXTRA 5
/* 4 for field name "data", 1 for "=" */
#define MAXINPUT MAXLEN+EXTRA+2
/* 1 for added line break, 1 for trailing NUL */
#define DATAFILE "../data/data.txt"


int get_token();
void unencode();
unsigned char get_mac();

int main()
{
  int authorized = 0;
  char user[30] = "init";
  char password[30] = "init";
  char mac[30] = "init";  
  char *data;
  data = getenv("QUERY_STRING");
  char *p;
  char tmp[30]="temp";
  
  printf("HTTP/1.1 200 OK\n");
  printf("Content-Type: text/html\n\n");
  printf("<html>\n");
  printf("<form action='/cgi-bin/nph-mjprox' METHOD='GET'>\n");
  printf("<div><label>Username: <input name='user' size='5'></label></div>\n");
  printf("<div><label>Password: <input name='password' size='5'></label></div>\n");
  printf("<div><input type='submit' value='Login'></div>\n");
  printf("</form>\n");

  p = strtok (data,"&");
  while (p != NULL)
  {
    strcpy (tmp,p);    
    if (strncmp(tmp, "user=", 5) == 0) { 
      sscanf(tmp,"user=%s",user);      
    }
    if (strncmp(tmp, "password=", 9) == 0) { 
      sscanf(tmp,"password=%s",password);      
    }    
    p = strtok (NULL, "&");
  }

  printf("USER: %s | ",user);
  printf("PASSWORD: %s",password);  
  unsigned char mac_address = ' ';
  mac_address = get_mac();
  printf("mac: %d",mac_address);
  //get_token();

  if (strncmp(password, "1234", 4) == 0) { 
    printf("AUTHORIZED!!");
    authorized=1;    
  }  
  printf("</html>\n"); 
  if (authorized){
	int sockfd;
	int len;
	int cameranumber;
	struct sockaddr_in address;
	int result;
	char *querystr;
	char *dummy;
	char buffer[65536];
/*	Get camera number from querystring  */
	cameranumber=1;
	querystr = getenv("QUERY_STRING");
	sscanf(querystr,"%d",&cameranumber);
	if (cameranumber<1 || cameranumber>UPPERCAMERANUMBER)
	{
		exit(0);
	}

/*  Create a socket for the client.  */

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

/*  Name the socket, as agreed with the server.  */

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(IPADDR);
	address.sin_port = htons(PORTBASE + cameranumber);
	len = sizeof(address);

/*  Now connect our socket to the server's socket.  */

	result = connect(sockfd, (struct sockaddr *)&address, len);

	if(result == -1)
	{
		perror("oops: Cannot connect to Motion");
		exit(1);
	}

/*  We can now read/write via sockfd.  */

	for(;;)
	{
		int n;
		dummy = buffer;

		/* read as much as is possible for a read
		* if less is available: don't worry, read will return
		* what is available
		*/
		n = read(sockfd, buffer, 65536);

		/* read failed? */
		if (n == -1)
		{
			/* read was interrupted? then just do it
			* again
			*/
			if (errno == EINTR)
				continue;

			exit(1);
		}
		/* n=0 means socket is closed */
		if (n == 0)
			break;

		/* send received buffer to stdout descriptor
		* again: as much as possible so that you have
		* not so much buffers
		*/
		while(n > 0)
		{
			int nsend;
			/* send some */
			nsend = write(1, dummy, n);

			if (nsend == -1)
			{
				if (errno == EINTR)
					continue;

				exit(1);
			}
			/* strange: stdout is close?! */
			else if (nsend == 0)
				exit(1);

			/* keep track of what was send */
			dummy += nsend;
			n -= nsend;
		}
	}

	close(sockfd);
	exit(0);
}
return 0;
}

unsigned char get_mac()
{
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];
    int success = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) { /* handle error*/ };

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) { /* handle error */ }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    success = 1;
                    break;
                }
            }
        }
        else { /* handle error */ }
    }

    unsigned char mac_address[6];

    if (success) memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
}

//int set_user(char *user, char *password, char *mac)
int get_token(void)
{
  CURL *curl;
  CURLcode res;
  printf("--hit--");
  //printf(user);
 
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "http://pyfi.org");
    /* example.com is redirected, so we tell libcurl to follow redirection */ 
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
 
    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }
  return 0;
}

void unencode(char *src, char *last, char *dest)
{
 for(; src != last; src++, dest++)
   if(*src == '+')
     *dest = ' ';
   else if(*src == '%') {
     int code;
     if(sscanf(src+1, "%2x", &code) != 1) code = '?';
     *dest = code;
     src +=2; }     
   else
     *dest = *src;
 *dest = '\n';
 *++dest = '\0';
}

int post_data(void)
{
  printf("--hit post_data--");
char *lenstr;
char input[MAXINPUT], data[MAXINPUT];
long len;
printf("%s%c%c\n",
"Content-Type:text/html;charset=iso-8859-1",13,10);
printf("<TITLE>Response</TITLE>\n");
lenstr = getenv("CONTENT_LENGTH");
if(lenstr == NULL || sscanf(lenstr,"%ld",&len)!=1 || len > MAXLEN)
  printf("<P>Error in invocation - wrong FORM probably.");
else {
  FILE *f;
  fgets(input, len+1, stdin);
  unencode(input+EXTRA, input+len, data);
  f = fopen(DATAFILE, "a");
  if(f == NULL)
    printf("<P>Sorry, cannot store your data.");
  else
    fputs(data, f);
  fclose(f);
  printf("<P>Thank you! Your contribution has been stored.");
  }
return 0;
}
