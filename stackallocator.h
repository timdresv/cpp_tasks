#ifndef CPP_STACKALLOCATOR_H
#define CPP_STACKALLOCATOR_H

#include <iostream>

template<size_t N>
class StackStorage {
  void* top_ = stack_;

  char stack_[N]{};

public:

  StackStorage() : stack_() {};

  StackStorage(const StackStorage&) = delete;

  StackStorage& operator=(const StackStorage&) = delete;

  template<typename T>
  T* allocate(size_t n);
};

template<size_t N>
template<typename T>
T* StackStorage<N>::allocate(size_t n) {
  int align = alignof(T);
  auto num_ptr = reinterpret_cast<std::uintptr_t>(top_);
  if (num_ptr & (align - 1)) {
    num_ptr = (num_ptr & ~(align - 1)) + align;
  }

  T* ptr = reinterpret_cast<T*>(num_ptr);
  T* top_ptr = ptr + n;

  top_ = reinterpret_cast<char*>(top_ptr);

  return ptr;
}

template<typename T, size_t N>
class StackAllocator {
  StackStorage<N>* storage_;

  template<typename, size_t>
  friend class StackAllocator;

public:
  explicit StackAllocator(StackStorage<N>& stack) : storage_(&stack) {}

  template<typename U>
  StackAllocator(const StackAllocator<U, N>& alloc) : storage_(alloc.storage_) {}

  template<typename U>
  StackAllocator& operator=(const StackAllocator<U, N>& alloc);

  T* allocate(size_t n);

  void deallocate(T*, size_t) {}

  using value_type = T;

  template<typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  template<typename U>
  bool operator==(const StackAllocator<U, N>& other) const;
};

template<typename T, size_t N>
template<typename U>
StackAllocator<T, N>& StackAllocator<T, N>::operator=(const StackAllocator<U, N>& alloc) {
  storage_ = alloc.storage_;
  return *this;
}

template<typename T, size_t N>
T* StackAllocator<T, N>::allocate(size_t n) {
  return storage_->template allocate<T>(n);
}

template<typename T, size_t N>
template<typename U>
bool StackAllocator<T, N>::operator==(const StackAllocator<U, N>& other) const {
  return storage_ == other.storage_;
}

template<typename T, typename Allocator=std::allocator<T>>
class List {
  struct BaseNode {
    BaseNode* next;
    BaseNode* prev;
  };

  struct Node : BaseNode {
    T value;
  };

  using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using NodeAllocatorTraits = typename std::allocator_traits<NodeAllocator>;

  BaseNode fake_node_;
  size_t sz_;

  [[no_unique_address]] Allocator allocator_;
  [[no_unique_address]] NodeAllocator node_allocator_;

  template<bool is_const>
  class common_iterator;

  void swap(List& other) {
    if (std::allocator_traits<NodeAllocator>::propagate_on_container_swap::value) {
      std::swap(allocator_, other.allocator_);
      std::swap(node_allocator_, other.node_allocator_);
    }

    std::swap(sz_, other.sz_);
    std::swap(fake_node_, other.fake_node_);

    if (sz_ != 0) {
      fake_node_.next->prev = &fake_node_;
      fake_node_.prev->next = &fake_node_;
    } else {
      fake_node_.next = &fake_node_;
      fake_node_.prev = &fake_node_;
    }
    if (other.sz_ != 0) {
      other.fake_node_.next->prev = &other.fake_node_;
      other.fake_node_.prev->next = &other.fake_node_;
    } else {
      other.fake_node_.next = &other.fake_node_;
      other.fake_node_.prev = &other.fake_node_;
    }
  }

  void delete_from_node(BaseNode* node) {
    NodeAllocatorTraits::deallocate(node_allocator_, static_cast<Node*>(node->next), 1);
    while (node != &fake_node_) {
      node = node->prev;
      NodeAllocatorTraits::template destroy<T>(node_allocator_,
                                               &static_cast<Node*>(node->next)->value);
      NodeAllocatorTraits::deallocate(node_allocator_, static_cast<Node*>(node->next), 1);
    }
  }

public:
  explicit List(const Allocator& alloc = Allocator());

  explicit List(size_t count, const T& value, const Allocator& alloc = Allocator());

  explicit List(size_t count, const Allocator& alloc = Allocator());

  List(const List& other);

  List(const List& other, const Allocator& alloc);

  List& operator=(const List& other);

  ~List();

  Allocator get_allocator() const { return allocator_; }

  [[nodiscard]] size_t size() const { return sz_; }

  void push_back(const T& value);

  void push_front(const T& value);

  void pop_back();

  void pop_front();

  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() { return iterator(fake_node_.next); }

  iterator end() { return iterator(&fake_node_); }

  const_iterator begin() const { return const_iterator(fake_node_.next); }

  const_iterator end() const { return const_iterator(&fake_node_); }

  const_iterator cbegin() const { return const_iterator(fake_node_.next); }

  const_iterator cend() const { return const_iterator(&fake_node_); }

  reverse_iterator rbegin() { return std::reverse_iterator(end()); }

  reverse_iterator rend() { return std::reverse_iterator(begin()); }

  const_reverse_iterator rbegin() const { return std::reverse_iterator(cend()); }

  const_reverse_iterator rend() const { return std::reverse_iterator(cbegin()); }

  const_reverse_iterator crbegin() const { return std::reverse_iterator(cend()); }

  const_reverse_iterator crend() const { return std::reverse_iterator(cbegin()); }

  void insert(iterator pos, const T& value);

  void insert(const_iterator pos, const T& value);

  void erase(iterator pos);

  void erase(const_iterator pos);
};

template<typename T, typename Allocator>
List<T, Allocator>::List(const Allocator& alloc)
        : fake_node_(BaseNode{&fake_node_, &fake_node_}), sz_(0), allocator_(alloc),
          node_allocator_(NodeAllocator(allocator_)) {}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t count, const T& value, const Allocator& alloc)
        : fake_node_(BaseNode{&fake_node_, &fake_node_}), sz_(count), allocator_(alloc),
          node_allocator_(NodeAllocator(alloc)) {
  BaseNode* prev_node = &fake_node_;
  try {
    for (size_t i = 0; i < count; ++i) {
      prev_node->next = NodeAllocatorTraits::allocate(node_allocator_, 1);
      NodeAllocatorTraits::template construct<T>(node_allocator_,
                                                 &static_cast<Node*>(prev_node->next)->value,
                                                 value);

      prev_node->next->prev = prev_node;
      prev_node = prev_node->next;
    }

    prev_node->next = &fake_node_;
    fake_node_.prev = prev_node;
  } catch (...) {
    delete_from_node(prev_node);
    throw;
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(size_t count, const Allocator& alloc)
        : fake_node_(BaseNode{&fake_node_, &fake_node_}), sz_(count), allocator_(alloc),
          node_allocator_(NodeAllocator(allocator_)) {
  BaseNode* prev_node = &fake_node_;
  try {
    for (size_t i = 0; i < count; ++i) {
      prev_node->next = NodeAllocatorTraits::allocate(node_allocator_, 1);
      NodeAllocatorTraits::template construct<T>(node_allocator_,
                                                 &static_cast<Node*>(prev_node->next)->value);

      prev_node->next->prev = prev_node;
      prev_node = prev_node->next;
    }

    prev_node->next = &fake_node_;
    fake_node_.prev = prev_node;
  } catch (...) {
    delete_from_node(prev_node);
    throw;
  }
}

template<typename T, typename Allocator>
List<T, Allocator>::List(const List& other)
        : List(other, std::allocator_traits<Allocator>::select_on_container_copy_construction(
        other.get_allocator())) {}

template<typename T, typename Allocator>
List<T, Allocator>::List(const List& other, const Allocator& alloc)
        : fake_node_(BaseNode{&fake_node_, &fake_node_}), sz_(other.sz_),
          allocator_(alloc), node_allocator_(allocator_) {
  BaseNode* prev_node = &fake_node_;
  try {
    BaseNode* node = other.fake_node_.next;
    for (size_t i = 0; i < other.sz_; ++i) {
      prev_node->next = NodeAllocatorTraits::allocate(node_allocator_, 1);
      NodeAllocatorTraits::template construct<T>(node_allocator_,
                                                 &static_cast<Node*>(prev_node->next)->value,
                                                 static_cast<Node*>(node)->value);

      prev_node->next->prev = prev_node;
      prev_node = prev_node->next;
      node = node->next;
    }

    prev_node->next = &fake_node_;
    fake_node_.prev = prev_node;
  } catch (...) {
    delete_from_node(prev_node);
    throw;
  }
}

template<typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(const List& other) {
  if (this == &other) {
    return *this;
  }

  Allocator alloc = (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value
                     ? other.get_allocator() : allocator_);
  List<T, Allocator> copy(other, alloc);

  swap(copy);

  if (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
    allocator_ = other.get_allocator();
    node_allocator_ = allocator_;
  }

  return *this;
}

template<typename T, typename Allocator>
List<T, Allocator>::~List() {
  BaseNode* node = fake_node_.next;
  while (node != &fake_node_) {
    NodeAllocatorTraits::template destroy<T>(node_allocator_, &static_cast<Node*>(node)->value);
    node = node->next;
    NodeAllocatorTraits::deallocate(node_allocator_, static_cast<Node*>(node->prev), 1);
  }
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_back(const T& value) {
  Node* node = NodeAllocatorTraits::allocate(node_allocator_, 1);

  try {
    NodeAllocatorTraits::template construct<T>(node_allocator_, &node->value, value);
  } catch (...) {
    NodeAllocatorTraits::deallocate(node_allocator_, node, 1);
    throw;
  }

  fake_node_.prev->next = node;
  node->next = &fake_node_;
  node->prev = fake_node_.prev;
  fake_node_.prev = node;

  ++sz_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& value) {
  Node* node = NodeAllocatorTraits::allocate(node_allocator_, 1);

  try {
    NodeAllocatorTraits::template construct<T>(node_allocator_, &node->value, value);
  } catch (...) {
    NodeAllocatorTraits::deallocate(node_allocator_, node, 1);
    throw;
  }

  fake_node_.next->prev = node;
  node->prev = &fake_node_;
  node->next = fake_node_.next;
  fake_node_.next = node;

  ++sz_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::pop_back() {
  BaseNode* node = fake_node_.prev;
  fake_node_.prev = fake_node_.prev->prev;
  fake_node_.prev->next = &fake_node_;

  NodeAllocatorTraits::template destroy<T>(node_allocator_, &static_cast<Node*>(node)->value);
  NodeAllocatorTraits::deallocate(node_allocator_, static_cast<Node*>(node), 1);

  --sz_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::pop_front() {
  BaseNode* node = fake_node_.next;
  fake_node_.next = fake_node_.next->next;
  fake_node_.next->prev = &fake_node_;

  NodeAllocatorTraits::template destroy<T>(node_allocator_, &static_cast<Node*>(node)->value);
  NodeAllocatorTraits::deallocate(node_allocator_, static_cast<Node*>(node), 1);

  --sz_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::insert(List::iterator pos, const T& value) {
  Node* node = NodeAllocatorTraits::allocate(node_allocator_, 1);

  try {
    NodeAllocatorTraits::template construct<T>(node_allocator_, &node->value, value);
  } catch (...) {
    NodeAllocatorTraits::deallocate(node_allocator_, node, 1);
    throw;
  }

  pos.node->prev->next = node;
  node->next = pos.node;
  node->prev = pos.node->prev;
  pos.node->prev = node;

  ++sz_;
}

template<typename T, typename Allocator>
void List<T, Allocator>::insert(List::const_iterator pos, const T& value) {
  insert(pos.iter_const_cast(), value);
}

template<typename T, typename Allocator>
void List<T, Allocator>::erase(List::iterator pos) {
  pos.node->prev->next = pos.node->next;
  pos.node->next->prev = pos.node->prev;

  NodeAllocatorTraits::template destroy<T>(node_allocator_, &static_cast<Node*>(pos.node)->value);
  NodeAllocatorTraits::deallocate(node_allocator_, static_cast<Node*>(pos.node), 1);

  --sz_;
}


template<typename T, typename Allocator>
void List<T, Allocator>::erase(List::const_iterator pos) {
  erase(pos.iter_const_cast());
}

template<typename T, typename Allocator>
template<bool is_const>
class List<T, Allocator>::common_iterator {
  friend List;

  using Type = std::conditional_t<is_const, const T, T>;
  using BaseNodeType = std::conditional_t<is_const, const BaseNode, BaseNode>;
  using NodeType = std::conditional_t<is_const, const Node, Node>;

  BaseNodeType* node;

  explicit common_iterator(BaseNodeType* node) : node(node) {}

  common_iterator<false> iter_const_cast() {
    return common_iterator<false>(const_cast<BaseNode*>(node));
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
  common_iterator(const common_iterator<other_const>& other) : node(other.node) {}

  common_iterator() : node(nullptr) {}

  template<bool other_const>
  requires(is_const || !other_const)
  common_iterator& operator=(const common_iterator<other_const>& other) {
    node = other.node;
    return *this;
  }

  Type& operator*() const {
    return static_cast<NodeType*>(node)->value;
  }

  Type* operator->() const {
    return &static_cast<NodeType*>(node)->value;
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

#endif //CPP_STACKALLOCATOR_H
