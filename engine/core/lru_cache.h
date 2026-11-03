// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <list>
#include <stdexcept>
#include <unordered_map>
#include <utility>
namespace trackrs {
// Stable node addresses until eviction. All lookup/touch/eviction operations
// O(1) on average; ownership and eviction are confined to the caller's thread.
template <class Key, class Value, class Hash = std::hash<Key>> class LruCache {
  using Nodes = std::list<std::pair<Key, Value>>;
  Nodes nodes_;
  std::unordered_map<Key, typename Nodes::iterator, Hash> index_;
  std::size_t capacity_;

public:
  explicit LruCache(std::size_t capacity) : capacity_(capacity) {
    if (!capacity)
      throw std::invalid_argument("empty LRU capacity");
  }
  LruCache(const LruCache &) = delete;
  LruCache &operator=(const LruCache &) = delete;
  const Value *find(const Key &key) const {
    auto item = index_.find(key);
    return item == index_.end() ? nullptr : &item->second->second;
  }
  template <class Factory> Value &get(const Key &key, Factory factory) {
    auto item = index_.find(key);
    if (item != index_.end()) {
      nodes_.splice(nodes_.begin(), nodes_, item->second);
      return item->second->second;
    }
    nodes_.emplace_front(key, factory());
    try {
      index_.emplace(key, nodes_.begin());
    } catch (...) {
      nodes_.pop_front();
      throw;
    }
    if (nodes_.size() > capacity_) {
      index_.erase(nodes_.back().first);
      nodes_.pop_back();
    }
    return nodes_.front().second;
  }
  void clear() {
    index_.clear();
    nodes_.clear();
  }
  std::size_t size() const { return nodes_.size(); }
};
struct TileKey {
  int x = 0, y = 0, level = 0;
  bool operator==(const TileKey &other) const {
    return x == other.x && y == other.y && level == other.level;
  }
};
struct TileHash {
  std::size_t operator()(const TileKey &key) const noexcept {
    auto hash = std::hash<int>{}(key.x);
    hash ^= std::hash<int>{}(key.y) + std::size_t{0x9e3779b9u} + (hash << 6u) +
            (hash >> 2u);
    hash ^= std::hash<int>{}(key.level) + std::size_t{0x9e3779b9u} +
            (hash << 6u) + (hash >> 2u);
    return hash;
  }
};
} // namespace trackrs
