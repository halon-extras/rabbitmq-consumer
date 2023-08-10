#include "message.hpp"
#include <vector>
#include <string>

std::vector<std::string> split(const std::string& str, const std::string& delim);

bool ParsePacket(HalonInjectContext* hic, amqp_connection_state_t conn)
{
    struct timeval tv = { 5, 0 };
    amqp_frame_t frame;

    int res = amqp_simple_wait_frame_noblock(conn, &frame, &tv);
    if (res != AMQP_STATUS_OK)
        return false;
    if (frame.frame_type != AMQP_FRAME_HEADER)
        return false;

    amqp_basic_properties_t* p = (amqp_basic_properties_t *) frame.payload.properties.decoded;
    for (int i = 0; i < p->headers.num_entries; ++i)
    {
        std::string key = std::string((char*)p->headers.entries[i].key.bytes, p->headers.entries[i].key.len);
        if (key == "sender")
        {
            if (p->headers.entries[i].value.kind != AMQP_FIELD_KIND_BYTES &&
                p->headers.entries[i].value.kind != AMQP_FIELD_KIND_UTF8)
                continue;
            amqp_bytes_t* foo = (amqp_bytes_t*)&(p->headers.entries[i].value.value);
            auto addr = split(std::string((char*)foo->bytes, foo->len), "@");
            HalonMTA_inject_sender_set(hic, addr[0].c_str(), addr.size() > 1 ? addr[1].c_str() : nullptr);
        }
        if (key == "recipients")
        {
            if (p->headers.entries[i].value.kind != AMQP_FIELD_KIND_ARRAY)
                continue;
            amqp_array_t* foo = (amqp_array_t*)&(p->headers.entries[i].value.value);
            for (int x = 0; x < foo->num_entries; ++x)
            {
                if (foo->entries[x].kind != AMQP_FIELD_KIND_BYTES &&
                    foo->entries[x].kind != AMQP_FIELD_KIND_UTF8)
                    continue;
                auto bar = (amqp_bytes_t)(&(foo->entries[x]))->value.bytes;
                auto addr = split(std::string((char*)bar.bytes, bar.len), "@");
                if (addr.size() == 2)
                    HalonMTA_inject_recipient_add(hic, addr[0].c_str(), addr.size() > 1 ? addr[1].c_str() : nullptr, nullptr);
            }
        }
        if (key == "metadata")
        {
            if (p->headers.entries[i].value.kind != AMQP_FIELD_KIND_BYTES &&
                p->headers.entries[i].value.kind != AMQP_FIELD_KIND_UTF8)
                continue;
            amqp_bytes_t* foo = (amqp_bytes_t*)&(p->headers.entries[i].value.value);
            HalonHSLValue* metadata;
            HalonMTA_inject_getinfo(hic, HALONMTA_MESSAGE_METADATA, nullptr, 0, &metadata, nullptr);
            char* err;
            size_t errlen;
            if (!HalonMTA_hsl_value_from_json(metadata, std::string((char*)foo->bytes, foo->len).c_str(), &err, &errlen))
                continue;
        }
    }

    size_t body_remaining = frame.payload.properties.body_size;
    while (body_remaining)
    {
        res = amqp_simple_wait_frame_noblock(conn, &frame, &tv);
        if (res != AMQP_STATUS_OK)
            return false;
        if (frame.frame_type != AMQP_FRAME_BODY)
            return false;
        if (!HalonMTA_inject_message_append(hic, (char*)frame.payload.body_fragment.bytes, frame.payload.body_fragment.len))
            return false;
        body_remaining -= frame.payload.body_fragment.len;
    }

    return true;
}

std::vector<std::string> split(const std::string& str, const std::string& delim)
{
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) pos = str.length();
        std::string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
    return tokens;
}