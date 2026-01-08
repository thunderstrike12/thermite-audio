#pragma once
#include <unordered_set>
#include <vector>

namespace tmt {

/* Small Types only since we duplicate the data */
template <typename T>
class InsertionOrderedSet {
    std::vector<T> items;
    std::unordered_set<T> lookup;

   public:
    bool insert(const T& value) {
        auto [it, inserted] = lookup.insert(value);
        if (inserted) {
            items.push_back(value);
        }
        return inserted;
    }

    auto begin() { return items.begin(); }
    auto end() { return items.end(); }
    auto begin() const { return items.begin(); }
    auto end() const { return items.end(); }

    size_t size() const { return items.size(); }
    bool contains(const T& value) const { return lookup.count(value) > 0; }

    T& operator[](size_t index) { return items[index]; }
    const T& operator[](size_t index) const { return items[index]; }

    T& at(size_t index) { return items.at(index); }
    const T& at(size_t index) const { return items.at(index); }

    T& front() { return items.front(); }
    const T& front() const { return items.front(); }

    T& back() { return items.back(); }
    const T& back() const { return items.back(); }

    bool empty() const { return items.empty(); }

    operator const std::vector<T>&() const { return items; }

    bool erase(const T& value) {
        auto it = lookup.find(value);
        if (it != lookup.end()) {
            lookup.erase(it);
            items.erase(std::remove(items.begin(), items.end(), value), items.end());
            return true;
        }
        return false;
    }

    void clear() {
        items.clear();
        lookup.clear();
    }
};
}  // namespace tmt