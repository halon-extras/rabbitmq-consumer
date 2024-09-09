#include "configuration.hpp"
#include <cstring>
#include <stdexcept>

bool ParseConfig(HalonConfig* cfg, std::list<std::shared_ptr<AMPQInjector>>& injectors)
{
    auto queues = HalonMTA_config_object_get(cfg, "queues");
    if (queues)
    {
        size_t l = 0;
        HalonConfig* queue;
        while ((queue = HalonMTA_config_array_get(queues, l++)))
        {
            auto injector = std::make_shared<AMPQInjector>();

            const char* id = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "id"), nullptr);
            if (id) injector->id = id;
            const char* hostname = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "hostname"), nullptr);
            if (hostname) injector->connection.hostname = hostname;
            const char* port = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "port"), nullptr);
            if (port) injector->connection.port = (int)strtol(port, nullptr, 10);
            const char* connect_timeout = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "connect_timeout"), nullptr);
            if (connect_timeout) injector->connection.connect_timeout = (int)strtol(connect_timeout, nullptr, 10);
            const char* queue_ = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "queue"), nullptr);
            if (queue_) injector->connection.queue = queue_;
            const char* consumer_tag = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "consumer_tag"), nullptr);
            if (consumer_tag) injector->connection.consumer_tag = consumer_tag;
            const char* vhost = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "vhost"), nullptr);
            if (vhost) injector->connection.vhost = vhost;
            const char* username = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "username"), nullptr);
            if (username) injector->connection.username = username;
            const char* password = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "password"), nullptr);
            if (password) injector->connection.password = password;
            const char* ack = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "ack"), nullptr);
            if (ack && strcmp(ack, "true") != 0) injector->connection.ack = false;
            const char* tls = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "tls"), nullptr);
            if (tls && strcmp(tls, "false") != 0) injector->connection.tls = true;
            const char* tls_verify_peer = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "tls_verify_peer"), nullptr);
            if (tls_verify_peer && strcmp(tls_verify_peer, "false") != 0) injector->connection.tls_verify_peer = true;
            const char* tls_verify_host = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "tls_verify_host"), nullptr);
            if (tls_verify_host && strcmp(tls_verify_host, "false") != 0) injector->connection.tls_verify_host = true;
            const char* threads = HalonMTA_config_string_get(HalonMTA_config_object_get(queue, "threads"), nullptr);
            if (threads) injector->threads = strtoul(threads, nullptr, 10);

            if (!id || !queue_)
                throw std::runtime_error("missing required property");

            injectors.push_back(injector);
        }
    }

    return true;
}