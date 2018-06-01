#include "buffer.h"

namespace beam { namespace io {

SharedBuffer normalize(const SerializedMsg& msg) {
    size_t n = msg.size();
    if (n==0) return SharedBuffer(0, 0);
    // copies
    if (n==1) return SharedBuffer(msg[0].data, msg[0].size);

    size_t size = 0;
    for (const auto& fr : msg) {
        size += fr.size;
    }
    uint8_t* data = (uint8_t*)malloc(size);
    uint8_t* ptr = data;

    // TODO handle data==0

    for (const auto& fr : msg) {
        memcpy(ptr, fr.data, fr.size);
        ptr += fr.size;
    }
    return SharedBuffer(data, size, SharedMem(data, [](void* p) { free(p); }));
}

}} //namespaces