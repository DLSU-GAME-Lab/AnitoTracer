#pragma once

// Generic growable array exposed to scripts as `array<T>`.

#include <algorithm>
#include <cstddef>
#include <vector>

namespace anito {

template <class T>
class Array {
public:
    std::vector<T> items;

    int count() const { return static_cast<int>(items.size()); }
    void add(const T& value) { items.push_back(value); }
    void insert(int index, const T& value) {
        if (index < 0 || index > count()) return;
        items.insert(items.begin() + index, value);
    }
    void removeAt(int index) {
        if (InRange(index)) items.erase(items.begin() + index);
    }
    void clear() { items.clear(); }
    bool contains(const T& value) const { return indexOf(value) >= 0; }
    int indexOf(const T& value) const {
        const auto it = std::find(items.begin(), items.end(), value);
        return it == items.end() ? -1 : static_cast<int>(it - items.begin());
    }
    T get(int index) const { return InRange(index) ? items[static_cast<size_t>(index)] : T{}; }

    // Drops elements whose handle is no longer valid (handle types only).
    void prune() {
        items.erase(std::remove_if(items.begin(), items.end(), [](const T& v) { return !v.valid(); }), items.end());
    }

    // Out-of-range access yields a scratch default value instead of undefined behaviour.
    T& operator[](int index) {
        if (InRange(index)) return items[static_cast<size_t>(index)];
        static thread_local T scratch;
        scratch = T{};
        return scratch;
    }

private:
    bool InRange(int index) const { return index >= 0 && index < count(); }
};

} // namespace anito
