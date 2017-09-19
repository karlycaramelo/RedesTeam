#include "header.h"
#define CONN_MAX    1000                // Máximo número de clientes permitidos
#define BOLD_BLUE   "\e[1m\e[34m"       // Para colorear bonito los mensajes
#define BOLD_GREEN  "\e[1m\e[32m"
#define RESET       "\e[0m"
#define BYTES       1024


int listenfd, clients[CONN_MAX];
char *root;

int main(int argc, char *argv[]) {
  struct sockaddr_in cliaddr;
  socklen_t addrlen;
  int slot = 0, pid;
  char c;

  char *port = malloc(6);   // Dado que el máximo puerto es 65,535 5 para dígitos y uno para \0
  root = getenv("PWD");     // Usamos como directorio default el directorio actual
  strcpy(port, "9999");     // usamos como puerto default el 9999, porque por qué no

  // Procesar las opciones de linea de comandos
  while((c = getopt(argc, argv, "p:")) != -1)
    switch(c) {
      case 'p':            // Recibimos el puerto como parámetro con -p
        strcpy(port, optarg);
        break;
      case '?':
        fprintf(stderr, "Algo salio mal :/\n");
        exit(1);
      default: 
        exit(1);
    }
  

  printf(
      "Inicializando servidor en el puerto %s%s%s con raíz en el directorio %s%s%s\n", 
      BOLD_BLUE,        // coloritos bonitos
      port, 
      RESET,
      BOLD_BLUE,
      root,
      RESET
      );

  for(int i = 0; i < CONN_MAX; i++)
    clients[i] = -1;    // -1 significa que no hay clientes conectados

  start_server(atoi(port));

  while(1) {
    addrlen = sizeof(cliaddr);
    if((clients[slot] = accept(listenfd, (struct sockaddr *) &cliaddr, &addrlen)) < 0)
      error("Error on accept()");

    pid = fork();
 
    if(pid < 0)
      error("Error on fork()");
    
    if(pid == 0){
      respond(slot);  // respondemos a peticiones en otro proceso
      exit(0);
    }
    
    while(clients[slot] != -1) slot = (slot + 1) % CONN_MAX;
  }

  close(listenfd);
  free(port);
  free(root);
  return 0;
}

void start_server(int port) {
  struct sockaddr_in saddr;
  
  if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    error("Error on socket()");

  bzero((char *) &saddr, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htons(INADDR_ANY);
  saddr.sin_port = htons(port);

  if(bind(listenfd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0)
    error("Error on bind()");

  if(listen(listenfd, CONN_MAX * 10) != 0)
    error("Error on listen()");
}

void respond(int n) {

  // Si inicalizamos esto distion a 0 el servidor respondera como
  // si estuviera bajo mantenimiento
  int isUnderManteinance = 0;

  char request[99999], *req[3], path[99999];
  bzero(request, 99999);
  request_type req_type;

  if(read(clients[n], request, 99999) < 0){
    error("Error on read()");
    send_response(500, NULL, n);
  }
  printf("%s\n", request);

  if(strlen(request) > 99998)
    send_response(413, NULL, n);
  
  req[0] = strtok(request, " \t\n");
  req[1] = strtok(NULL, " \t");
  req[2] = strtok(NULL, " \t\r\n");
  
  if(strcmp(req[0], "GET") == 0)
    req_type = GET;
  else if(strcmp(req[0], "POST") == 0)
    req_type = POST;
  else {
    req_type = DEFAULT;
  }

  if(strcmp(req[2], "HTTP/1.0") != 0 && strcmp(req[2], "HTTP/1.1") != 0) {
    send_response(400, NULL, n);
    shutdown(clients[n], SHUT_RDWR);
    close(clients[n]);
    clients[n] = -1;
    return;
  }

  if (isUnderManteinance){
      send_response(503, NULL, n);
  }else{

  switch(req_type) {
    case GET:
      if(strcmp(req[1], "/brewcoffe") == 0){
          send_response(418, NULL, n);
	  break;
      }

      if(strcmp(req[1], "/gonefile.html") == 0){
          send_response(410, NULL, n);
	  break;
      }

      if(strcmp(req[1], "/ilegal.html") == 0){
          send_response(451, NULL, n);
	  break;
      }

      if(strcmp(req[1], "/") == 0)
          req[1] = "/index.html";
      strcpy(path, root);
      strcpy(&path[strlen(root)], req[1]);
      printf("file: %s\n", path);
      if(access(path, F_OK) != -1) {
        send_response(200, path, n);
      } else {
        send_response(404, NULL, n);
      }

      break;

    case POST:
      send_response(204, NULL, n);
      break;
    case DEFAULT:
      send_response(405, NULL, n);
      break;
  }
  }

  shutdown(clients[n], SHUT_RDWR);
  close(clients[n]);
  clients[n] = -1;
}

void send_response(int code, char *fname, int n) {
  char *file = NULL;

  switch(code) {
    case 200:
      write(clients[n], "HTTP/1.0 200 OK\r\n", 17);
      if(fname != NULL){
        write(clients[n], "Content-Type: text/html; charset=utf-8\r\n", 40);
        write(clients[n], "\r\n", 2);
        file = read_file_to_buffer(fname);
        write(clients[n], file, strlen(file));
      }
      break;
    case 204:
      write(clients[n], "HTTP/1.0 204 No Content\r\n", 25);
      break;
    case 404:
      write(clients[n], "HTTP/1.0 404 Not Found\r\n", 24);
      write(clients[n], "Content-Type: text/html; charset=utf-8\r\n", 40);
      write(clients[n], "\r\n", 2);
      file = read_file_to_buffer("404.html");
      write(clients[n], file, strlen(file));
      break;
    case 405: 
      write(clients[n], "HTTP/1.0 405 Method Not Allowed\r\n", 33);
      write(clients[n], "Allow: GET, POST\r\n", 18);
      break;
    case 410:
      write(clients[n], "HTTP/1.0 410 Gone\r\n", 19);
      write(clients[n], "Content-Type: text/html; charset=utf-8\r\n", 40);
      write(clients[n], "\r\n", 2);
      file = read_file_to_buffer("410.html");
      write(clients[n], file, strlen(file));
      break;
    case 413:
      write(clients[n], "HTTP/1.0 413 Request Entity Too Large\r\n", 39);
      break;
    case 418:
      write(clients[n], "HTTP/1.0 418  I'm a teapot\r\n", 28);
      write(clients[n], "Content-Type: text/html; charset=utf-8\r\n", 40);
      write(clients[n], "\r\n", 2);
      file = read_file_to_buffer("418.html");
      write(clients[n], file, strlen(file));
      break;
    case 451:
      write(clients[n], "HTTP/1.0 451 Unavailable For Legal Reasons\r\n", 44);
      write(clients[n], "Content-Type: text/html; charset=utf-8\r\n", 40);
      write(clients[n], "\r\n", 2);
      file = read_file_to_buffer("451.html");
      write(clients[n], file, strlen(file));
      break;
    case 500:
      write(clients[n], "HTTP/1.0 500 Internal Server Error\r\n", 36);
      break;
    case 503:
      write(clients[n], "HTTP/1.0 451 Service Unavailable\r\n", 34);
      write(clients[n], "Content-Type: text/html; charset=utf-8\r\n", 40);
      write(clients[n], "\r\n", 2);
      file = read_file_to_buffer("503.html");
      write(clients[n], file, strlen(file));
      break;
  }
  if(file != NULL)
    free(file);
}


void help(char *filename) {
  printf("Uso: %s%s [-p PUERTO] [-r DIRECTORIO]%s\n", BOLD_GREEN, filename, RESET);
}

char *read_file_to_buffer(char *filename) {
  FILE *fp;
  long size;
  char *buffer;

  if((fp = fopen(filename, "r")) == NULL)
    error("Error on fopen()");
  fseek(fp, 0L, SEEK_END);
  size = ftell(fp);
  rewind(fp);

  if((buffer = calloc(1, size+1)) == NULL){
    fclose(fp);
    error("Error con calloc() while reading file");
  }
  
  if(fread(buffer, size, 1, fp) != 1){
    fclose(fp);
    error("Error on fread() while reading file");
  }
  fclose(fp);
  return buffer;  // !! Recordar liberar esta memoria !!
}

void error(const char *msg) {
  perror(msg);
  exit(1);
}
