#include "server.h"

/*!
 * \brief Faz um parse os parametros do terminal para encontrar a porta e o
 * diretorio onde o servidor sera montado e escreve em uma estrutura
 *
 * \param[in] n_params Numero de parametros vindos do terminal
 * \param[in] dir_path Caminho do diretorio
 * \param[in] port Porta que o servidor ira escutar
 * \param[in] rate Rate de velocidade em bytes/segundo
 * \param[out] s_info Estrutura onde as informacoes serao salvas
 *
 * \return 0 se OK
 * \return -1 se der algum erro
 */

int parse_and_fill_server_info(int n_params, char *dir_path, char *port,
                               char *rate, server_info *s_info)
{
  if (n_params != 4)
  {
    fprintf(stderr, "Bad parameters\nUsage: server [DIR_PATH] [PORT]"
            "[RATE]\n");
    return -1;
  }

  strncpy(s_info->port, port, MAX_PORT_LEN);

  if ((atoi(s_info->port)) > MAX_PORT)
  {
    fprintf(stderr, "Bad port\nPort Number has to be lower than 65535\n"
                    "Usage: server [DIR_PATH] [PORT] [RATE]\n");
    return -1;
  }

  strncpy(s_info->dir_path, dir_path, PATH_MAX);
  s_info->client_rate =  strtod(rate, NULL);
  if (errno == ERANGE)
  {
    fprintf(stderr, "Bad rate number\nn"
                    "Usage: server [DIR_PATH] [PORT] [RATE]\n");
    return -1;
  }

  s_info->max_clients = NCLIENTS;

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

int server_start_listen(const server_info *s_info)
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

  freeaddrinfo(servinfo);
  return listenfd;
}

int server_socket_init()
{
  int sockfd;
  struct sockaddr_un server_addr;

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, "/tmp/SERVER_SOCKET");
  sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
  unlink("/tmp/SERVER_SOCKET");
  bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  return sockfd;
}

/*!
 * \brief Faz inicializacoes de variaveis na estrutura de clientes e do
 * servidor
 *
 * \param[in] listenfd Socket de escuta do servidor
 * \param[out] cli_list Estrutura de clientes, sendo que a posicao
 * SERVER_INDEX
 * no client armazena o socket de escuta do servidor
 */

void client_list_init(client_list *cli_list, int listenfd, int max_clients,
                      int sockfd)
{
  int i;
  memset(cli_list, 0, sizeof(*cli_list));
  cli_list->client = calloc(max_clients, sizeof(struct pollfd));
  cli_list->client[SERVER_INDEX].fd = listenfd;
  cli_list->client[SERVER_INDEX].events = POLLIN;
  cli_list->client[LOCAL_SOCKET].fd = sockfd;
  cli_list->client[LOCAL_SOCKET].events = POLLIN;
  for (i = LOCAL_SOCKET + 1; i < max_clients; i++)
    cli_list->client[i].fd = -1;
}

/*!
 * \brief Dobra a quantidade de clientes que podem ser alocados
 *
 * \param[out] cli_list Lista de clientes
 * \param[out] s_info Dados de configuracao do servidor
 */

static void inc_max_clients(client_list *cli_list, server_info *s_info)
{
  int i;

  s_info->max_clients *= 2;
  cli_list->client = realloc(cli_list->client,
                             s_info->max_clients * sizeof(*cli_list->client));

  for (i = cli_list->list_len + 1; i < s_info->max_clients; i++)
    cli_list->client[i].fd = -1;

}

/*!
 * \brief Divide pela metade a quantidade de clientes que podem ser alocados
 *
 * \param[out] cli_list Lista de clientes
 * \param[out] s_info Dados de configuracao do servidor
 */

static void dec_max_clients(client_list *cli_list, server_info *s_info)
{
  s_info->max_clients /= 2;
  cli_list->client = realloc(cli_list->client,
                             s_info->max_clients * sizeof(*cli_list->client));
}

void get_thread_msg(client_list *cli_list)
{
  int num_bytes, sockfd, i;
  void *s_msg = NULL;
  client_info *cli;

  num_bytes = read(cli_list->client[LOCAL_SOCKET].fd, &s_msg,
                   sizeof(cli_list->head));
  cli = (client_info *)s_msg;
  sockfd = cli->sockfd;

  for (i = 2; i <= cli_list->list_len + 2; i++)
  {
    if (sockfd == cli_list->client[i].fd)
    {
      cli_list->client[i].events = POLLIN | POLLOUT;
      cli->thread_finished = true;
      break;
    }
  }
 //  if(method = put)
   //  cli_info->bytes_read = 0;
}


/*!
 * \brief Realiza um shift para a esquerda da estrutura de clientes para
 * deixar o vetor client paralelo a lista de clientes quando um node é
 * deletado
 *
 * param[in] cli_num posicao do vetor client que foi deletada
 * param[out] cli_list Lista de clientes
 */

static void shift_client_list(client_list *cli_list, int cli_num)
{
  memmove(&cli_list->client[cli_num], &cli_list->client[cli_num + 1],
         (cli_list->list_len - cli_num + 1) * sizeof(*cli_list->client));
}

static int set_client_struct(client_info *cli_info, client_list *cli_list,
                             int cli_num)
{
  cli_info->buffer = (char *) calloc(BUFSIZE, sizeof(char));
  if (cli_info->buffer == NULL)
    return -1;
  cli_info->is_ready = true;
  cli_info->thread_finished = true;
  cli_info->sockfd = cli_list->client[cli_num].fd;

  if (cli_info->bucket.rate > BUFSIZE)
    cli_info->bucket.to_be_consumed_tokens = BUFSIZE - 1;
  else
    cli_info->bucket.to_be_consumed_tokens = cli_info->bucket.rate;

  return 0;
}
/*!
 * \brief Adiciona um node a lista de clientes, checa a quantidade de clientes
 * e chama uma funcao para desalocar ou alocar mais espaco.
 * param[in] s_info Dados de configuracao do servidor
 * param[out] cli_list Lista de clientes
 *
 * return cli_info node atual
 * return NULL se nao conseguir alocar o node
 */

static client_info *list_add(client_list *cli_list, server_info *s_info)
{
  int cli_num;
  client_info *cli_info;

  cli_num = cli_list->list_len + 2;

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

  if (cli_list->list_len == s_info->max_clients)
    inc_max_clients(cli_list, s_info);
  else if (cli_list->list_len == (s_info->max_clients/4 &&
           cli_list->list_len > NCLIENTS))
    dec_max_clients(cli_list, s_info);

  if ((set_nonblock(cli_list->client[cli_num].fd)) == -1)
  {
    close(cli_list->client[cli_num].fd);
    shift_client_list(cli_list, cli_num);
    return NULL;
  }

  bucket_init(&cli_info->bucket, 50000, 50000, s_info->client_rate);

  if (set_client_struct(cli_info, cli_list, cli_num) == -1)
    return NULL;

  return cli_info;
}

/*!
 * \brief Seta socket como nonblocking
 *
 * param[out] sockfd socket que sera setado como nonblock
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

int check_connection(client_list *cli_list, int listenfd, server_info *s_info)
{
  int connfd, cli_num;
  socklen_t clilen;
  struct sockaddr_storage cli_addr;

  cli_num = cli_list->list_len + 2;

  clilen = sizeof(cli_addr);
  if((connfd = accept(listenfd, (struct sockaddr *)&cli_addr,
                        &clilen)) == -1)
    return -1;

  cli_list->client[cli_num].fd = connfd;
  cli_list->client[cli_num].events = POLLIN;

  if(!(list_add(cli_list, s_info)))
  {
    close(cli_list->client[cli_num].fd);
    shift_client_list(cli_list, cli_num);
    return -1;
  }

  return 0;
}

/*!
 * \brief Funcao para desalocar o buffer de um node e fechar o descritor de
 * arquivos
 *
 * param[out] cli_info Node atual
 */

static void clean_struct(client_info *cli_info)
{
  if (cli_info->fp)
    fclose(cli_info->fp);

  free(cli_info->buffer);
}

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
  clean_struct(cli_info);
  close(cli_list->client[cli_num].fd);
  shift_client_list(cli_list, cli_num);
  memset(&cli_list->client[cli_list->list_len + 1], 0,
         sizeof(*cli_list->client));
  cli_list->client[cli_list->list_len + 1].fd = -1;
  list_del(cli_info, cli_list);
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

int check_request(client_info *cli_info, server_info *s_info)
{
  switch (cli_info->method)
  {
    case GET:

    if (strncmp(cli_info->file_path, s_info->dir_path,
              strlen(s_info->dir_path)) != 0)
      return cli_info->request_status = FORBIDDEN;

    if (access(cli_info->file_path, R_OK) == 0)
      return cli_info->request_status = OK;
    else
      return cli_info->request_status = NOT_FOUND;

    break;

    case PUT:

    if (strncmp(cli_info->file_path, s_info->dir_path,
              strlen(s_info->dir_path)) != 0)
      return cli_info->request_status = FORBIDDEN;
    else
      return cli_info->request_status = ACCEPTED;

    break;
  }

  return -1;
}

static int check_header_end(client_info *cli_info)
{
  int num_bytes_header = 0;
  char *header_aux;

  if ((header_aux =  strstr(cli_info->buffer,"\r\n\r\n")) != NULL)
  {
    num_bytes_header = header_aux - cli_info->buffer + 4;
    cli_info->bytes_read -= num_bytes_header;
    return num_bytes_header;
  }

 if ((header_aux =  strstr(cli_info->buffer,"\n\n")) != NULL)
  {
    num_bytes_header = header_aux - cli_info->buffer + 2;
    cli_info->bytes_read -= num_bytes_header;
    return num_bytes_header;
  }

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

static int parse_http_request(client_info *cli_info, const char *dir_path)
{
  int ret;
  char path[PATH_MAX], method[MAX_METHOD_LEN];

  strcpy(path, dir_path);
  strcat(path, "/");

  if ((ret = sscanf(cli_info->buffer, "%"STR_METHOD"s %"STR_PATH"s",
                    method, path + strlen(path))) != 2)
    return -1;

  realpath(path, cli_info->file_path);
  if (strcmp(method, "PUT") == 0)
    cli_info->method = PUT;
  if (strcmp(method, "GET") == 0)
    cli_info->method = GET;


  return 0;
}

static int get_client_data(client_info *cli_info)
{
  int nread = 0;

  if ((nread = recv(cli_info->sockfd, cli_info->buffer, BUFSIZE - 1, 0)) < 0)
  {
    if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
      return -2;
    else
      return -1;
  }

  if (nread <= 0)
    return -1;

  cli_info->bytes_read = nread;

  return 0;
}

static int read_data(client_info *cli_info, thread_pool *t_pool)
{
  if (!cli_info->header_sent)
  {
    if (get_client_data(cli_info) == -1)
      return -1;
    cli_info->header_size = check_header_end(cli_info);
    return 0;
  }

  if (cli_info->bytes_read != 0)
    return 0;

  switch (cli_info->method)
  {
  case GET:
    add_job(t_pool, read_filedata, cli_info);
    return 0;
    break;
  case PUT:
    if (get_client_data(cli_info) == -1)
      return -1;
    return 1;
    break;
  }
}

/*!
 * \brief Faz o processamento do request HTTP
 *
 * param[in] sockfd socket connectado
 * param[in] cli_info Node atual
 * param[in] dir_path path do diretorio raiz do servidor
 *
 * return ret 0 para tudo ok, -1 para erro no request -2 caso socket esteja
 * vazio
 *
 */

int process_http_request(client_info *cli_info, const char *dir_path,
                         thread_pool *t_pool)
{
  int ret;

  ret = read_data(cli_info, t_pool);

  if (ret == -1)
    return ret;

  if (ret == 0)
    return parse_http_request(cli_info, dir_path);

  return ret;
}

/*!
 * \brief Abre o arquivo de acordo com o path do node atual
 *
 * param[out] cli_info Node atual
 */
//FALTA CHECAR SE O ARQUIVO JA ESTA SENDO ENVIANDO PARA NAO FAZER PUT EM CIMA
//DELE OU TENTAR DAR GET EM UM PUT

int open_file(client_info *cli_info)
{
  if (cli_info->method == GET)
  {
    cli_info->fp = fopen(cli_info->file_path, "rb");
    if (cli_info->fp == NULL)
      return -1;
  }
  else if (cli_info->method == PUT)
  {
    cli_info->fp = fopen(cli_info->file_path, "w");
    if (cli_info->fp == NULL)
      return -1;
  }

  return 0;
}

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
    case ACCEPTED:
      return "ACCEPTED";
      break;
  }

  return NULL;
}


/*!
 * \brief Seta events como 0 caso nao possa enviar dados
 * *
 * \param[in] cli_list Lista de clientes
 * \param[in] cli_info node de clientes atual
 * \param[in] cli_num posicao do vetor do pollfd
 *
 */

void set_clients(client_info *cli_info, client_list *cli_list, int cli_num)
{
  cli_info->is_ready = bucket_check(&cli_info->bucket);

  if (!cli_info->is_ready)
    cli_list->client[cli_num].events = 0;
  else if (cli_info->header_sent && cli_info->thread_finished)
    cli_list->client[cli_num].events = POLLIN | POLLOUT;


}
/*!
 * \brief Escreve o header da mensagem de resposta de uma conexao
 *
 * param[in] status numero do status http
 * param[out] cli_info Node atual
 *
 */

static void build_header(client_info *cli_info, int status)
{
    snprintf(cli_info->header, sizeof(cli_info->header),
    "HTTP/1.0 %d %s\r\n\r\n", status, status_msg(status));
}

/*!
 * \brief Le os dados do arquivo do path do node e escreve no  buffer do node
 *
 * param[out] cli_info Node atual
 *
 */

void read_filedata(void *cli_info)
{
  client_info *client = (client_info *)cli_info;

  client->bytes_read = fread(client->buffer, 1,
  client->bucket.to_be_consumed_tokens, client->fp);

}



void write_client_data (void *cli_info)
{
  client_info *client = (client_info *)cli_info;

  fwrite(client->buffer + client->header_size, 1, client->bytes_read,
         client->fp);
}

static void end_job(client_list *cli_list, client_info *cli_info, int cli_num)
{
  cli_list->client[cli_num].events = 0;
  cli_info->thread_finished = false;
}


static int write_data(client_info *cli_info, server_info *s_info,
                      thread_pool *t_pool)
{
  int ret;

  if (!cli_info->header_sent)
  {
    if (cli_info->incomplete_send == 0)
    {
      build_header(cli_info, check_request(cli_info, s_info));
      if (cli_info->request_status > ACCEPTED)
        return -1;
    }
    ret = send(cli_info->sockfd, cli_info->header + cli_info->incomplete_send,
               ARRAY_LEN(cli_info->header) - cli_info->incomplete_send,
               MSG_NOSIGNAL);

    if (ret < 0)
    {
      if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        return 0;
      else
        return -1;
    }

    if (ret != ARRAY_LEN(cli_info->header))
    {
      cli_info->incomplete_send = ret;
      return 0;
    }

    if (ret > 0)
    {
      cli_info->header_sent = true;
      open_file(cli_info);
      return 0;
    }
  }
  else
  {
    switch(cli_info->method)
    {
    case GET:
      ret = send(cli_info->sockfd, cli_info->buffer +
      cli_info->incomplete_send, cli_info->bytes_read -
      cli_info->incomplete_send, MSG_NOSIGNAL);

      if (ret < 0)
      {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
          return 0;
        else
          return -1;
      }

      if (ret != cli_info->bytes_read)
      {
        cli_info->incomplete_send = ret;
        return 0;
      }
      return 1;
      break;
    case PUT:
      add_job(t_pool, write_client_data, cli_info);
      cli_info->header_size = 0;
      cli_info->incomplete_send = 0;
      return 0;
      break;
    }
  }

  return 0;
}
/*!
 * \brief Chama as funcoes para envio da resposta http (header) caso seja
 * necessario, checa o bucket para ver se é possível enviar, caso seja, faz o
 * consumo dos tokens e envia os dados.
 *
 * param[in] cli_info Node atual
 * param[in] s_info Estrutura com dados do servidor
 * param[in] sockfd socket do client conectado
 *
 * return 0 se o envio for correto e nao tiver terminado
 * return -1 se der algum erro ou tiver terminado o envio
 */
int process_bucket_and_data(client_info *cli_info, server_info *s_info,
                            thread_pool *t_pool, client_list *cli_list,
                            int cli_num)
{
  cli_info->is_ready = bucket_check(&cli_info->bucket);

  if (!cli_info->is_ready)
    return 0;

  if (!cli_info->header_sent)
  {
    write_data(cli_info, s_info, t_pool);
   // return 0;
  }

  if (cli_info->thread_finished)
  {
    int read_ret = 0, write_ret = 0;

    read_ret = read_data(cli_info, t_pool);
    write_ret = write_data(cli_info, s_info, t_pool);
    end_job(cli_list, cli_info, cli_num);

    if (read_ret == 1 || write_ret == 1)
      bucket_consume(&cli_info->bucket);
  }

  return 0;
}

/*!
 * \brief Funcao para calcular o tempo de espera do poll, o wait e' o maior
 * tempo da lista de clientes
 *
 * \param[in] cli_list Lista de clientes
 *
 * \return t_wait tempo a ser esperado pelo poll
 */

struct timespec find_poll_wait_time(client_info *cli_info,
                                    struct timespec t_wait)
{
  struct timespec t_wait_aux;
  memset(&t_wait_aux, 0, sizeof(t_wait_aux));

  if (!cli_info->is_ready)
  {
    t_wait_aux = bucket_wait(&cli_info->bucket);

    if (timespec_isset(&t_wait) == 0)
      t_wait = bucket_wait(&cli_info->bucket);
    else if (timespec_compare(&t_wait, &t_wait_aux) < 0)
      t_wait = t_wait_aux;
  }

  return t_wait;
}


/*!
 * \brief Faz o cleanup caso o programa se receba um sinal
 *
 * \param[in] cli_list Lista de clientes
 *
 */
void cleanup(client_list *cli_list)
{
  client_info *cli_info, *curr_cli;

  cli_info = cli_list->head;

  while (cli_info)
  {
    curr_cli = cli_info;
    clean_struct(cli_info);
    cli_info = cli_info->next;
    free(curr_cli);
  }
  free(cli_list->client);
}

