#ifndef CONFIGURATION_HPP_
#define CONFIGURATION_HPP_

#include <HalonMTA.h>
#include <amqp.h>
#include <string>
#include <mutex>
#include <list>
#include <set>
#include <memory>
#include <condition_variable>

struct AMPQConnection
{
    std::string hostname = "localhost";
    int port = 5672;
    int connect_timeout = 10;
    std::string vhost = "/";
    std::string username = "guest";
    std::string password = "guest";
    std::string queue;
    std::string consumer_tag;
    bool ack = true;
    bool tls = false;
    bool tls_verify_peer = false;
    bool tls_verify_host = false;
};

struct AMPQInjector
{
    std::string id;
    AMPQConnection connection;
    size_t threads = 1;
};

bool ParseConfig(HalonConfig* cfg, std::list<std::shared_ptr<AMPQInjector>>& injectors);

#endif