#pragma once

#pragma pack(push)
#pragma pack(1)
struct packet_header {
    unsigned short  _length;
    unsigned char   _command;
};
#pragma pack(pop)

namespace packet_commands {
    enum type {
        // cl -> sv
        client_name,

        // sv -> cl
        client_set,
        client_remove,

        // sv -> cl
        // cl -> sv
        string_query,
        string_result,

        combat_event,
    };
}