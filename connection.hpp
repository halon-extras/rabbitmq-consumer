#ifndef CONNECTION_HPP_
#define CONNECTION_HPP_

#include <amqp.h>
#include <string>
#include <set>

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
);

#endif