#ifndef CPP_DEQUE_H
#define CPP_DEQUE_H

#include <iostream>
#include <vector>

template<typename T>
class Deque {
  static const int size_blocks_ = 16;

  std::vector<T*> pointers_;

  size_t begin_block_;
  size_t begin_index_;
  size_t end_block_;
  size_t end_index_;

  T* allocate();

  void construct(T* block, size_t begin, size_t end);

  void construct(T* block, size_t begin, size_t end, const T& value);

  void construct(T* block, size_t begin, size_t end, const T* other_block);

  void destruct(T* block, size_t begin, size_t end);

  void deallocate(T* block);

  template<bool is_const>
  class common_iterator;

  template<typename ...Args>
  void help_construct(Args ...args);

public:
  Deque();

  explicit Deque(int size);

  Deque(int size, const T& object);

  Deque(const Deque& other);

  Deque& operator=(const Deque& other);

  ~Deque();

  [[nodiscard]] size_t size() const;

  T& operator[](size_t index);

  const T& operator[](size_t index) const;

  T& at(size_t index);

  const T& at(size_t index) const;

  void push_back(const T& object);

  void pop_back();

  void push_front(const T& object);

  void pop_front();

  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() { return iterator(pointers_, begin_block_, begin_index_); }

  iterator end() { return iterator(pointers_, end_block_, end_index_); }

  const_iterator begin() const { return const_iterator(pointers_, begin_block_, begin_index_); }

  const_iterator end() const { return const_iterator(pointers_, end_block_, end_index_); }

  const_iterator cbegin() const { return const_iterator(pointers_, begin_block_, begin_index_); }

  const_iterator cend() const { return const_iterator(pointers_, end_block_, end_index_); }

  reverse_iterator rbegin() { return std::reverse_iterator(end()); }

  reverse_iterator rend() { return std::reverse_iterator(begin()); }

  const_reverse_iterator rbegin() const { return std::reverse_iterator(cend()); }

  const_reverse_iterator rend() const { return std::reverse_iterator(cbegin()); }

  const_reverse_iterator crbegin() const { return std::reverse_iterator(cend()); }

  const_reverse_iterator crend() const { return std::reverse_iterator(cbegin()); }

  void insert(iterator pos, const T& object);

  void erase(iterator pos);
};

template<typename T>
template<typename... Args>
void Deque<T>::help_construct(Args... args) {
  size_t i = 0;
  try {
    for (; i < pointers_.size(); ++i) {
      pointers_[i] = allocate();
      construct(pointers_[i], 0, i == end_block_ ? end_index_ : size_blocks_, args...);
    }
  } catch (...) {
    deallocate(pointers_[i]);
    for (size_t j = 0; j < i; ++j) {
      destruct(pointers_[j], 0, j == end_block_ ? end_index_ : size_blocks_);
      deallocate(pointers_[j]);
    }
    throw;
  }
}

template<typename T>
T* Deque<T>::allocate() {
  return reinterpret_cast<T*>(new char[size_blocks_ * sizeof(T)]);
}


template<typename T>
void Deque<T>::construct(T* block, size_t begin, size_t end) {
  size_t i = begin;
  try {
    for (; i < end; ++i) {
      new(block + i) T();
    }
  } catch (...) {
    for (size_t j = 0; j < i; ++j) {
      (block + j)->~T();
    }
    throw;
  }
}

template<typename T>
void Deque<T>::construct(T* block, size_t begin, size_t end, const T& value) {
  size_t i = begin;
  try {
    for (; i < end; ++i) {
      new(block + i) T(value);
    }
  } catch (...) {
    for (size_t j = 0; j < i; ++j) {
      (block + j)->~T();
    }
    throw;
  }
}

template<typename T>
void Deque<T>::construct(T* block, size_t begin, size_t end, const T* other_block) {
  size_t i = begin;
  try {
    for (; i < end; ++i) {
      new(block + i) T(other_block[i]);
    }
  } catch (...) {
    for (size_t j = 0; j < i; ++j) {
      (block + j)->~T();
    }
    throw;
  }
}

template<typename T>
void Deque<T>::destruct(T* block, size_t begin, size_t end) {
  for (size_t j = begin; j < end; ++j) {
    (block + j)->~T();
  }
}

template<typename T>
void Deque<T>::deallocate(T* block) {
  delete[] reinterpret_cast<char*>(block);
}

template<typename T>
Deque<T>::Deque() : pointers_(std::vector<T*>(1)), begin_block_(0), begin_index_(0), end_block_(0),
                    end_index_(0) {
  pointers_[0] = allocate();
}

template<typename T>
Deque<T>::Deque(int size) : pointers_(std::vector<T*>(size / size_blocks_ + 1)), begin_block_(0),
                            begin_index_(0),
                            end_block_(size / size_blocks_), end_index_(size % size_blocks_) {
  help_construct();
}

template<typename T>
Deque<T>::Deque(int size, const T& object) : pointers_(
        std::vector<T*>(size / size_blocks_ + 1)), begin_block_(0), begin_index_(0),
                                             end_block_(size / size_blocks_),
                                             end_index_(size % size_blocks_) {
  help_construct(object);
}

template<typename T>
Deque<T>::Deque(const Deque& other) {
  pointers_.resize(other.pointers_.size());
  begin_block_ = other.begin_block_;
  begin_index_ = other.begin_index_;
  end_block_ = other.end_block_;
  end_index_ = other.end_index_;

  size_t i = 0;
  try {
    for (; i < pointers_.size(); ++i) {
      pointers_[i] = allocate();
      construct(pointers_[i], i == begin_block_ ? begin_index_ : 0,
                i == end_block_ ? end_index_ : size_blocks_, other.pointers_[i]);
    }
  } catch (...) {
    deallocate(pointers_[i]);
    for (size_t j = 0; j < i; ++j) {
      destruct(pointers_[j], j == 0 ? begin_index_ : 0,
               j + 1 == pointers_.size() ? end_index_ : size_blocks_);
      deallocate(pointers_[j]);
    }
    throw;
  }
}

template<typename T>
Deque<T>& Deque<T>::operator=(const Deque& other) {
  if (this == &other) {
    return *this;
  }

  std::vector<T*> tmp_pointers_ = pointers_;
  int tmp_begin_block_ = begin_block_;
  int tmp_begin_index_ = begin_index_;
  int tmp_end_block_ = end_block_;
  int tmp_end_index_ = end_index_;

  pointers_.resize(other.pointers_.size());
  begin_block_ = other.begin_block_;
  begin_index_ = other.begin_index_;
  end_block_ = other.end_block_;
  end_index_ = other.end_index_;

  size_t i = 0;
  try {
    for (; i < pointers_.size(); ++i) {
      pointers_[i] = allocate();
      construct(pointers_[i], i == begin_block_ ? begin_index_ : 0,
                i == end_block_ ? end_index_ : size_blocks_, other.pointers_[i]);
    }
  } catch (...) {
    for (size_t j = 0; j < i; ++j) {
      destruct(pointers_[j], j == begin_block_ ? begin_index_ : 0,
               j == end_block_ ? end_index_ : size_blocks_);
      deallocate(pointers_[j]);
    }
    deallocate(pointers_[i]);

    pointers_ = tmp_pointers_;
    begin_block_ = tmp_begin_block_;
    begin_index_ = tmp_begin_index_;
    end_block_ = tmp_end_block_;
    end_index_ = tmp_end_index_;
    throw;
  }

  for (i = 0; i < tmp_pointers_.size(); ++i) {
    destruct(tmp_pointers_[i], i == static_cast<size_t>(tmp_begin_block_) ? tmp_begin_index_ : 0,
             i == static_cast<size_t>(tmp_end_block_) ? tmp_end_index_ : size_blocks_);
    deallocate(tmp_pointers_[i]);
  }

  return *this;
}

template<typename T>
Deque<T>::~Deque() {
  for (size_t i = begin_block_; i <= end_block_; ++i) {
    destruct(pointers_[i], i == begin_block_ ? begin_index_ : 0,
             i == end_block_ ? end_index_ : size_blocks_);
  }
  for (size_t i = 0; i < pointers_.size(); ++i) {
    deallocate(pointers_[i]);
  }
}

template<typename T>
size_t Deque<T>::size() const {
  return (end_block_ - begin_block_) * size_blocks_ + end_index_ - begin_index_;
}

template<typename T>
T& Deque<T>::operator[](size_t index) {
  return pointers_[begin_block_ + (begin_index_ + index) / size_blocks_][(begin_index_ + index) %
                                                                         size_blocks_];
}

template<typename T>
const T& Deque<T>::operator[](size_t index) const {
  return pointers_[begin_block_ + (begin_index_ + index) / size_blocks_][(begin_index_ + index) %
                                                                         size_blocks_];
}

template<typename T>
T& Deque<T>::at(size_t index) {
  if (index >= size()) {
    throw std::out_of_range("");
  }

  return pointers_[begin_block_ + (begin_index_ + index) / size_blocks_][
          (begin_index_ + index) % size_blocks_];
}

template<typename T>
const T& Deque<T>::at(size_t index) const {
  if (index >= size()) {
    throw std::out_of_range("");
  }

  return pointers_[begin_block_ + (begin_index_ + index) / size_blocks_][
          (begin_index_ + index) % size_blocks_];
}

template<typename T>
void Deque<T>::push_back(const T& object) {
  if (end_block_ + 1 == pointers_.size() && end_index_ + 1 == size_blocks_) {
    pointers_.insert(pointers_.end(), end_block_ - begin_block_ + 1, nullptr);
    for (size_t i = end_block_ + 1; i < pointers_.size(); ++i) {
      pointers_[i] = allocate();
    }
  }

  new(pointers_[end_block_] + end_index_) T(object);
  ++end_index_;

  if (end_index_ == size_blocks_) {
    end_index_ = 0;
    ++end_block_;
  }
}

template<typename T>
void Deque<T>::pop_back() {
  if (end_index_ == 0) {
    end_index_ = size_blocks_;
    --end_block_;
  }

  --end_index_;
  (pointers_[end_block_] + end_index_)->~T();
}

template<typename T>
void Deque<T>::push_front(const T& object) {
  if (begin_block_ == 0 && begin_index_ == 0) {
    pointers_.insert(pointers_.begin(), end_block_ + 1, nullptr);
    for (size_t i = 0; i < end_block_ + 1; ++i) {
      pointers_[i] = allocate();
    }
    begin_block_ = end_block_ + 1;
    end_block_ = end_block_ * 2 + 1;
  }

  if (begin_index_ == 0) {
    begin_index_ = size_blocks_;
    --begin_block_;
  }

  try {
    --begin_index_;
    new(pointers_[begin_block_] + begin_index_) T(object);
  } catch (...) {
    ++begin_index_;

    if (begin_index_ == 16) {
      ++begin_block_;
      begin_index_ = 0;
    }
    throw;
  }
}

template<typename T>
void Deque<T>::pop_front() {
  (pointers_[begin_block_] + begin_index_)->~T();
  ++begin_index_;

  if (begin_index_ == size_blocks_) {
    begin_index_ = 0;
    ++begin_block_;
  }
}

template<typename T>
void Deque<T>::insert(Deque::iterator pos, const T& object) {
  push_back(object);

  for (auto it = end() - 1; it != pos; --it) {
    *it = *(it - 1);
  }

  *pos = object;
}

template<typename T>
void Deque<T>::erase(Deque::iterator pos) {
  for (auto it = pos; it + 1 != end(); ++it) {
    *it = *(it + 1);
  }

  pop_back();
}

template<typename T>
template<bool is_const>
class Deque<T>::common_iterator {
  friend Deque;

  using Type = std::conditional_t<is_const, const T, T>;
  using vectorType = std::conditional_t<is_const, const std::vector<T*>, std::vector<T*>>;
  using PointerType = std::conditional_t<is_const, T* const*, T**>;

  static const int size_blocks_ = 16;
  PointerType pointers_;

  Type* block_;

  int pointers_size_;

  int block_index_;

  int index_;

  common_iterator(vectorType& pointers, int block_index, int index)
          : pointers_(pointers.data()), block_(pointers_[block_index]),
            pointers_size_(pointers.size()), block_index_(block_index), index_(index) {}

public:

  using iterator_concept = std::random_access_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::remove_cv_t<Type>;
  using difference_type = int;
  using pointer = Type*;
  using reference = Type&;

  template<bool other_const>
  requires(is_const || !other_const)
  common_iterator(const common_iterator<other_const>& other) : pointers_(other.pointers_),
                                                               block_(other.block_),
                                                               pointers_size_(other.pointers_size_),
                                                               block_index_(other.iter_),
                                                               index_(other.index_) {}


  template<bool other_const>
  requires(is_const || !other_const)
  common_iterator& operator=(const common_iterator<other_const>& other) {
    pointers_ = other.pointers_;
    block_ = other.block_;
    pointers_size_ = other.pointers_size_;
    block_index_ = other.iter_;
    index_ = other.index_;
    return *this;
  }

  Type& operator*() const {
    return block_[index_];
  }

  Type* operator->() const {
    return block_ + index_;
  }

  common_iterator& operator++() {
    ++index_;

    if (index_ == size_blocks_) {
      index_ = 0;
      block_ = pointers_[++block_index_ % pointers_size_];
    }

    return *this;
  }

  common_iterator& operator--() {
    --index_;

    if (index_ < 0) {
      index_ = size_blocks_ - 1;
      block_ = pointers_[--block_index_ % pointers_size_];
    }

    return *this;
  }

  common_iterator operator++(int) {
    common_iterator tmp = *this;
    ++*this;
    return tmp;
  }

  common_iterator operator--(int) {
    common_iterator tmp = *this;
    --*this;
    return tmp;
  }

  common_iterator& operator+=(int shift) {
    if (index_ + shift < 0) {
      block_index_ += (index_ + shift + 1) / size_blocks_ - 1;
      block_ = pointers_[block_index_ % pointers_size_];
    } else {
      block_index_ += (index_ + shift) / size_blocks_;
      block_ = pointers_[block_index_ % pointers_size_];
    }

    index_ = (size_blocks_ + index_ + shift % size_blocks_) % size_blocks_;
    return *this;
  }

  common_iterator& operator-=(int shift) {
    return *this += -shift;
  }

  common_iterator operator+(int shift) const {
    common_iterator tmp = *this;
    tmp += shift;
    return tmp;
  }

  common_iterator operator-(int shift) const {
    common_iterator tmp = *this;
    tmp -= shift;
    return tmp;
  }

  int operator-(const common_iterator& other) const {
    return (block_index_ - other.block_index_) * size_blocks_ + index_ - other.index_;
  }

  bool operator==(const common_iterator& other) const {
    return block_index_ == other.block_index_ && index_ == other.index_;
  }

  std::strong_ordering operator<=>(const common_iterator& other) const {
    if (block_index_ < other.block_index_ ||
        (block_index_ == other.block_index_ && index_ < other.index_)) {
      return std::strong_ordering::less;
    }

    if (block_ == other.block_ && index_ == other.index_) {
      return std::strong_ordering::equivalent;
    }

    return std::strong_ordering::greater;
  }
};

#endif //CPP_DEQUE_H
