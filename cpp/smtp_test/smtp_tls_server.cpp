#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <signal.h>

#define PORT 465

// Prototipo per cleanup_openssl, se definita dopo
void cleanup_openssl();

int sock = -1;
SSL_CTX *ctx = nullptr;

void sigint_handler(int signum)
{
    std::cout << "\nRicevuto SIGINT, chiudo server...\n";
    if (sock > 0) close(sock);
    if (ctx) SSL_CTX_free(ctx);
    cleanup_openssl();
    exit(0);
}
void init_openssl()
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
    EVP_cleanup();
}

SSL_CTX* create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        std::cerr << "Unable to create SSL context\n";
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_context(SSL_CTX *ctx)
{
    // Carica certificato e chiave privata (file PEM)
    if (SSL_CTX_use_certificate_file(ctx, "/home/sergio/certificati_ssl/baldoweb.it/www_baldoweb_it.crt", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "/home/sergio/certificati_ssl/baldoweb.it/www_baldoweb_it.key", SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

int main()
{
    signal(SIGINT, sigint_handler);
    
    struct sockaddr_in addr;

    init_openssl();
    ctx = create_context();

    configure_context(ctx);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 1) < 0)
    {
        perror("listen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    std::cout << "SMTP TLS server listening on port " << PORT << "\n";

    while (1)
    {
        struct sockaddr_in client_addr;
        uint len = sizeof(client_addr);
        int client = accept(sock, (struct sockaddr*)&client_addr, &len);
        if (client < 0)
        {
            perror("accept");
            continue;
        }

        std::cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) << "\n";

        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0)
        {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client);
            continue;
        }

        // Invia banner SMTP
        const char *banner = "220 SimpleSMTP TLS Server Ready\r\n";
        SSL_write(ssl, banner, strlen(banner));

        char buffer[1024];
        bool quit = false;

        while (!quit)
        {
            memset(buffer, 0, sizeof(buffer));
            int bytes = SSL_read(ssl, buffer, sizeof(buffer)-1);
            if (bytes <= 0)
            {
                int err = SSL_get_error(ssl, bytes);
                if (err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_SYSCALL)
                {
                    std::cout << "Client disconnected\n";
                    break;
                }
                ERR_print_errors_fp(stderr);
                break;
            }

            std::string cmd(buffer);
            std::cout << "Received: " << cmd;

            // Risposte base
            if (cmd.find("EHLO") == 0 || cmd.find("HELO") == 0)
            {
                const char *response =
                    "250-SimpleSMTP\r\n"
                    "250-PIPELINING\r\n"
                    "250-SIZE 10240000\r\n"
                    "250-STARTTLS\r\n"
                    "250 OK\r\n";
                SSL_write(ssl, response, strlen(response));
            }
            else if (cmd.find("QUIT") == 0)
            {
                const char *response = "221 Bye\r\n";
                SSL_write(ssl, response, strlen(response));
                quit = true;
            }
            else if (cmd.find("MAIL FROM:") == 0)
            {
                const char *response = "250 OK\r\n";
                SSL_write(ssl, response, strlen(response));
            }
            else if (cmd.find("RCPT TO:") == 0)
            {
                const char *response = "250 OK\r\n";
                SSL_write(ssl, response, strlen(response));
            }
            else if (cmd.find("DATA") == 0)
            {
                const char *response = "354 End data with <CR><LF>.<CR><LF>\r\n";
                SSL_write(ssl, response, strlen(response));

                std::string data;
                while (true)
                {
                    memset(buffer, 0, sizeof(buffer));
                    int n = SSL_read(ssl, buffer, sizeof(buffer)-1);
                    if (n <= 0) break;
                    data.append(buffer, n);
                    if (data.find("\r\n.\r\n") != std::string::npos)
                        break;
                }
                std::cout << "Message data:\n" << data << "\n";

                const char *response2 = "250 OK: message accepted\r\n";
                SSL_write(ssl, response2, strlen(response2));
            }
            else
            {
                const char *response = "502 Command not implemented\r\n";
                SSL_write(ssl, response, strlen(response));
            }
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
    }

    close(sock);
    SSL_CTX_free(ctx);
    cleanup_openssl();

    return 0;
}
