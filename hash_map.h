#include <memory>
#include <vector>
#include <stdexcept>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType> >
class HashMap {
private:
    void expand() {
        if (capacity_ == 0) {
            capacity_ = 1;
            entry_.resize(capacity_);
            origin_.resize(capacity_);
            used.resize(capacity_);
        } else {
            std::vector<std::pair<KeyType, ValueType>> old_entry = entry_;
            std::vector<bool> old_used = used;

            entry_.clear();
            origin_.clear();
            used.clear();
            size_ = 0;

            capacity_ *= 2;
            entry_.resize(capacity_);
            origin_.resize(capacity_);
            used.resize(capacity_);
            for (size_t i = 0; i < capacity_ / 2; ++i) {
                if (old_used[i]) {
                    insert(old_entry[i]);
                }
            }
        }
    }

    void put(size_t pos, std::pair<KeyType, ValueType> entry) {
        entry_[pos] = entry;
        origin_[pos] = pos;
        used[pos] = true;
        ++size_;
    }

    void move(size_t prev_pos, size_t new_pos) {
        entry_[new_pos] = entry_[prev_pos];
        origin_[new_pos] = origin_[prev_pos];
        used[new_pos] = true;

        used[prev_pos] = false;
    }

    size_t dist(size_t f_pos, size_t s_pos) const {
        if (s_pos >= f_pos) {
            return s_pos - f_pos;
        } else {
            return s_pos - f_pos + capacity_;
        }
    }

    size_t get_pos(KeyType key) const {
        return hasher_(key) % capacity_;
    }

    void remove(size_t pos) {
        used[pos] = false;
        --size_;
    }

public:
    class iterator {
    private:
        void next() {
            while (pos < map_->capacity_ && !map_->used[pos]) {
                ++pos;
            }
        }

        void prev() {
            while (!map_->used[pos]) {
                --pos;
            }
        }

        HashMap *map_;

    public:
        size_t pos;

        explicit iterator() = default;

        explicit iterator(HashMap *map, size_t pos) : map_(map), pos(pos) {}

        std::pair<KeyType, ValueType> operator*() {
            next();
            return map_->entry_[pos];
        }

        std::pair<const KeyType, ValueType> *operator->() {
            next();
            return reinterpret_cast<std::pair<const KeyType, ValueType> *>(&map_->entry_[pos]);
        }

        iterator &operator=(const iterator &other) {
            pos = other.pos;
            return *this;
        }

        iterator &operator++() {
            next();
            pos = std::min(pos + 1, map_->capacity_);
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            next();
            pos = std::min(pos + 1, map_->capacity_);
            return tmp;
        }

        iterator &operator--() {
            --pos;
            prev();
            return *this;
        }

        iterator operator--(int) {
            iterator tmp = *this;
            --pos;
            prev();
            return tmp;
        }

        bool operator==(iterator other) {
            next();
            other.next();
            return map_ == other.map_ && pos == other.pos;
        }

        bool operator!=(iterator other) {
            next();
            other.next();
            return map_ != other.map_ || pos != other.pos;
        }
    };

    class const_iterator {
    private:
        void next() {
            while (pos < map_->capacity_ && !map_->used[pos]) {
                ++pos;
            }
        }

        void prev() {
            while (!map_->used[pos]) {
                --pos;
            }
        }

        const HashMap *map_;

    public:
        size_t pos;

        explicit const_iterator() = default;

        explicit const_iterator(const HashMap *map, size_t pos) : map_(map), pos(pos) {}

        std::pair<KeyType, ValueType> operator*() {
            next();
            return map_->entry_[pos];
        }

        const std::pair<KeyType, ValueType> *operator->() {
            next();
            return &map_->entry_[pos];
        }

        const_iterator &operator=(const const_iterator &other) {
            pos = other.pos;
            return *this;
        }

        const_iterator &operator++() {
            next();
            ++pos;
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            next();
            ++pos;
            return tmp;
        }

        const_iterator &operator--() {
            --pos;
            prev();
            return *this;
        }

        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --pos;
            prev();
            return tmp;
        }

        bool operator==(const_iterator other) {
            next();
            other.next();
            return map_ == other.map_ && pos == other.pos;
        }

        bool operator!=(const_iterator other) {
            next();
            other.next();
            return map_ != other.map_ || pos != other.pos;
        }
    };

    explicit HashMap(Hash hasher = Hash()) : hasher_(hasher) {
    }

    template<class Iterator>
    HashMap(Iterator begin, Iterator end, Hash hasher = Hash()): hasher_(hasher) {
        for (auto it = begin; it != end; ++it) {
            insert(*it);
        }
    }

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list, Hash hasher = Hash()) : hasher_(hasher) {
        for (const auto &entry: list) {
            insert(entry);
        }
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    Hash hash_function() const {
        return hasher_;
    }

    iterator insert_without_check(std::pair<KeyType, ValueType> elem) {
        if (capacity_ == 0 || 1. * (size_ + 1) / capacity_ > LOAD_FACTOR) {
            expand();
        }
        size_t pos = get_pos(elem.first);
        if (!used[pos]) {
            put(pos, elem);
            return iterator(this, pos);
        } else {
            for (size_t shift = 1; shift < capacity_; ++shift) {
                size_t n_pos = pos + shift;
                if (n_pos >= capacity_) {
                    n_pos -= capacity_;
                }
                if (!used[n_pos]) {
                    size_t cur_pos = n_pos;
                    while (dist(pos, cur_pos) >= H) {
                        bool moved = false;
                        for (size_t l_shift = 1; l_shift < H; ++l_shift) {
                            size_t prev;
                            if (cur_pos < l_shift) {
                                prev = capacity_ - l_shift + cur_pos;
                            } else {
                                prev = cur_pos - l_shift;
                            }
                            if (dist(origin_[prev], cur_pos) < H) {
                                move(prev, cur_pos);
                                cur_pos = prev;
                                moved = true;
                                break;
                            }
                        }
                        if (!moved) {
                            H *= 2;
                            return insert_without_check(elem);
                        }
                    }
                    if (dist(origin_[pos], cur_pos) < H) {
                        move(pos, cur_pos);
                        put(pos, elem);
                        return iterator(this, pos);
                    } else {
                        H *= 2;
                        return insert_without_check(elem);
                    }
                }
            }
            expand();
            return insert_without_check(elem);
        }
    }

    iterator insert(std::pair<KeyType, ValueType> elem) {
        iterator it = find(elem.first);
        if (find(elem.first) != end()) {
            return it;
        }
        return insert_without_check(elem);
    }

    bool erase(KeyType key) {
        iterator it = find(key);
        if (it == end()) {
            return false;
        } else {
            remove(it.pos);
            return true;
        }
    }

    iterator begin() {
        return iterator(this, 0);
    }

    const_iterator begin() const {
        return const_iterator(this, 0);
    }

    iterator end() {
        return iterator(this, capacity_);
    }

    const_iterator end() const {
        return const_iterator(this, capacity_);
    }

    iterator find(KeyType key) {
        if (capacity_ == 0) return end();
        size_t pos = get_pos(key);
        for (size_t shift = 0; shift < H; ++shift) {
            size_t n_pos = pos + shift;
            if (pos + shift >= capacity_) {
                n_pos -= capacity_;
            }
            if (used[n_pos] && key == entry_[n_pos].first) {
                return iterator(this, n_pos);
            }
        }
        return end();
    }

    const_iterator find(KeyType key) const {
        if (capacity_ == 0) return end();
        size_t pos = get_pos(key);
        for (size_t shift = 0; shift < H; ++shift) {
            size_t n_pos = pos + shift;
            if (pos + shift >= capacity_) {
                n_pos -= capacity_;
            }
            if (used[n_pos] && key == entry_[n_pos].first) {
                return const_iterator(this, n_pos);
            }
        }
        return end();
    }

    ValueType &operator[](KeyType key) {
        iterator it = find(key);
        if (it == end()) {
            it = insert({key, ValueType()});
        }
        return entry_[it.pos].second;
    }

    const ValueType &at(KeyType key) const {
        const_iterator it = find(key);
        if (it == end()) {
            throw std::out_of_range("Item not found");
        } else {
            return entry_[it.pos].second;
        }
    }

    void clear() {
        entry_.clear();
        origin_.clear();
        used.clear();
        size_ = 0;
        capacity_ = 0;
        H = 1;
    }

private:
    std::vector<std::pair<KeyType, ValueType>> entry_;
    std::vector<size_t> origin_;
    std::vector<bool> used;

    size_t size_ = 0;
    size_t capacity_ = 0;
    uint32_t H = 1;

    constexpr static double LOAD_FACTOR = 0.5;

    Hash hasher_;
};

