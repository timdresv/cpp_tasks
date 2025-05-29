#ifndef CPP_SMART_POINTERS_H
#define CPP_SMART_POINTERS_H

#include <iostream>

template<typename T>
class EnableSharedFromThis;

struct BaseControlBlock {
  size_t shared_count;
  size_t weak_count;

  BaseControlBlock() : shared_count(1), weak_count(0) {}

  virtual bool delete_object(void*) {
    return false;
  }

  virtual bool deallocate() {
    return false;
  }

  virtual ~BaseControlBlock() = default;
};

template<typename U, typename Allocator>
struct ControlBlockMakeShared : BaseControlBlock {
  U value;
  Allocator alloc;

  ControlBlockMakeShared() : BaseControlBlock(), value(), alloc(std::allocator<U>()) {}

  explicit ControlBlockMakeShared(const Allocator& alloc)
          : BaseControlBlock(), value(), alloc(alloc) {}

  explicit ControlBlockMakeShared(U&& value)
          : BaseControlBlock(), value(std::move(value)), alloc(std::allocator<U>()) {}

  ControlBlockMakeShared(const Allocator& alloc, U&& value)
          : BaseControlBlock(), value(std::move(value)), alloc(alloc) {}

  template<typename ...Args>
  explicit ControlBlockMakeShared(const Allocator& alloc, Args&& ... args)
          : BaseControlBlock(), value(std::forward<Args>(args)...), alloc(alloc) {}

  bool delete_object(void*) override {
    std::allocator_traits<Allocator>::destroy(alloc, &value);
    return true;
  }

  bool deallocate() override {
    using traits = std::allocator_traits<Allocator>;
    typename traits::template rebind_alloc<ControlBlockMakeShared> allocator;
    traits::template rebind_traits<ControlBlockMakeShared>::deallocate(allocator, this, 1);
    return true;
  }
};

template<typename T, typename Deleter, typename Allocator>
struct ControlBlockRegular : BaseControlBlock {
  Allocator alloc;
  Deleter del;

  ControlBlockRegular(const Allocator& alloc, const Deleter& del) : alloc(alloc), del(del) {}

  explicit ControlBlockRegular(const Deleter& del) : alloc(std::allocator<T>()), del(del) {}

  bool delete_object(void* ptr) override {
    del(ptr);
    return true;
  }

  bool deallocate() override {
    using traits = std::allocator_traits<Allocator>;
    typename traits::template rebind_alloc<ControlBlockRegular> allocator;
    traits::template rebind_traits<ControlBlockRegular>::deallocate(allocator, this, 1);
    return true;
  }
};

template<typename T>
class SharedPtr {
  template<typename U>
  friend SharedPtr<U> makeShared();

  template<typename U, typename... Args>
  friend SharedPtr<U> makeShared(Args&& ... args);

  template<typename U, typename Allocator>
  friend SharedPtr<U> allocateShared(const Allocator& alloc);

  template<typename U, typename Allocator, typename... Args>
  friend SharedPtr<U> allocateShared(const Allocator& alloc, Args&& ... args);

  template<typename> friend
  class SharedPtr;

  template<typename> friend
  class WeakPtr;

  T* ptr_;

  BaseControlBlock* block_;

  template<typename U, typename Allocator>
  explicit SharedPtr(ControlBlockMakeShared<U, Allocator>* block);

  SharedPtr(T* ptr, BaseControlBlock* block) : ptr_(ptr), block_(block) {
    ++block_->shared_count;
  }

public:
  explicit SharedPtr(T* ptr);

  SharedPtr() : ptr_(nullptr), block_(nullptr) {}

  SharedPtr(const SharedPtr& other);

  template<typename U, typename = std::enable_if<std::is_convertible<U*, T*>::value, void>::type>
  SharedPtr(const SharedPtr<U>& other);

  SharedPtr& operator=(const SharedPtr& other);

  SharedPtr(SharedPtr&& other) noexcept;

  template<typename U, typename = std::enable_if<std::is_convertible<U*, T*>::value, void>::type>
  SharedPtr(SharedPtr<U>&& other);

  SharedPtr& operator=(SharedPtr&& other) noexcept;

  ~SharedPtr();

  template<typename U, typename Deleter>
  SharedPtr(U* ptr, const Deleter& del);

  template<typename U, typename Deleter, typename Allocator>
  SharedPtr(U* ptr, const Deleter& del, const Allocator& alloc);

  template<typename U>
  SharedPtr(const SharedPtr<U>& other, T* ptr);

  template<typename U>
  SharedPtr(SharedPtr<U>&& other, T* ptr);

  T* get() const;

  T& operator*() const;

  T* operator->() const;

  void swap(SharedPtr<T>& other);

  [[nodiscard]] size_t use_count() const;

  void reset();

  void reset(T* new_ptr);
};

template<typename T>
template<typename U, typename Allocator>
SharedPtr<T>::SharedPtr(ControlBlockMakeShared<U, Allocator>* block)
        : ptr_(&block->value), block_(block) {
  if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
    ptr_->weakPtr = *this;
  }
}

template<typename T>
SharedPtr<T>::SharedPtr(T* ptr) : ptr_(ptr), block_(new BaseControlBlock()) {
  if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
    ptr_->weakPtr = *this;
  }
}

template<typename T>
SharedPtr<T>::SharedPtr(const SharedPtr& other)
        : ptr_(other.ptr_), block_(other.block_) {
  if (other.ptr_ != nullptr) {
    ++other.block_->shared_count;
  }
}

template<typename T>
template<typename U, typename>
SharedPtr<T>::SharedPtr(const SharedPtr<U>& other)
        : ptr_(other.ptr_), block_(reinterpret_cast<BaseControlBlock*>(other.block_)) {
  if (other.ptr_ != nullptr) {
    ++other.block_->shared_count;
  }
}

template<typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr& other) {
  if (&other == this) {
    return *this;
  }

  reset();

  ptr_ = other.ptr_;
  block_ = other.block_;

  ++block_->shared_count;

  return *this;
}

template<typename T>
SharedPtr<T>::SharedPtr(SharedPtr&& other) noexcept
        : ptr_(other.ptr_), block_(other.block_) {
  other.ptr_ = nullptr;
  other.block_ = nullptr;
}

template<typename T>
template<typename U, typename>
SharedPtr<T>::SharedPtr(SharedPtr<U>&& other)
        : ptr_(other.ptr_),
          block_(reinterpret_cast<BaseControlBlock*>(other.block_)) {
  other.ptr_ = nullptr;
  other.block_ = nullptr;
}

template<typename T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr&& other) noexcept {
  reset();

  ptr_ = other.ptr_;
  block_ = other.block_;

  other.ptr_ = nullptr;
  other.block_ = nullptr;

  return *this;
}

template<typename T>
SharedPtr<T>::~SharedPtr() {
  reset();
}

template<typename T>
template<typename U, typename Deleter>
SharedPtr<T>::SharedPtr(U* ptr, const Deleter& del) : ptr_(ptr) {
  using type = ControlBlockRegular<T, Deleter, std::allocator<T>>;
  std::allocator<type> alloc;
  auto* regular_block = std::allocator_traits<std::allocator<type>>::allocate(alloc, 1);
  new(regular_block) ControlBlockRegular<T, Deleter, std::allocator<T>>(del);
  block_ = regular_block;

  if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
    ptr_->weakPtr = *this;
  }
}

template<typename T>
template<typename U, typename Deleter, typename Allocator>
SharedPtr<T>::SharedPtr(U* ptr, const Deleter& del, const Allocator& alloc) :ptr_(ptr) {
  using type = ControlBlockRegular<T, Deleter, Allocator>;
  typename std::allocator_traits<Allocator>::template rebind_alloc<type> regular_alloc;
  auto* regular_block = std::allocator_traits<Allocator>::template rebind_traits<type>::allocate(
          regular_alloc, 1);
  new(regular_block) type(alloc, del);
  block_ = regular_block;

  if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
    ptr_->weakPtr = *this;
  }
}

template<typename T>
template<typename U>
SharedPtr<T>::SharedPtr(const SharedPtr<U>& other, T* ptr)
        : ptr_(ptr), block_(reinterpret_cast<BaseControlBlock*>(other.block_)) {
  if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
    ptr_->weakPtr = *this;
  }
  if (other.ptr_ != nullptr) {
    ++other.block_->shared_count;
  }
}

template<typename T>
template<typename U>
SharedPtr<T>::SharedPtr(SharedPtr<U>&& other, T* ptr)
        : ptr_(ptr), block_(reinterpret_cast<BaseControlBlock*>(other.block_)) {
  if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
    ptr_->weakPtr = *this;
  }
  other.ptr_ = nullptr;
  other.block_ = nullptr;
}

template<typename T>
T* SharedPtr<T>::get() const {
  return ptr_;
}

template<typename T>
T& SharedPtr<T>::operator*() const {
  return *ptr_;
}

template<typename T>
T* SharedPtr<T>::operator->() const {
  return ptr_;
}

template<typename T>
void SharedPtr<T>::swap(SharedPtr<T>& other) {
  std::swap(ptr_, other.ptr_);
  std::swap(block_, other.block_);
}

template<typename T>
size_t SharedPtr<T>::use_count() const {
  return block_->shared_count;
}

template<typename T>
void SharedPtr<T>::reset() {
  if (ptr_ == nullptr || block_ == nullptr) {
    return;
  }
  --block_->shared_count;
  if (block_->shared_count == 0) {
    if (!block_->delete_object(ptr_)) {
      delete ptr_;
    }
    if (block_->weak_count == 0) {
      if (!block_->deallocate()) {
        delete block_;
      }
    }
  }
  block_ = nullptr;
  ptr_ = nullptr;
}

template<typename T>
void SharedPtr<T>::reset(T* new_ptr) {
  reset();

  ptr_ = new_ptr;
  block_ = new BaseControlBlock();
}

template<typename T>
SharedPtr<T> makeShared() {
  using type = ControlBlockMakeShared<T, std::allocator<T>>;

  std::allocator<type> alloc;
  auto* block = std::allocator_traits<std::allocator<type>>::allocate(alloc, 1);
  std::allocator_traits<std::allocator<type>>::construct(alloc, block);

  SharedPtr<T> sharedPtr(block);
  return sharedPtr;
}

template<typename T, typename... Args>
SharedPtr<T> makeShared(Args&& ... args) {
  using type = ControlBlockMakeShared<T, std::allocator<T>>;

  std::allocator<type> alloc;
  auto* block = std::allocator_traits<std::allocator<type>>::allocate(alloc, 1);
  std::allocator_traits<std::allocator<type>>::construct(alloc, block, std::forward<Args>(args)...);

  SharedPtr<T> sharedPtr(block);
  return sharedPtr;
}

template<typename T, typename Allocator>
SharedPtr<T> allocateShared(const Allocator& alloc) {
  using type = ControlBlockMakeShared<T, Allocator>;
  using traits = std::allocator_traits<Allocator>::template rebind_traits<type>;

  typename std::allocator_traits<Allocator>::template rebind_alloc<type> control_alloc;
  auto* block = traits::allocate(control_alloc, 1);
  traits::construct(control_alloc, block, alloc);

  SharedPtr<T> sharedPtr(block);
  return sharedPtr;
}

template<typename T, typename Allocator, typename... Args>
SharedPtr<T> allocateShared(const Allocator& alloc, Args&& ... args) {
  using type = ControlBlockMakeShared<T, Allocator>;
  using traits = std::allocator_traits<Allocator>::template rebind_traits<type>;

  typename std::allocator_traits<Allocator>::template rebind_alloc<type> control_alloc;
  auto* block = traits::allocate(control_alloc, 1);
  traits::construct(control_alloc, block, alloc, std::forward<Args>(args)...);

  SharedPtr<T> sharedPtr(block);
  return sharedPtr;
}


template<typename T>
class WeakPtr {
  T* ptr_;
  BaseControlBlock* block_;

  template<typename> friend
  class WeakPtr;

public:
  WeakPtr();

  WeakPtr(const WeakPtr<T>& other);

  template<typename U, typename = std::enable_if<std::is_convertible<U*, T*>::value, void>::type>
  WeakPtr(const WeakPtr<U>& other);

  WeakPtr(WeakPtr<T>&& other) noexcept;

  template<typename U, typename = std::enable_if<std::is_convertible<U*, T*>::value, void>::type>
  WeakPtr(WeakPtr<U>&& other);

  WeakPtr& operator=(const WeakPtr<T>& other);

  template<typename U, typename = std::enable_if<std::is_convertible<U*, T*>::value, void>::type>
  WeakPtr& operator=(const WeakPtr<U>& other);

  WeakPtr& operator=(WeakPtr<T>&& other) noexcept;

  template<typename U, typename = std::enable_if<std::is_convertible<U*, T*>::value, void>::type>
  WeakPtr& operator=(WeakPtr<U>&& other);

  WeakPtr(const SharedPtr<T>& other);

  template<typename U, typename = std::enable_if<std::is_convertible<U*, T*>::value, void>::type>
  WeakPtr(const SharedPtr<U>& other);

  WeakPtr& operator=(const SharedPtr<T>& other);

  template<typename U, typename = std::enable_if<std::is_convertible<U*, T*>::value, void>::type>
  WeakPtr& operator=(const SharedPtr<U>& other);

  ~WeakPtr();

  void reset();

  size_t use_count();

  [[nodiscard]] bool expired() const;

  SharedPtr<T> lock() const;
};

template<typename T>
WeakPtr<T>::WeakPtr() : ptr_(nullptr), block_(nullptr) {}

template<typename T>
WeakPtr<T>::WeakPtr(const WeakPtr<T>& other)
        : ptr_(other.ptr_), block_(other.block_) {
  ++other.block_->weak_count;
}

template<typename T>
template<typename U, typename>
WeakPtr<T>::WeakPtr(const WeakPtr<U>& other)
        : ptr_(other.ptr_),
          block_(reinterpret_cast<BaseControlBlock*>(other.block_)) {
  ++other.block_->weak_count;
}

template<typename T>
WeakPtr<T>::WeakPtr(WeakPtr<T>&& other) noexcept
        : ptr_(other.ptr_), block_(other.block_) {
  other.ptr_ = nullptr;
  other.block_ = nullptr;
}

template<typename T>
template<typename U, typename>
WeakPtr<T>::WeakPtr(WeakPtr<U>&& other)
        : ptr_(other.ptr_),
          block_(reinterpret_cast<BaseControlBlock*>(other.block_)) {
  other.ptr_ = nullptr;
  other.block_ = nullptr;
}

template<typename T>
WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr<T>& other) {
  if (&other == this) {
    return *this;
  }

  reset();

  ptr_ = other.ptr_;
  block_ = other.block_;

  ++other.block_->weak_count;

  return *this;
}

template<typename T>
template<typename U, typename>
WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr<U>& other) {
  if (&other == this) {
    return *this;
  }

  reset();

  ptr_ = other.ptr_;
  block_ = reinterpret_cast<BaseControlBlock*>(other.block_);

  ++other.block_->weak_count;

  return *this;
}

template<typename T>
WeakPtr<T>& WeakPtr<T>::operator=(WeakPtr<T>&& other) noexcept {
  reset();

  ptr_ = other.ptr_;
  block_ = other.block_;

  other.ptr_ = nullptr;
  other.block_ = nullptr;

  return *this;
}

template<typename T>
template<typename U, typename>
WeakPtr<T>& WeakPtr<T>::operator=(WeakPtr<U>&& other) {
  reset();

  ptr_ = other.ptr_;
  block_ = reinterpret_cast<BaseControlBlock*>(other.block_);

  other.ptr_ = nullptr;
  other.block_ = nullptr;

  return *this;
}

template<typename T>
WeakPtr<T>::WeakPtr(const SharedPtr<T>& other)
        : ptr_(other.ptr_), block_(other.block_) {
  ++other.block_->weak_count;
}

template<typename T>
template<typename U, typename>
WeakPtr<T>::WeakPtr(const SharedPtr<U>& other)
        : ptr_(other.ptr_),
          block_(reinterpret_cast<BaseControlBlock*>(other.block_)) {
  ++other.block_->weak_count;
}

template<typename T>
WeakPtr<T>& WeakPtr<T>::operator=(const SharedPtr<T>& other) {
  reset();

  ptr_ = other.ptr_;
  block_ = other.block_;

  ++other.block_->weak_count;

  return *this;
}

template<typename T>
template<typename U, typename>
WeakPtr<T>& WeakPtr<T>::operator=(const SharedPtr<U>& other) {
  reset();

  ptr_ = other.ptr_;
  block_ = reinterpret_cast<BaseControlBlock*>(other.block_);

  ++other.block_->weak_count;

  return *this;
}

template<typename T>
WeakPtr<T>::~WeakPtr() {
  reset();
}

template<typename T>
void WeakPtr<T>::reset() {
  if (block_ == nullptr) {
    return;
  }
  --block_->weak_count;
  if (block_->shared_count == 0 && block_->weak_count == 0) {
    if (!block_->deallocate()) {
      delete block_;
    }
  }
}

template<typename T>
size_t WeakPtr<T>::use_count() {
  return block_->shared_count;
}

template<typename T>
bool WeakPtr<T>::expired() const {
  return block_->shared_count == 0;
}

template<typename T>
SharedPtr<T> WeakPtr<T>::lock() const {
  SharedPtr<T> sharedPtr(ptr_, block_);
  return sharedPtr;
}

template<typename T>
class EnableSharedFromThis {
  WeakPtr<T> weakPtr_;

  template<typename> friend
  class SharedPtr;

protected:
  EnableSharedFromThis() = default;

public:
  SharedPtr<T> shared_from_this() {
    if (weakPtr_.expired()) {
      return nullptr;
    }
    return weakPtr_.lock();
  }
};

#endif //CPP_SMART_POINTERS_H
