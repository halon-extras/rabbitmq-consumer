#include "connection.hpp"
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <amqp_ssl_socket.h>
#include <string>

amqp_connection_state_t open_connection(
    char const *hostname,
    int port,
    int connect_timeout,
    char const *vhost,
    char const *username,
    char const *password,
    bool tls_enabled,
    bool tls_verify_peer,
    bool tls_verify_host,
    std::string &error
) {
    amqp_connection_state_t conn = amqp_new_connection();
    if (!conn)
    {
        error = "failed to allocate and initialize connection object";
        return nullptr;
    }

    amqp_socket_t *socket = tls_enabled ? amqp_ssl_socket_new(conn) : amqp_tcp_socket_new(conn);
    if (!socket)
    {
        error = "failed to create tcp socket";
        amqp_destroy_connection(conn);
        return nullptr;
    }

    if (tls_enabled)
    {
        amqp_set_initialize_ssl_library(false);
        amqp_ssl_socket_set_verify_peer(socket, tls_verify_peer);
        amqp_ssl_socket_set_verify_hostname(socket, tls_verify_host);
    }

    struct timeval tv = { connect_timeout, 0 };

    int status = amqp_socket_open_noblock(socket, hostname, port, connect_timeout > 0 ? &tv : nullptr);
    if (status != AMQP_STATUS_OK)
    {
        error = "failed to open socket connection";
        amqp_destroy_connection(conn);
        return nullptr;
    }

    amqp_rpc_reply_t reply;
    reply = amqp_login(conn, vhost, 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, username, password);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL)
    {
        error = "failed to login to the broker";
        amqp_destroy_connection(conn);
        return nullptr;
    }

    amqp_channel_open(conn, 1);
    reply = amqp_get_rpc_reply(conn);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL)
    {
        error = "failed to open channel";
        amqp_destroy_connection(conn);
        return nullptr;
    }

    return conn;
}