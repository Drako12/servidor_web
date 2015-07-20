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

/*!
 * \brief Faz inicializacoes de variaveis na estrutura de clientes e do
 * servidor
 *
 * \param[in] listenfd Socket de escuta do servidor
 * \param[out] cli_list Estrutura de clientes, sendo que a posicao SERVER_INDEX
 * no client armazena o socket de escuta do servidor
 */

void client_list_init(client_list *cli_list, int listenfd, int max_clients)
{
  int i;  
  memset(cli_list, 0, sizeof(*cli_list));
  cli_list->client = calloc(max_clients, sizeof(struct pollfd));
  cli_list->client[SERVER_INDEX].fd = listenfd;
  cli_list->client[SERVER_INDEX].events = POLLIN;
  for (i = SERVER_INDEX + 1; i < max_clients; i++)
    cli_list->client[i].fd = -1;
}

static void inc_max_clients(client_list *cli_list, server_info *s_info)
{
  int i;

  s_info->max_clients *= 2;
  cli_list->client = realloc(cli_list->client,
                             s_info->max_clients * sizeof(*cli_list->client));
  
  for (i = cli_list->list_len + 1; i < s_info->max_clients; i++)
    cli_list->client[i].fd = -1;
  
}

static void dec_max_clients(client_list *cli_list, server_info *s_info)
{
  s_info->max_clients /= 2;
  cli_list->client = realloc(cli_list->client, 
                             s_info->max_clients * sizeof(*cli_list->client));
}



/*!
 * \brief Adiciona um node a lista de clientes
 *
 * param[out] cli_list Lista de clientes
 *
 * return cli_info node atual
 * return NULL se nao conseguir alocar o node
 */

static client_info *list_add(client_list *cli_list, server_info *s_info)
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
  
  if (cli_list->list_len == s_info->max_clients)
    inc_max_clients(cli_list, s_info);
  else if (cli_list->list_len == (s_info->max_clients/4 && 
           cli_list->list_len > NCLIENTS))  
    dec_max_clients(cli_list, s_info); 
  
  cli_info->can_send = true;

  bucket_init(&cli_info->bucket, 0, 50000, s_info->client_rate);
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
  int i, connfd;
  socklen_t clilen;
  struct sockaddr_storage cli_addr;

    clilen = sizeof(cli_addr);
    if((connfd = accept(listenfd, (struct sockaddr *)&cli_addr,
                        &clilen)) == -1)
      return -1;

    for (i = cli_list->list_len; i < s_info->max_clients; i++)
    {
      if (cli_list->client[i].fd == -1)
      {
        cli_list->client[i].fd = connfd;
        if ((set_nonblock(cli_list->client[i].fd)) == -1)
        {
          close(cli_list->client[i].fd);
          shift_client_list(cli_list, i);
          return -1;
        }
        cli_list->client[i].events = POLLIN;
        if(!(list_add(cli_list, s_info)))
        {
          close(cli_list->client[i].fd);
          shift_client_list(cli_list, i);
          return -1;
        }
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

static int get_http_request(client_info *cli_info, int sockfd)
{
  int num_bytes_read = 0;
  int num_bytes_aux = 0;

  cli_info->buffer = (char *) calloc(BUFSIZE, sizeof(char));
  if (cli_info->buffer == NULL)
    return -1;

  while (num_bytes_aux < BUFSIZE - 1)
  {
    if ((num_bytes_read = recv(sockfd, cli_info->buffer + num_bytes_aux,
                               BUFSIZE - num_bytes_aux - 1, 0)) < 0)
    {
      if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
        return -2;
      else
        return -1;
    }

    if ((strstr(cli_info->buffer, "\r\n\r\n")) != NULL ||
       (strstr(cli_info->buffer, "\n\n")) != NULL)
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

int check_request(client_info *cli_info, server_info *s_info)
{
  if (strncmp(cli_info->file_path, s_info->dir_path,
              strlen(s_info->dir_path)) != 0)
    return cli_info->request_status = FORBIDDEN;

  if (access(cli_info->file_path, R_OK) == 0)
    return cli_info->request_status = OK;

  else
    return cli_info->request_status = NOT_FOUND;

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


  return 0;
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
                         int sockfd)
{
  int ret;

  ret = get_http_request(cli_info, sockfd);

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

void open_file(client_info *cli_info)
{
  cli_info->fp = fopen(cli_info->file_path, "rb");
  //if (cli_info->fp == NULL)
  //  return -1;
//  return 0;
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
  }

  return NULL;
}


/*!
 * \brief Envia o header da mensagem de resposta de uma conexao
 *
 * param[in] sockfd Socket conectado
 * param[out] cli_info Node atual
 *
 * return 0 se tudo der certo
 * return -1 se der algum erro
 */
static int send_http_response(client_info *cli_info, int sockfd, int status)
{
  char response_msg[HEADERSIZE];

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

static int get_filedata(client_info *cli_info)
{
  int num_bytes_read = 0;

  if (cli_info->bucket.rate > BUFSIZE)
    cli_info->bucket.to_be_consumed_tokens = BUFSIZE - 1;
  else
    cli_info->bucket.to_be_consumed_tokens = cli_info->bucket.rate;

  num_bytes_read = fread(cli_info->buffer, 1,
  cli_info->bucket.to_be_consumed_tokens, cli_info->fp);

  if (num_bytes_read < 0)
    return -1;

  return num_bytes_read;
}

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
static int send_requested_data(client_info *cli_info, int sockfd)
{
  int ret;
  ret = send(sockfd, cli_info->buffer + cli_info->incomplete_send,
             cli_info->bucket.to_be_consumed_tokens - cli_info->incomplete_send,
             MSG_NOSIGNAL);

  if (ret < 0)
  {
    if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
      return 0;
    else
      return -1;
  }

  if (ret != cli_info->bucket.to_be_consumed_tokens)
  {
    cli_info->incomplete_send = ret;
    return 0;
  }
  cli_info->incomplete_send = 0;

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

int process_bucket_and_send_data(client_info *cli_info, server_info *s_info,
                                 int sockfd)
{
  int ret;

  if (cli_info->header_sent == false)
  {
    ret = send_http_response(cli_info, sockfd, check_request(cli_info,
                             s_info));
    if (ret == -1 || cli_info->request_status != OK)
      return -1;

    open_file(cli_info);
  }
  else
  {
    cli_info->can_send = bucket_check(&cli_info->bucket);

    if (cli_info->can_send == true)
    {
      if (cli_info->incomplete_send == 0)
      {
        cli_info->bucket.to_be_consumed_tokens = get_filedata(cli_info);
        bucket_consume(&cli_info->bucket, cli_info->bucket.to_be_consumed_tokens);
      }
      ret = send_requested_data(cli_info, sockfd);
      if (ret == -1)
        return ret;
    }

    if (feof(cli_info->fp))
      return -1;
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

  if (!cli_info->can_send)
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
 * \brief Seta uma flag na estrutura de clientes para saber se pode ou nao
 * enviar dados
 *
 * \param[in] cli_list Lista de clientes
 * \param[in] cli_info node de clientes atual
 * \param[in] cli_num posicao do vetor do pollfd 
 *
 */

void set_clients(client_info *cli_info, client_list *cli_list, int cli_num)
{
  cli_info->can_send = bucket_check(&cli_info->bucket);

  if (!cli_info->can_send)
    cli_list->client[cli_num].events = 0;
  else if (cli_info->header_sent && cli_info->can_send)
    cli_list->client[cli_num].events = POLLOUT;

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

