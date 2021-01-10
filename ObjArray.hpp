#ifndef UD1_VIEWER_IMGUI_OBJARRAY_HPP
#define UD1_VIEWER_IMGUI_OBJARRAY_HPP

#include <cstddef>
#include <array>
#include <memory>
#include <vector>
#include <cassert>

const size_t BUF_ARR_SIZE = 1024 * 10;
struct ObjArray {
    ObjArray() {
        bytes = std::make_unique<std::array<unsigned char, BUF_ARR_SIZE>>();
        cur_pos = bytes->data();
        remaining = BUF_ARR_SIZE;
    }

    void * alloc(size_t s, size_t a) {
        return std::align(a, s, cur_pos, remaining);
    }

    std::unique_ptr<std::array<unsigned char, BUF_ARR_SIZE>> bytes;
    void *cur_pos;
    size_t remaining;
};

enum class ObjType {
    STRING,
    DOUBLE,
    UINT,
    P2,
    V3,
    M4
};

struct ObjEntry {
    ObjType type;
    void *ptr;
};

struct Storage {
    Storage() {
        data.emplace_back();
    }

    void push(std::string &&s) {
        _push<>(std::move(s), ObjType::STRING);
    }

    void push(double val) {
        _push<>(std::move(val), ObjType::DOUBLE);
    }

    void push(uint32_t val) {
        _push<>(std::move(val), ObjType::UINT);
    }

private:
    template<typename T>
    void _push(T &&t, ObjType type) {
        void *ptr = alloc(sizeof(T), alignof(T));
        new (ptr) T{std::forward<T>(t)};
        objects.push_back({.type = type, .ptr = ptr});
    }

    void * alloc(size_t s, size_t a) {
        assert(s < BUF_ARR_SIZE);
        ObjArray &ar = *data.rbegin();
        void * res = ar.alloc(s, a);
        if (res)
            return res;

        data.emplace_back();
        return data.rbegin()->alloc(s, a);
    }
    std::vector<ObjArray> data;
    std::vector<ObjEntry> objects;
};


#endif //UD1_VIEWER_IMGUI_OBJARRAY_HPP
