#ifndef CAST_H__
#define CAST_H__

#include <cstddef>

template<typename T, typename U> T ptr_cast(U* p) {
    return reinterpret_cast<T>(reinterpret_cast<size_t>(p));
}

#endif
