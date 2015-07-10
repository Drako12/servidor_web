#include "server.h"

/*!
 * \brief Faz um parse os parametros do terminal para encontrar a porta e o
 * diretorio onde o servidor sera montado e escreve em uma estrutura
 *
 * \param[in] n_params Numero de parametros vindos do terminal
 * \param[in] dir_path Caminho do diretorio
 * \param[in] port Porta que o servidor ira escutar
 * \param[out] s_info Estrutura onde as informacoes serao salvas
 *
 * \return 0 se OK
 * \return -1 se der algum erro
 */

int parse_param(int n_params, char *dir_path, char *port, server_info *s_info)
{  
  if (n_params != 3)
  {
    fprintf(stderr, "Bad parameters\nUsage: server [DIR_PATH] [PORT]\n");
    return -1;
  }

  strncpy(s_info->port, port, MAX_PORT_LEN);    

  if ((atoi(s_info->port)) > MAX_PORT)
  {
    fprintf(stderr, "Bad port\nPort Number has to be lower than 65535\n"
                    "Usage: server [DIR_PATH] [PORT]\n");
    return -1;
  }

  strncpy(s_info->dir_path, dir_path, PATH_MAX);    
  
  return 0;
}

  /*!
   * \brief Inicia o socket de escuta para o servidor e faz o bind ao endereco 
   *
   * \param[in] s_info Estrutura com a porta e o path do diretorio do servidor
   * 
   * \return listenfd socket de escuta do servidor, se OK
   * \return -1 se der algum erro
   */

int server_start(const server_info *s_info)
{
  int enable = 1, listenfd = -1, ret = 0;
  struct addrinfo hints, *servinfo, *aux;
  
  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
    
  if ((ret = getaddrinfo(NULL, s_info->port, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "Getaddrinfo error: %s\n", gai_strerror(ret));
    return -1;
  }
  
 
  for (aux = servinfo; aux != NULL; aux = aux->ai_next)
  {
    if ((listenfd = socket(aux->ai_family, aux->ai_socktype,
                           aux->ai_protocol)) == -1)
    {
      fprintf(stderr, "Socket error: %s\n", strerror(errno));
      continue;
    }
    
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
   
    if (bind(listenfd, aux->ai_addr, aux->ai_addrlen) == -1)
    {
      close (listenfd);
      fprintf(stderr, "Bind error:%s\n",strerror(errno));
      continue;    
    }
  }

  if (listen(listenfd, MAX_LISTEN) == -1)
  {
    fprintf(stderr, "Listen error:%s\n",strerror(errno));
    return -1;    
  }
  
  
  return listenfd;
}

/*!
 * \brief Faz inicializacoes de variaveis na estrutura de clientes e do
 * servidor
 *
 * \param[in] listenfd Socket de escuta do servidor
 * \param[out] cli_list Estrutura de clientes, sendo que a posicao SERVER_INDEX
 * no client armazena o socket de escuta do servidor
 */

void server_init(client_list *cli_list, int listenfd, server_info *s_info)
{
  int i, ret, num_bytes_read = 0;
  server_cache *cache; 
  FILE *fp;

  if ((s_info->dir = opendir(s_info->dir_path)) != NULL)
  {
    while ((s_info->ent = readdir(s_info->dir)) != NULL)
    {
      if (s_info->ent->d_type == DT_REG)
      {        
        cache = calloc(1, sizeof(*cache));
        
        if(s_info->head == NULL)
          s_info->head = cache;
        strcpy(cache->filename, s_info->dir_path);
        strcat(cache->filename, s_info->ent->d_name);
        fp = fopen(cache->filename, "rb");
        fseek(fp, 0, SEEK_END);
        cache->file_size = ftell(fp);
        rewind(fp);
        cache->file = malloc(cache->file_size); 
        
        while(num_bytes_read < cache->file_size)
        {
          ret = fread(cache->file, 1, cache->file_size, fp);
          num_bytes_read += ret;
        }
        cache = cache->next;
      }
    }    
  } 
  // fprintf(stderr,"%s\n", s_info->ent->d_name);
   closedir(s_info->dir);
     
  memset(cli_list, 0, sizeof(*cli_list));   
  cli_list->client[SERVER_INDEX].fd = listenfd;
  cli_list->client[SERVER_INDEX].events = POLLIN;
  for (i = SERVER_INDEX + 1; i < MAX_CLIENTS; i++)
    cli_list->client[i].fd = -1;
}

//reset das configuracoes do poll na posicao do servidor

void reset_poll(client_list *cli_list, int listenfd)
{
  cli_list->client[0].fd = listenfd;
  cli_list->client[0].events = POLLIN;
  cli_list->client[0].revents = 0;
}

/*!
 * \brief Adiciona um node a lista de clientes
 *
 * param[out] cli_list Lista de clientes
 *
 * return cli_info node atual 
 * return NULL se nao conseguir alocar o node
 */

static client_info *list_add(client_list *cli_list)
{
  client_info *cli_info;
  
  cli_info = calloc(1, sizeof(*cli_info));
  if (cli_info == NULL)
    return NULL;
    
  if (cli_list->head == NULL)
  cli_list->head = cli_info;
  else
  {
    client_info *i = cli_list->head;
    while (i->next)
      i = i->next;
    i->next = cli_info;
  }
  cli_list->list_len++;
  return cli_info; 
}

/*!
 * \brief Realiza um shift para a esquerda da estrutura de clientes para
 * deixar o vetor client paralelo a lista de clientes quando um node é deletado
 *
 * param[in] cli_num posicao do vetor client que foi deletada
 * param[out] cli_list Lista de clientes
 */

static void shift_client_list(client_list *cli_list, int cli_num)
{
  memmove(&cli_list->client[cli_num], &cli_list->client[cli_num + 1],
         (cli_list->list_len - cli_num) * sizeof(*cli_list->client));
}

/*!
 * \brief Seta socket como nonblocking
 *
 * param[out] sockfd socket que sera setado como nonblock
 * 
 */

int set_nonblock(int sockfd)
{
  int flags;

  if ((flags = fcntl(sockfd, F_GETFL, 0)) == -1)
    flags = 0;
   
  return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

/*!
 * \brief Faz as tarefas necessarias para adicionar um novo cliente a lista, 
 * usa a lista de conexoes pendentes para aceitar a primeira conexao,  cria um
 * novo socket conectado que 'e entao amazenado na estrutura de clientes
 *
 * param[in] listenfd socket de escuta do servidor
 * param[out] cli_list Lista de clientes
 *
 * return 0 se tudo for ok
 * return -1 se der algum erro
 */

int check_connection(client_list *cli_list, int listenfd)
{
  int i, connfd;
  socklen_t clilen;
  struct sockaddr_storage cli_addr;
    
  clilen = sizeof(cli_addr);
  if ((connfd = accept(listenfd, (struct sockaddr *)&cli_addr,
    &clilen)) == -1)
  {
    return -1;
  }
  
  for (i = SERVER_INDEX + 1; i < MAX_CLIENTS; i++)
  {
    if (cli_list->client[i].fd < 0)
    {
      cli_list->client[i].fd = connfd;
      if ((set_nonblock(cli_list->client[i].fd)) == -1)
      {
        close(cli_list->client[i].fd);
        shift_client_list(cli_list, i); // ou so seto fd = -1 ?
        return -1;
      }
      cli_list->client[i].events = POLLIN;
      if (!(list_add(cli_list)))
        fprintf(stderr,"Add to list error");
      break;
    }
  }
  return 0;
}

/*!
* \brief Funcao para desalocar o buffer de um node e fechar o descritor de
* arquivos
*
* param[out] cli_info Node atual
*/
/*
static void clean_struct(client_info *cli_info)
{ 
  if (cli_info->fp)
  fclose(cli_info->fp);
  free(cli_info->header);
}*/

/*!
 * \brief Deleta um node da lista de clientes
 *
 * param[out] cli_list Lista de clientes
 * param[out] cli_info Node atual
 */

static void list_del(client_info *cli_info, client_list *cli_list)
{
  if (cli_info == cli_list->head)
  {
    cli_list->head = cli_info->next;
    free(cli_info);   
  }
  else
  {
    client_info *current_cli = cli_info;
    client_info *i = cli_list->head;
    
    while (i->next)
    {
      if (i->next == current_cli)
      {
        i->next = current_cli->next;
        i = i->next;
        free(current_cli);
        break;
      }
      i = i->next;
    }  
  }
  cli_list->list_len--;
}


/*!
 * \brief Realiza as tarefas para fechar uma conexao, desaloca os recursos,
 * fecha os descritores/sockets, faz o shift do vetor de clientes, reseta as
 * estruturas e deleta o node da lista
 *
 * param[in] cli_num posicao do cliente na lista que sera removido
 * param[out] cli_list Lista de clientes
 * param[out] cli_info node a ser removido
 */

void close_connection(client_info *cli_info, client_list *cli_list,
                      int cli_num)
{   
  free(cli_info->header);
  //clean_struct(cli_info);
  close(cli_list->client[cli_num].fd);
  shift_client_list(cli_list, cli_num);
  memset(&cli_list->client[cli_list->list_len], 0, sizeof(*cli_list->client));
  cli_list->client[cli_list->list_len].fd = -1;
  list_del(cli_info, cli_list);
}

/*!
 * \brief Le o HTTP request vindo de uma conexao e armazena na estrutura do 
 * node de clientes 
 *
 * param[in] sockfd socket connectado
 * param[out] cli_info Node atual
 *
 * return 0 se tudo der certo 
 * return -1 se der algum erro
 */

int get_http_request(int sockfd, client_info *cli_info)
{     
  int num_bytes_read = 0;
  int num_bytes_aux = 0;
  
  cli_info->header = (char *) calloc(BUFSIZE, sizeof(char));
  if (cli_info->header == NULL)
    return -1;
    
  while (num_bytes_aux < BUFSIZE - 1)
  {   
    if ((num_bytes_read = recv(sockfd, cli_info->header + num_bytes_aux,
                              BUFSIZE - num_bytes_aux - 1, 0)) < 0)
    {
      if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
        return -2;
      else
        return -1;
    }
                         
    if ((strstr(cli_info->header, "\r\n\r\n")) != NULL ||
       (strstr(cli_info->header, "\n\n")) != NULL)
    break;

    if (num_bytes_read <= 0)
      return -1;
     
    num_bytes_aux += num_bytes_read;     
  }

  return 0;
}

/*!
 * \brief Checa o request e retorna o valor do status HTTP 
 *
 * param[in] path Path vindo do client
 * param[in] realpath Path resolvido pelo realpath
 * param[in] path do servidor
 *
 * return status HTTP
 * return -1 se der algum erro
 */
static int check_request(const char *realpath, const char *server_dir)
{
  if (strcmp(realpath, server_dir) < 0)
    return FORBIDDEN;    
  if (access(realpath, R_OK) == 0)
    return OK;
  else
    return NOT_FOUND;
    
  return -1;
}

/*!
 * \brief Faz o parse do HTTP request, resolve o path e chama a funcao para
 * preencher o status HTTP 
 *
 * param[in] dir_path path do diretorio raiz do servidor
 * param[out] cli_info Node atual
 *
 * return 0 se tudo der certo 
 * return -1 se der algum erro
 */
int parse_http_request(client_info *cli_info, const char *dir_path)
{
  int ret;
  char path[PATH_MAX], method[MAX_METHOD_LEN];

  strcpy(path, dir_path);
  strcat(path, "/");

  if ((ret = sscanf(cli_info->header, "%"STR_METHOD"s %"STR_PATH"s",
                    method, path + strlen(path))) != 2)
    return -1;
  
  realpath(path, cli_info->file_path); 
  cli_info->request_status = check_request(path, dir_path);  
  return 0;
} 

/*!
 * \brief Abre o arquivo de acordo com o path do node atual 
 *
 * param[out] cli_info Node atual
 */
/*
void open_file(client_info *cli_info)
{  
  cli_info->fp = fopen(cli_info->file_path, "rb");
  if (cli_info->fp == NULL)
    return -1;
   
  return 0;
}*/

/*!
 * \brief Funcao para transformar a mensagem de status em uma string  
 *
 * param[in] status Enum com os status
 *
 * return status se tudo der certo 
 * return NULL se der algum erro
 */

static char *status_msg(const http_code status)
{
  switch (status)
  {
    case OK:
      return "OK";
      break;

    case BAD_REQUEST:
      return "BAD REQUEST";
      break;

    case FORBIDDEN:
      return "FORBIDDEN";
      break;

    case NOT_FOUND:
      return "NOT FOUND";
      break;
    case INTERNAL_ERROR:
      return "INTERNAL ERROR";
      break;    
  }

  return NULL;
}

/*!
 * \brief Envia o header da mensagem de resposta de uma conexao
 *
 * param[in] sockfd Socket conectado
 * param[out] cli_info Node atual
 *
 * return 0 se tudo der certo ou o socket retornar erro
 * return -1 se der algum erro
 */
int send_http_response_header(int sockfd, client_info *cli_info)
{
  char response_msg[HEADERSIZE];
  int status = cli_info->request_status;
   
    snprintf(response_msg, sizeof(response_msg), "HTTP/1.0 %d %s\r\n\r\n",
             status, status_msg(status));

    if (send(sockfd, response_msg, strlen(response_msg), MSG_NOSIGNAL) < 0)
    {      
      if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
        return 0;
      else
        return -1;
    }

  cli_info->header_sent = true;
  return 0;
}

/*!
 * \brief Armazena os dados do arquivo do path do node no buffer do node
 *
 * param[out] cli_info Node atual
 *
 * return 0 se tudo der certo 
 * return -1 se der algum erro
 */
/*
int get_filedata(client_info *cli_info)
{
  int num_bytes_read = 0;
 
  num_bytes_read = fread(cli_info->buffer, 1, BUFSIZE, cli_info->fp);          
  if (num_bytes_read < 0)
    return -1;
  
  return num_bytes_read;
}*/

/*!
 * \brief Envia os dados do buffer do node para o client conectado 
 *
 * param[in] cli_info Node atual
 * param[in] num_bytes_read numero de bytes lidos do arquivo
 * param[in] sockfd socket do client conectado
 *
 * return 0 se tudo der certo 
 * return -1 se der algum erro
 */

int send_requested_data(client_info *cli_info, server_cache *cache,
                         int sockfd, server_info *s_info)
{
  int ret = 0;
  cache = s_info->head;
  while (cache)
  {
    if (strcmp(cache->filename, cli_info->file_path) == 0)  
    {
      cli_info->buffer = cache->file;
      if (cache->file_size < BUFSIZE + cli_info->bytes_sent)
        ret = send(sockfd, cli_info->buffer + cli_info->bytes_sent, 
                   cache->file_size - cli_info->bytes_sent, MSG_NOSIGNAL);
      else 
      ret = send(sockfd, cli_info->buffer + cli_info->bytes_sent, BUFSIZE,
                 MSG_NOSIGNAL); 
      if (ret < 0)
      {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
        return 0; 
        //fseek(cli_info->fp, -num_bytes_read, SEEK_CUR);       
        }
        else
          return -1;
      }
      cli_info->bytes_sent += ret;
      return 0;
    }
    cache = cache->next;
  }
return -1;
}
/*
int send_requested_data(client_info *cli_info, int num_bytes_read, 
                        int sockfd)
{
  int ret;
  ret = send(sockfd, cli_info->buffer, num_bytes_read, MSG_NOSIGNAL); 
  if (ret < 0)
  {
    if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
    {
      fprintf(stderr, "erro no socket\n%s", strerror(errno));
      fseek(cli_info->fp, -num_bytes_read, SEEK_CUR);       
    }
    else 
      return -1;
  }

  return 0;
}*/

