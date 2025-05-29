#ifndef CPP_UNORDERED_MAP_H
#define CPP_UNORDERED_MAP_H

#include <iostream>
#include <vector>
#include <cmath>

template<typename Key, typename Value, typename Hash=std::hash<Key>, typename Equal=std::equal_to<Key>,
        typename Allocator=std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
public:
  using NodeType = std::pair<Key, Value>;
  using ValueType = std::pair<const Key, Value>;

private:
  class List;

  List list;
  std::vector<typename List::Node*> buckets;

  Allocator alloc;

  double current_load_factor;
  double mx_load_factor;

  template<bool is_const>
  class common_iterator;

  void update_load_factor();

public:
  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;

  UnorderedMap();

  UnorderedMap(const UnorderedMap& other);

  UnorderedMap(UnorderedMap&& other) noexcept;

  UnorderedMap& operator=(const UnorderedMap& other);

  UnorderedMap& operator=(UnorderedMap&& other) noexcept;

  ~UnorderedMap() = default;

  Value& operator[](const Key& key);

  Value& operator[](Key&& key);

  Value& at(const Key& key);

  const Value& at(const Key& key) const;

  size_t size() const;

  size_t bucket_count() const;

  iterator begin() { return iterator(&*list.begin()); }

  iterator end() { return iterator(&*list.end()); }

  const_iterator begin() const { return const_iterator(&*list.cbegin()); }

  const_iterator end() const { return const_iterator(&*list.cend()); }

  const_iterator cbegin() const { return const_iterator(&*list.cbegin()); }

  const_iterator cend() const { return const_iterator(&*list.cend()); }

  std::pair<iterator, bool> insert(const NodeType& object);

  std::pair<iterator, bool> insert(NodeType&& object);

  template<typename InputIterator>
  void insert(InputIterator first, InputIterator last);

  template<typename ...Args>
  std::pair<iterator, bool> emplace(Args&& ... args);

  std::pair<iterator, bool> emplace(const ValueType& pair);

  std::pair<iterator, bool> emplace(const Key& key, const Value& value);

  std::pair<iterator, bool> emplace(const Key& key, Value&& value);

  void erase(iterator iter);

  void erase(iterator first, iterator last);

  iterator find(const Key& key);

  const_iterator find(const Key& key) const;

  void reserve(size_t sz);

  double load_factor() const;

  double max_load_factor() const;

  void set_max_load_factor(double factor);

  void rehash(size_t sz);

  auto get_allocator() const;
};

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
auto UnorderedMap<Key, Value, Hash, Equal, Allocator>::get_allocator() const {
  return list.get_allocator();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::UnorderedMap():buckets(
        std::vector<typename List::Node*>(16)), current_load_factor(0), mx_load_factor(0.8) {}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::UnorderedMap(const UnorderedMap& other) :
        list(other.list), buckets(std::vector<typename List::Node*>(other.buckets.size())),
        current_load_factor(other.current_load_factor), mx_load_factor(other.mx_load_factor) {
  for (auto iter = list.begin(); iter != list.end(); ++iter) {
    if (buckets[iter->hash % buckets.size()] == nullptr) {
      buckets[iter->hash % buckets.size()] = &*iter;
    }
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::UnorderedMap(UnorderedMap&& other) noexcept :
        list(std::move(other.list), other.alloc), buckets(std::move(other.buckets)),
        current_load_factor(other.current_load_factor), mx_load_factor(other.mx_load_factor) {
  other.current_load_factor = 0;
  other.mx_load_factor = 0;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>&
UnorderedMap<Key, Value, Hash, Equal, Allocator>::operator=(const UnorderedMap& other) {
  if (&other == this) {
    return *this;
  }
  list = other.list;
  current_load_factor = other.current_load_factor;
  mx_load_factor = other.mx_load_factor;

  buckets.assign(other.buckets.size(), nullptr);
  for (auto iter = list.begin(); iter != list.end(); ++iter) {
    if (buckets[iter->hash % buckets.size()] == nullptr) {
      buckets[iter->hash % buckets.size()] = &*iter;
    }
  }
  return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>&
UnorderedMap<Key, Value, Hash, Equal, Allocator>::operator=(UnorderedMap&& other) noexcept {
  list = std::move(other.list);
  current_load_factor = other.current_load_factor;
  mx_load_factor = other.mx_load_factor;

  buckets.assign(other.buckets.size(), nullptr);
  for (auto iter = list.begin(); iter != list.end(); ++iter) {
    if (buckets[iter->hash % buckets.size()] == nullptr) {
      buckets[iter->hash % buckets.size()] = &*iter;
    }
  }

  other.buckets.clear();
  other.current_load_factor = 0;
  other.mx_load_factor = 0;
  return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
Value& UnorderedMap<Key, Value, Hash, Equal, Allocator>::operator[](const Key& key) {
  iterator iter = find(key);
  if (iter == end()) {
    size_t hash = Hash{}(key) % buckets.size();
    if (buckets[hash] == nullptr) {
      list.emplace(list.begin(), key, Value());
      buckets[hash] = &*list.begin();
    } else {
      list.emplace(typename List::iterator(buckets[hash]), key, Value());
      buckets[hash] = static_cast<List::Node*>(buckets[hash]->prev);
    }
    update_load_factor();
    iter = iterator(buckets[hash]);
  }
  return iter->second;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
Value& UnorderedMap<Key, Value, Hash, Equal, Allocator>::operator[](Key&& key) {
  iterator iter = find(key);
  if (iter == end()) {
    size_t hash = Hash{}(key) % buckets.size();
    if (buckets[hash] == nullptr) {
      list.emplace(list.begin(), std::move(key), Value());
      buckets[hash] = &*list.begin();
    } else {
      list.emplace(typename List::iterator(buckets[hash]), std::move(key), Value());
      buckets[hash] = static_cast<List::Node*>(buckets[hash]->prev);
    }
    update_load_factor();
    iter = iterator(buckets[hash]);
  }
  return iter->second;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
const Value& UnorderedMap<Key, Value, Hash, Equal, Allocator>::at(const Key& key) const {
  const_iterator iter = find(key);
  if (iter == end()) {
    throw std::out_of_range("");
  }
  return iter->second;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
Value& UnorderedMap<Key, Value, Hash, Equal, Allocator>::at(const Key& key) {
  iterator iter = find(key);
  if (iter == end()) {
    throw std::out_of_range("");
  }
  return iter->second;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
size_t UnorderedMap<Key, Value, Hash, Equal, Allocator>::size() const {
  return list.size();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
size_t UnorderedMap<Key, Value, Hash, Equal, Allocator>::bucket_count() const {
  return buckets.size();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Allocator>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::insert(const std::pair<Key, Value>& object) {
  auto iter = find(object.first);
  if (iter != end()) {
    return {iter, false};
  }
  size_t hash = Hash{}(object.first) % buckets.size();
  if (buckets[hash] == nullptr) {
    list.emplace(list.begin(), object);
    buckets[hash] = &*list.begin();
  } else {
    list.emplace(List::iterator(buckets[hash]), object);
    buckets[hash] = static_cast<List::Node*>(buckets[hash]->prev);
  }
  update_load_factor();
  return {iterator(buckets[hash]), true};
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Allocator>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::insert(std::pair<Key, Value>&& object) {
  auto iter = find(object.first);
  if (iter != end()) {
    return {iter, false};
  }
  size_t hash = Hash{}(object.first) % buckets.size();
  if (buckets[hash] == nullptr) {
    list.emplace(list.begin(), std::move(object));
    buckets[hash] = &*list.begin();
  } else {
    list.emplace(typename List::iterator(buckets[hash]), std::move(object));
    buckets[hash] = static_cast<List::Node*>(buckets[hash]->prev);
  }
  update_load_factor();
  return {iterator(buckets[hash]), true};
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
template<typename InputIterator>
void
UnorderedMap<Key, Value, Hash, Equal, Allocator>::insert(InputIterator first, InputIterator last) {
  for (; first != last; ++first) {
    insert(*first);
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
template<typename ...Args>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Allocator>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::emplace(Args&& ... args) {
  list.emplace(list.begin(), std::forward<Args>(args)...);
  auto iter = find(list.begin()->pair.first);
  if (iter != end()) {
    list.pop_front();
    return {iter, false};
  }
  size_t hash = list.begin()->hash % buckets.size();
  if (buckets[hash] == nullptr) {
    buckets[hash] = &*list.begin();
  } else {
    list.insert(typename List::iterator(buckets[hash]), list.extract(list.begin()));
    buckets[hash] = static_cast<List::Node*>(buckets[hash]->prev);
  }
  update_load_factor();
  return {iterator(buckets[hash]), true};
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Allocator>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::emplace(const ValueType& pair) {
  auto iter = find(list.begin()->pair.first);
  if (iter != end()) {
    return {iter, false};
  }
  size_t hash = list.begin()->hash % buckets.size();
  if (buckets[hash] == nullptr) {
    list.emplace(list.begin(), pair);
    buckets[hash] = &*list.begin();
  } else {
    list.emplace(typename List::iterator(buckets[hash]), pair);
    buckets[hash] = static_cast<List::Node*>(buckets[hash]->prev);
  }
  update_load_factor();
  return {iterator(buckets[hash]), true};
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Allocator>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::emplace(const Key& key, const Value& value) {
  auto iter = find(key);
  if (iter != end()) {
    return {iter, false};
  }
  size_t hash = list.begin()->hash % buckets.size();
  if (buckets[hash] == nullptr) {
    list.emplace(list.begin(), key, value);
    buckets[hash] = &*list.begin();
  } else {
    list.emplace(typename List::iterator(buckets[hash]), key, value);
    buckets[hash] = static_cast<List::Node*>(buckets[hash]->prev);
  }
  update_load_factor();
  return {iterator(buckets[hash]), true};
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Allocator>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::emplace(const Key& key, Value&& value) {
  auto iter = find(key);
  if (iter != end()) {
    return {iter, false};
  }
  size_t hash = list.begin()->hash % buckets.size();
  if (buckets[hash] == nullptr) {
    list.emplace(list.begin(), key, std::move(value));
    buckets[hash] = &*list.begin();
  } else {
    list.emplace(typename List::iterator(buckets[hash]), key, std::move(value));
    buckets[hash] = static_cast<List::Node*>(buckets[hash]->prev);
  }
  update_load_factor();
  return {iterator(buckets[hash]), true};
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::erase(UnorderedMap::iterator iter) {
  size_t hash = Hash{}(iter->first) % buckets.size();
  typename List::Node* node = buckets[hash];
  if (iterator(node) == iter) {
    buckets[hash] = &*++typename List::iterator(buckets[hash]);
    if (buckets[hash]->hash % buckets.size() != node->hash % buckets.size()) {
      buckets[hash] = nullptr;
    }
  }
  list.erase(iter.list_iter);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::erase(UnorderedMap::iterator first,
                                                             UnorderedMap::iterator last) {
  auto prev_iter = first;
  while (first != last) {
    ++first;
    erase(prev_iter);
    prev_iter = first;
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Allocator>::find(const Key& key) const {
  size_t hash = Hash{}(key) % buckets.size();
  if (buckets[hash] == nullptr) {
    return end();
  }
  auto iter = iterator(buckets[hash]);
  while (!Equal{}(iter->first, key)) {
    ++iter;
    if (iter == end()) {
      break;
    }
  }
  return iter;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::iterator
UnorderedMap<Key, Value, Hash, Equal, Allocator>::find(const Key& key) {
  size_t hash = Hash{}(key) % buckets.size();
  if (buckets[hash] == nullptr) {
    return end();
  }
  auto iter = iterator(buckets[hash]);
  while (!Equal{}(iter->first, key)) {
    ++iter;
    if (iter == end()) {
      break;
    }
  }
  return iter;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::reserve(size_t sz) {
  rehash(std::ceil(sz / max_load_factor()));
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
double UnorderedMap<Key, Value, Hash, Equal, Allocator>::max_load_factor() const {
  return mx_load_factor;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
double UnorderedMap<Key, Value, Hash, Equal, Allocator>::load_factor() const {
  return current_load_factor;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::set_max_load_factor(double factor) {
  mx_load_factor = factor;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::rehash(size_t sz) {
  sz = std::max(sz, static_cast<size_t>(std::ceil(size() / max_load_factor())));
  List tmp_list(std::move(list));
  buckets.clear();
  buckets.resize(sz);
  typename List::BaseNode* node = tmp_list.fake_node.next;
  while (node != &tmp_list.fake_node) {
    size_t hash = Hash{}(static_cast<typename List::Node*>(node)->pair.first) % buckets.size();
    node = node->next;
    if (buckets[hash] == nullptr) {
      list.insert(list.begin(), tmp_list.extract(typename List::iterator(node->prev)));
      buckets[hash] = &*list.begin();
    } else {
      list.insert(typename List::iterator(buckets[hash]),
                  tmp_list.extract(typename List::iterator(node->prev)));
    }
  }
  update_load_factor();
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::update_load_factor() {
  current_load_factor = list.size() / buckets.size();
  if (current_load_factor >= mx_load_factor) {
    rehash(buckets.size() * 2 + 1);
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
template<bool is_const>
class UnorderedMap<Key, Value, Hash, Equal, Allocator>::common_iterator {
  friend UnorderedMap;

  using Type = std::conditional_t<is_const, const ValueType, ValueType>;
  using ListBaseNode = std::conditional_t<is_const, const typename List::BaseNode, typename List::BaseNode>;
  using ListIterator = std::conditional_t<is_const, typename List::const_iterator, typename List::iterator>;

  ListIterator list_iter;

  explicit common_iterator(ListBaseNode* node) : list_iter(node) {}

  common_iterator<false> iter_const_cast() {
    return common_iterator<false>(const_cast<ListIterator>(list_iter));
  }

public:

  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = int;
  using value_type = std::remove_cv_t<Type>;
  using pointer = Type*;
  using reference = Type&;

  template<bool other_const>
  requires(is_const || !other_const)
  common_iterator(const common_iterator<other_const>& other) : list_iter(other.list_iter) {}

  template<bool other_const>
  requires(is_const || !other_const)
  common_iterator& operator=(const common_iterator<other_const>& other) {
    list_iter = other.list_iter;
    return *this;
  }

  Type& operator*() const {
    return reinterpret_cast<Type&>(list_iter->pair);
  }

  Type* operator->() const {
    return reinterpret_cast<Type*>(&list_iter->pair);
  }

  common_iterator& operator++() {
    ++list_iter;
    return *this;
  }

  common_iterator& operator--() {
    --list_iter;
    return *this;
  }

  common_iterator operator++(int) {
    common_iterator tmp = *this;
    ++list_iter;
    return tmp;
  }

  common_iterator operator--(int) {
    common_iterator tmp = *this;
    --list_iter;
    return tmp;
  }

  bool operator==(const common_iterator& other) const {
    return list_iter == other.list_iter;
  }
};

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
class UnorderedMap<Key, Value, Hash, Equal, Allocator>::List {
public:
  struct BaseNode {
    BaseNode* next;
    BaseNode* prev;
  };

  struct Node : BaseNode {
    NodeType pair;
    size_t hash;

    void calculate_hash() {
      hash = Hash{}(pair.first);
    }
  };

  using NodeAlloc = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using NodeAllocatorTraits = typename std::allocator_traits<NodeAlloc>;

  BaseNode fake_node;
  size_t sz;

  [[no_unique_address]] Allocator allocator;
  [[no_unique_address]] NodeAlloc node_allocator;

  explicit List(const Allocator& alloc = Allocator());

  explicit List(size_t count, const ValueType& value, const Allocator& alloc = Allocator());

  explicit List(size_t count, ValueType&& value, const Allocator& alloc = Allocator());

  explicit List(size_t count, const Allocator& alloc = Allocator());

  List(const List& other);

  List(List&& other) noexcept;

  List(List&& other, const Allocator& alloc) noexcept;

  List& operator=(const List& other);

  List& operator=(List&& other) noexcept;

  ~List();

  Allocator get_allocator() const { return allocator; }

  [[nodiscard]] size_t size() const { return sz; }

  void push_back(const ValueType& value);

  void push_front(const ValueType& value);

  void pop_back();

  void pop_front();

  template<bool is_const>
  class common_iterator;

  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() { return iterator(fake_node.next); }

  iterator end() { return iterator(&fake_node); }

  const_iterator begin() const { return const_iterator(fake_node.next); }

  const_iterator end() const { return const_iterator(&fake_node); }

  const_iterator cbegin() const { return const_iterator(fake_node.next); }

  const_iterator cend() const { return const_iterator(&fake_node); }

  reverse_iterator rbegin() { return std::reverse_iterator(end()); }

  reverse_iterator rend() { return std::reverse_iterator(begin()); }

  const_reverse_iterator rbegin() const { return std::reverse_iterator(cend()); }

  const_reverse_iterator rend() const { return std::reverse_iterator(cbegin()); }

  const_reverse_iterator crbegin() const { return std::reverse_iterator(cend()); }

  const_reverse_iterator crend() const { return std::reverse_iterator(cbegin()); }

  void insert(iterator pos, const ValueType& value);

  void insert(const_iterator pos, const ValueType& value);

  List::Node* extract(iterator pos);

  void insert(iterator pos, List::Node* node);

  template<typename ...Args>
  void emplace(iterator pos, Args&& ... args);

  template<typename ...Args>
  void emplace(const_iterator pos, Args&& ... args);

  void erase(iterator pos);

  void erase(const_iterator pos);
};

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::List(const Allocator& alloc) :
        fake_node(BaseNode{&fake_node, &fake_node}), sz(0), allocator(alloc),
        node_allocator(alloc) {}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::List(size_t count, const ValueType& value,
                                                             const Allocator& alloc) :
        fake_node(BaseNode{&fake_node, &fake_node}), sz(count), allocator(alloc),
        node_allocator(alloc) {
  BaseNode* prev_node = &fake_node;
  try {
    for (size_t i = 0; i < count; ++i) {
      prev_node->next = NodeAllocatorTraits::allocate(node_allocator, 1);
      NodeAllocatorTraits::template construct<NodeType>(node_allocator,
                                                        &static_cast<Node*>(prev_node->next)->pair,
                                                        value);
      static_cast<Node*>(prev_node->next)->calculate_hash();

      prev_node->next->prev = prev_node;
      prev_node = prev_node->next;
    }

    prev_node->next = &fake_node;
    fake_node.prev = prev_node;
  } catch (...) {
    while (prev_node != &fake_node) {
      prev_node = prev_node->prev;
      NodeAllocatorTraits::template destroy<NodeType>(node_allocator,
                                                      &static_cast<Node*>(prev_node->next)->pair);
      NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(prev_node->next), 1);
    }
    throw;
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::List(size_t count, ValueType&& value,
                                                             const Allocator& alloc) :
        fake_node(BaseNode{&fake_node, &fake_node}), sz(count), allocator(alloc),
        node_allocator(alloc) {
  BaseNode* prev_node = &fake_node;
  try {
    for (size_t i = 0; i < count; ++i) {
      prev_node->next = NodeAllocatorTraits::allocate(node_allocator, 1);
      NodeAllocatorTraits::template construct<NodeType>(node_allocator,
                                                        &static_cast<Node*>(prev_node->next)->pair,
                                                        std::move(value));
      static_cast<Node*>(prev_node->next)->calculate_hash();

      prev_node->next->prev = prev_node;
      prev_node = prev_node->next;
    }

    prev_node->next = &fake_node;
    fake_node.prev = prev_node;
  } catch (...) {
    NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(prev_node->next), 1);
    while (prev_node != &fake_node) {
      prev_node = prev_node->prev;
      NodeAllocatorTraits::template destroy<NodeType>(node_allocator,
                                                      &static_cast<Node*>(prev_node->next)->pair);
      NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(prev_node->next), 1);
    }
    throw;
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::List(size_t count, const Allocator& alloc)
        : fake_node(BaseNode{&fake_node, &fake_node}), sz(count), allocator(alloc),
          node_allocator(alloc) {
  BaseNode* prev_node = &fake_node;
  try {
    for (size_t i = 0; i < count; ++i) {
      prev_node->next = NodeAllocatorTraits::allocate(node_allocator, 1);
      NodeAllocatorTraits::template construct<NodeType>(node_allocator,
                                                        &static_cast<Node*>(prev_node->next)->pair);
      static_cast<Node*>(prev_node->next)->calculate_hash();

      prev_node->next->prev = prev_node;
      prev_node = prev_node->next;
    }

    prev_node->next = &fake_node;
    fake_node.prev = prev_node;
  } catch (...) {
    NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(prev_node->next), 1);
    while (prev_node != &fake_node) {
      prev_node = prev_node->prev;
      NodeAllocatorTraits::template destroy<NodeType>(node_allocator,
                                                      &static_cast<Node*>(prev_node->next)->pair);
      NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(prev_node->next), 1);
    }
    throw;
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::List(const List& other) : fake_node(
        BaseNode{&fake_node, &fake_node}), sz(other.sz), allocator(
        std::allocator_traits<Allocator>::select_on_container_copy_construction(
                other.get_allocator())), node_allocator(allocator) {
  BaseNode* prev_node = &fake_node;
  try {
    BaseNode* node = other.fake_node.next;
    for (size_t i = 0; i < other.sz; ++i) {
      prev_node->next = NodeAllocatorTraits::allocate(node_allocator, 1);
      NodeAllocatorTraits::template construct<NodeType>(node_allocator,
                                                        &static_cast<Node*>(prev_node->next)->pair,
                                                        static_cast<Node*>(node)->pair);
      static_cast<Node*>(prev_node->next)->calculate_hash();

      prev_node->next->prev = prev_node;
      prev_node = prev_node->next;
      node = node->next;
    }

    prev_node->next = &fake_node;
    fake_node.prev = prev_node;
  } catch (...) {
    NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(prev_node->next), 1);
    while (prev_node != &fake_node) {
      prev_node = prev_node->prev;
      NodeAllocatorTraits::template destroy<NodeType>(node_allocator,
                                                      &static_cast<Node*>(prev_node->next)->pair);
      NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(prev_node->next), 1);
    }
    throw;
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::List(List&& other) noexcept :
        allocator(std::allocator_traits<Allocator>::select_on_container_copy_construction(
                std::move(other.get_allocator()))), node_allocator(allocator) {
  fake_node = BaseNode{&fake_node, &fake_node};
  sz = other.sz;
  if (sz != 0) {
    fake_node = other.fake_node;
    fake_node.next->prev = &fake_node;
    fake_node.prev->next = &fake_node;
    other.sz = 0;
    other.fake_node = BaseNode{&other.fake_node, &other.fake_node};
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::List(List&& other,
                                                             const Allocator& alloc) noexcept :
        allocator(alloc), node_allocator(alloc) {
  fake_node = BaseNode{&fake_node, &fake_node};
  sz = other.sz;
  if (sz != 0) {
    fake_node = other.fake_node;
    fake_node.next->prev = &fake_node;
    fake_node.prev->next = &fake_node;
    other.sz = 0;
    other.fake_node = BaseNode{&other.fake_node, &other.fake_node};
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List&
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::operator=(
        const UnorderedMap<Key, Value, Hash, Equal, Allocator>::List& other) {
  if (this == &other) {
    return *this;
  }

  BaseNode tmp_fake_node = fake_node;
  Allocator tmp_allocator = allocator;
  NodeAlloc tmp_node_allocator = node_allocator;

  if (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
    allocator = other.get_allocator();
    node_allocator = allocator;
  }

  BaseNode* prev_node = &fake_node;
  try {
    BaseNode* node = other.fake_node.next;
    for (size_t i = 0; i < other.sz; ++i) {
      prev_node->next = NodeAllocatorTraits::allocate(node_allocator, 1);
      NodeAllocatorTraits::template construct<NodeType>(node_allocator,
                                                        &static_cast<Node*>(prev_node->next)->pair,
                                                        static_cast<Node*>(node)->pair);
      static_cast<Node*>(prev_node->next)->calculate_hash();

      prev_node->next->prev = prev_node;
      prev_node = prev_node->next;
      node = node->next;
    }

    prev_node->next = &fake_node;
    fake_node.prev = prev_node;
    sz = other.sz;
  } catch (...) {
    NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(prev_node->next), 1);
    while (prev_node != &fake_node) {
      prev_node = prev_node->prev;
      NodeAllocatorTraits::template destroy<NodeType>(node_allocator,
                                                      &static_cast<Node*>(prev_node->next)->pair);
      NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(prev_node->next), 1);
    }

    fake_node = tmp_fake_node;
    allocator = tmp_allocator;
    node_allocator = tmp_node_allocator;

    throw;
  }

  BaseNode* node = tmp_fake_node.next;
  tmp_fake_node.prev->next = &tmp_fake_node;
  while (node != &tmp_fake_node) {
    NodeAllocatorTraits::template destroy<NodeType>(tmp_node_allocator,
                                                    &static_cast<Node*>(node)->pair);
    node = node->next;
    NodeAllocatorTraits::deallocate(tmp_node_allocator, static_cast<Node*>(node->prev), 1);
  }

  return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List&
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::operator=(
        UnorderedMap<Key, Value, Hash, Equal, Allocator>::List&& other) noexcept {
  BaseNode* node = fake_node.next;
  while (node != &fake_node) {
    NodeAllocatorTraits::template destroy<NodeType>(node_allocator,
                                                    &static_cast<Node*>(node)->pair);
    node = node->next;
    NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(node->prev), 1);
  }
  if (std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
    allocator = other.get_allocator();
    node_allocator = allocator;
    fake_node = BaseNode{&fake_node, &fake_node};
    sz = other.sz;
    if (sz != 0) {
      fake_node = other.fake_node;
      fake_node.next->prev = &fake_node;
      fake_node.prev->next = &fake_node;
      other.sz = 0;
      other.fake_node.next = &other.fake_node;
      other.fake_node.prev = &other.fake_node;
    }
    return *this;
  }

  NodeAlloc other_node_allocator = other.get_allocator();

  BaseNode* prev_node = &fake_node;
  BaseNode* other_node = other.fake_node.next;
  for (size_t i = 0; i < other.sz; ++i) {
    prev_node->next = NodeAllocatorTraits::allocate(node_allocator, 1);
    NodeAllocatorTraits::template construct<NodeType>(node_allocator,
                                                      &static_cast<Node*>(prev_node->next)->pair,
                                                      std::forward<NodeType>(
                                                              static_cast<Node*>(other_node)->pair));
    static_cast<Node*>(prev_node->next)->calculate_hash();

    prev_node->next->prev = prev_node;
    prev_node = prev_node->next;
    other_node = other_node->next;

    NodeAllocatorTraits::template destroy<NodeType>(other_node_allocator,
                                                    &static_cast<Node*>(other_node->prev)->pair);
    NodeAllocatorTraits::deallocate(other_node_allocator, static_cast<Node*>(other_node->prev), 1);
  }

  prev_node->next = &fake_node;
  fake_node.prev = prev_node;
  sz = other.sz;

  other.fake_node.next = &other.fake_node;
  other.fake_node.prev = &other.fake_node;
  other.sz = 0;

  return *this;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::~List() {
  BaseNode* node = fake_node.next;
  while (node != &fake_node) {
    NodeAllocatorTraits::template destroy<NodeType>(node_allocator,
                                                    &static_cast<Node*>(node)->pair);
    node = node->next;
    NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(node->prev), 1);
  }
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::push_back(const ValueType& value) {
  Node* node = NodeAllocatorTraits::allocate(node_allocator, 1);
  NodeAllocatorTraits::template construct<NodeType>(node_allocator, &node->pair, value);
  node->calculate_hash();

  fake_node.prev->next = node;
  node->next = &fake_node;
  node->prev = fake_node.prev;
  fake_node.prev = node;

  ++sz;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::push_front(const ValueType& value) {
  Node* node = NodeAllocatorTraits::allocate(node_allocator, 1);
  NodeAllocatorTraits::template construct<NodeType>(node_allocator, &node->pair, value);
  node->calculate_hash();

  fake_node.next->prev = node;
  node->prev = &fake_node;
  node->next = fake_node.next;
  fake_node.next = node;

  ++sz;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::pop_back() {
  BaseNode* node = fake_node.prev;
  fake_node.prev = fake_node.prev->prev;
  fake_node.prev->next = &fake_node;

  NodeAllocatorTraits::template destroy<NodeType>(node_allocator, &static_cast<Node*>(node)->pair);
  NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(node), 1);

  --sz;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::pop_front() {
  BaseNode* node = fake_node.next;
  fake_node.next = fake_node.next->next;
  fake_node.next->prev = &fake_node;

  NodeAllocatorTraits::template destroy<NodeType>(node_allocator, &static_cast<Node*>(node)->pair);
  NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(node), 1);

  --sz;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::insert(
        UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::iterator pos,
        const ValueType& value) {
  Node* node = NodeAllocatorTraits::allocate(node_allocator, 1);
  NodeAllocatorTraits::template construct<NodeType>(node_allocator, &node->pair, value);
  node->calculate_hash();

  pos.node->prev->next = node;
  node->next = pos.node;
  node->prev = pos.node->prev;
  pos.node->prev = node;

  ++sz;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::insert(
        UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::const_iterator pos,
        const ValueType& value) {
  insert(pos.iter_const_cast(), value);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::Node*
UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::extract(iterator pos) {
  pos.node->prev->next = pos.node->next;
  pos.node->next->prev = pos.node->prev;

  --sz;

  return static_cast<Node*>(pos.node);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::insert(iterator pos,
                                                                    List::Node* node) {
  pos.node->prev->next = node;
  node->next = pos.node;
  node->prev = pos.node->prev;
  pos.node->prev = node;

  ++sz;
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
template<typename ...Args>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::emplace(
        UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::iterator pos, Args&& ...args) {
  Node* node = NodeAllocatorTraits::allocate(node_allocator, 1);
  NodeAllocatorTraits::template construct(node_allocator, reinterpret_cast<ValueType*>(&node->pair),
                                          std::forward<Args>(args)...);
  node->calculate_hash();

  pos.node->prev->next = node;
  node->next = pos.node;
  node->prev = pos.node->prev;
  pos.node->prev = node;

  ++sz;
}


template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
template<typename ...Args>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::emplace(
        UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::const_iterator pos,
        Args&& ...args) {
  insert(pos.iter_const_cast(), std::forward<Args>(args)...);
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::erase(
        UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::iterator pos) {
  pos.node->prev->next = pos.node->next;
  pos.node->next->prev = pos.node->prev;

  NodeAllocatorTraits::template destroy<NodeType>(node_allocator,
                                                  &static_cast<Node*>(pos.node)->pair);
  NodeAllocatorTraits::deallocate(node_allocator, static_cast<Node*>(pos.node), 1);

  --sz;
}


template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::erase(
        UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::const_iterator pos) {
  erase(pos.iter_const_cast());
}

template<typename Key, typename Value, typename Hash, typename Equal, typename Allocator>
template<bool is_const>
class UnorderedMap<Key, Value, Hash, Equal, Allocator>::List::common_iterator {
  friend List;
public:

  using Type = std::conditional_t<is_const, const Node, Node>;
  using ListBaseNodeType = std::conditional_t<is_const, const BaseNode, BaseNode>;
  using ListNodeType = std::conditional_t<is_const, const Node, Node>;

  ListBaseNodeType* node;

  explicit common_iterator(ListBaseNodeType* node) : node(node) {}

  common_iterator<false> iter_const_cast() {
    return common_iterator<false>(const_cast<Node*>(node));
  }

  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = int;
  using value_type = std::remove_cv_t<Type>;
  using pointer = Type*;
  using reference = Type&;

  template<bool other_const>
  requires(is_const || !other_const)
  common_iterator(const common_iterator<other_const>& other) : node(other.node) {}

  template<bool other_const>
  requires(is_const || !other_const)
  common_iterator& operator=(const common_iterator<other_const>& other) {
    node = other.node;
    return *this;
  }

  Type& operator*() const {
    return *static_cast<ListNodeType*>(node);
  }

  Type* operator->() const {
    return static_cast<ListNodeType*>(node);
  }

  common_iterator& operator++() {
    node = node->next;
    return *this;
  }

  common_iterator& operator--() {
    node = node->prev;
    return *this;
  }

  common_iterator operator++(int) {
    common_iterator tmp = *this;
    node = node->next;
    return tmp;
  }

  common_iterator operator--(int) {
    common_iterator tmp = *this;
    node = node->prev;
    return tmp;
  }

  bool operator==(const common_iterator& other) const {
    return node == other.node;
  }
};

#endif //CPP_UNORDERED_MAP_H
