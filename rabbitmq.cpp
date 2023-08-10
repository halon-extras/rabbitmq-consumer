#include <HalonMTA.h>
#include <string>
#include <thread>
#include <list>
#include <mutex>
#include <map>
#include <condition_variable>
#include <unistd.h>
#include <syslog.h>
#include "configuration.hpp"
#include "connection.hpp"
#include "message.hpp"

std::list<std::shared_ptr<AMPQInjector>> injectors;
std::list<std::thread> workers;
bool stop = false;

void InjectorWorker(std::shared_ptr<AMPQInjector> injector);
struct InjectCallbackPtr
{
    size_t delivery_tag;
    ssize_t* ack;
    std::mutex* mutex;
    std::condition_variable* cv;
};
void InjectCallback(HalonInjectResultContext* hirc, void* ptr);

HALON_EXPORT
int Halon_version()
{
    return HALONMTA_PLUGIN_VERSION;
}

HALON_EXPORT
bool Halon_init(HalonInitContext* hic)
{
    HalonConfig* cfg = nullptr;
    HalonMTA_init_getinfo(hic, HALONMTA_INIT_CONFIG, nullptr, 0, &cfg, nullptr);
    if (!cfg)
        return false;
    
    return ParseConfig(cfg, injectors);
}

HALON_EXPORT
void Halon_ready(HalonReadyContext* hrc)
{
    for (auto & injector : injectors)
    {
        for (size_t i = 0; i < injector->threads; ++i)
        {
            workers.push_back(std::thread([injector] () {
                pthread_setname_np(pthread_self(), std::string("p/rmq/" + injector->id).substr(0, 15).c_str());
                InjectorWorker(injector);
            }));
        }
    }
}

HALON_EXPORT
void Halon_early_cleanup()
{
    stop = true;
    for (auto & t : workers)
        t.join();
}

void InjectorWorker(std::shared_ptr<AMPQInjector> injector)
{
    std::condition_variable cv;
    std::mutex mutex;
    ssize_t ack = 0;
    InjectCallbackPtr* icptr = nullptr;

    amqp_connection_state_t conn = nullptr;
    while (!stop)
    {
        if (!conn)
        {
            std::string error;
            conn = open_connection(injector->connection.hostname.c_str(),
                                    injector->connection.port,
                                    injector->connection.connect_timeout,
                                    injector->connection.vhost.c_str(),
                                    injector->connection.username.c_str(),
                                    injector->connection.password.c_str(),
                                    injector->connection.tls,
                                    injector->connection.tls_verify_peer,
                                    injector->connection.tls_verify_host,
                                    error);
            if (!conn)
            {
                syslog(LOG_CRIT, "RabbitMQ: Connect failed: %s", error.c_str());
                sleep(10);
                continue;
            }
        }

        amqp_basic_consume(conn, 1,
            amqp_cstring_bytes(injector->connection.queue.c_str()),
            amqp_cstring_bytes(injector->connection.consumer_tag.c_str()),
            false, !injector->connection.ack, false, amqp_empty_table);

        struct timeval tv = { 1, 0 };
        while (true)
        {
            std::unique_lock<std::mutex> ul(mutex);
            cv.wait(ul, [icptr, &ack] () { return stop || !icptr || (icptr && ack != 0); });
            if (ack)
            {
                icptr = nullptr;
                int res = injector->connection.ack ? (
                        ack > 0 ? amqp_basic_ack(conn, 1, ack, false) : amqp_basic_reject(conn, 1, -ack, false)
                    ) : 0;
                ack = 0;
                if (res != 0)
                {
                    syslog(LOG_CRIT, "RabbitMQ: failed to send ack");
                    amqp_destroy_connection(conn);
                    conn = nullptr;
                    break;
                }
            }
            if (stop)
            {
                if (!icptr)
                    break;
                continue;
            }

            amqp_frame_t frame;
            int res = amqp_simple_wait_frame_noblock(conn, &frame, &tv);
            if (res != AMQP_STATUS_OK)
            {
                if (res == AMQP_STATUS_TIMEOUT)
                    continue;
                amqp_destroy_connection(conn);
                conn = nullptr;
                break;
            }
            if (frame.frame_type != AMQP_FRAME_METHOD || frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD)
            {
                amqp_destroy_connection(conn);
                conn = nullptr;
                break;
            }

            icptr = new InjectCallbackPtr;
            icptr->delivery_tag = ((amqp_basic_deliver_t *)frame.payload.method.decoded)->delivery_tag;
            icptr->ack = &ack;
            icptr->mutex = &mutex;
            icptr->cv = &cv;

            auto hic = HalonMTA_inject_new("rabbitmq");
            HalonMTA_inject_callback_set(hic, InjectCallback, icptr);
            if (ParsePacket(hic, conn))
                HalonMTA_inject_commit(hic);
            else
            {
                HalonMTA_inject_abort(hic);
                delete icptr;
                icptr = nullptr;

                amqp_destroy_connection(conn);
                conn = nullptr;
                break;
            }
            amqp_maybe_release_buffers(conn);
        }
    }
    if (conn)
    {
        amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
        amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(conn);
    }
}

void InjectCallback(HalonInjectResultContext* hirc, void* ptr)
{
    auto icp = (InjectCallbackPtr*)ptr;
    icp->mutex->lock();
    short int code;
    HalonMTA_inject_result_getinfo(hirc, HALONMTA_RESULT_CODE, nullptr, 0, &code, nullptr);
    if (code / 100 == 4)
        *icp->ack = -icp->delivery_tag;
    else
        *icp->ack = icp->delivery_tag;
    icp->mutex->unlock();
    icp->cv->notify_one();
    delete icp;
}