#pragma once

template<typename T>
class Function;

template<typename T>
class MoveOnlyFunction;

template<typename Ret, typename ...Args>
class Function<Ret(Args...)> {
  friend MoveOnlyFunction<Ret(Args...)>;

  static const size_t BUFFER_SIZE = 16;
  alignas(max_align_t) char buffer[BUFFER_SIZE];

  void* fptr;

  using invoke_ptr_t = Ret(*)(void*, Args...);
  using copy_ptr_t = void* (*)(void*, void*);
  using move_ptr_t = void* (*)(void*, void*);
  using destroy_ptr_t = void (*)(void*);
  using type_ptr_t = const std::type_info& (*)(void*);

  invoke_ptr_t invoke_ptr;
  copy_ptr_t copy_ptr;
  move_ptr_t move_ptr;
  destroy_ptr_t destroy_ptr;
  type_ptr_t type_ptr;

  template<typename F>
  static const std::type_info& type_fptr(F* fptr);;

  template<typename F>
  static Ret invoke_fptr(F* fptr, Args... args);

  template<typename F>
  static F* copy_fptr(F* other_fptr, void* buffer);

  template<typename F>
  static F* move_fptr(F* other_fptr, void* buffer);

  template<typename F>
  static void destroy_fptr(F* fptr);

public:

  Function();

  explicit Function(nullptr_t);

  template<typename F>
  requires (std::is_invocable_v<F, Args...> &&
            !std::is_same_v<Function, std::remove_reference_t<F>>)
  Function(F&& func);

  template<typename F>
  requires (std::is_invocable_v<F, Args...> &&
            !std::is_same_v<Function, std::remove_reference_t<F>>)
  Function& operator=(F&& func);

  Function(const Function& other);

  Function(Function&& other) noexcept;

  Function& operator=(const Function& other);

  Function& operator=(Function&& other) noexcept;

  operator bool() const;

  operator void*() const;

  Ret operator()(Args... args) const;

  ~Function();

  template<typename F>
  F* target();

  const std::type_info& target_type();
};

template<typename Ret, typename... Args>
template<typename F>
const std::type_info& Function<Ret(Args...)>::type_fptr(F*) {
  return typeid(F);
}

template<typename Ret, typename... Args>
template<typename F>
Ret Function<Ret(Args...)>::invoke_fptr(F* fptr, Args... args) {
  return std::invoke(*fptr, std::forward<Args>(args)...);
}

template<typename Ret, typename... Args>
template<typename F>
F* Function<Ret(Args...)>::copy_fptr(F* other_fptr, void* buffer) {
  if constexpr (requires(F){ sizeof(F); }) {
    if constexpr (sizeof(F) > BUFFER_SIZE) {
      return new F(*other_fptr);
    } else {
      new(buffer) F(*other_fptr);
      return reinterpret_cast<F*>(buffer);
    }
  } else {
    return other_fptr;
  }
}

template<typename Ret, typename... Args>
template<typename F>
F* Function<Ret(Args...)>::move_fptr(F* other_fptr, void* buffer) {
  if constexpr (requires(F){ sizeof(F); }) {
    if constexpr (sizeof(F) > BUFFER_SIZE) {
      return other_fptr;
    } else {
      new(buffer) F(std::move(*other_fptr));
      return reinterpret_cast<F*>(buffer);
    }
  } else {
    return other_fptr;
  }
}

template<typename Ret, typename... Args>
template<typename F>
void Function<Ret(Args...)>::destroy_fptr(F* fptr) {
  if constexpr (requires(F){ sizeof(F); }) {
    if constexpr (sizeof(F) > BUFFER_SIZE) {
      delete fptr;
    } else {
      fptr->~F();
    }
  }
}

template<typename Ret, typename... Args>
Function<Ret(Args...)>::Function() : fptr(nullptr), invoke_ptr(nullptr), copy_ptr(nullptr),
                                     move_ptr(nullptr), destroy_ptr(nullptr), type_ptr(nullptr) {}

template<typename Ret, typename... Args>
Function<Ret(Args...)>::Function(nullptr_t) : Function() {}

template<typename Ret, typename... Args>
template<typename F>
requires (std::is_invocable_v<F, Args...> &&
          !std::is_same_v<Function<Ret(Args...)>, std::remove_reference_t<F>>)
Function<Ret(Args...)>::Function(F&& func)
        : invoke_ptr(reinterpret_cast<invoke_ptr_t>(&invoke_fptr<
        std::remove_pointer_t<std::remove_reference_t<F>>>)),
          copy_ptr(reinterpret_cast<copy_ptr_t>(&copy_fptr<
                  std::remove_pointer_t<std::remove_reference_t<F>>>)),
          move_ptr(reinterpret_cast<move_ptr_t>(&move_fptr<
                  std::remove_pointer_t<std::remove_reference_t<F>>>)),
          destroy_ptr(reinterpret_cast<destroy_ptr_t>(&destroy_fptr<
                  std::remove_pointer_t<std::remove_reference_t<F>>>)),
          type_ptr(reinterpret_cast<type_ptr_t>(&type_fptr<
                  std::remove_pointer_t<std::remove_reference_t<F>>>)) {
  using T = std::remove_pointer_t<std::remove_reference_t<F>>;

  if constexpr (requires(T){ sizeof(T); }) {
    if constexpr (sizeof(T) > BUFFER_SIZE) {
      fptr = new std::remove_reference_t<F>(std::forward<F>(func));
    } else {
      new(buffer) std::remove_reference_t<F>(std::forward<F>(func));
      fptr = buffer;
    }
  } else {
    fptr = reinterpret_cast<void*>(func);
  }
}

template<typename Ret, typename... Args>
template<typename F>
requires (std::is_invocable_v<F, Args...> &&
          !std::is_same_v<Function<Ret(Args...)>, std::remove_reference_t<F>>)
Function<Ret(Args...)>& Function<Ret(Args...)>::operator=(F&& func) {
  if (destroy_ptr) {
    destroy_ptr(fptr);
  }

  using T = std::remove_pointer_t<std::remove_reference_t<F>>;

  invoke_ptr = reinterpret_cast<invoke_ptr_t>(&invoke_fptr<T>);
  copy_ptr = reinterpret_cast<copy_ptr_t>(&copy_fptr<T>);
  move_ptr = reinterpret_cast<move_ptr_t>(&move_fptr<T>);
  destroy_ptr = reinterpret_cast<destroy_ptr_t>(&destroy_fptr<T>);
  type_ptr = reinterpret_cast<type_ptr_t>(&type_fptr<T>);

  if constexpr (requires(T){ sizeof(T); }) {
    if constexpr (sizeof(T) > BUFFER_SIZE) {
      fptr = new std::remove_reference_t<F>(std::forward<F>(func));
    } else {
      new(buffer) std::remove_reference_t<F>(std::forward<F>(func));
      fptr = buffer;
    }
  } else {
    fptr = reinterpret_cast<void*>(func);
  }

  return *this;
}

template<typename Ret, typename... Args>
Function<Ret(Args...)>::Function(const Function& other) : invoke_ptr(other.invoke_ptr),
                                                          copy_ptr(other.copy_ptr),
                                                          move_ptr(other.move_ptr),
                                                          destroy_ptr(other.destroy_ptr),
                                                          type_ptr(other.type_ptr) {
  if (copy_ptr) {
    fptr = copy_ptr(other.fptr, buffer);
  }
}

template<typename Ret, typename... Args>
Function<Ret(Args...)>::Function(Function&& other) noexcept: invoke_ptr(other.invoke_ptr),
                                                             copy_ptr(other.copy_ptr),
                                                             move_ptr(other.move_ptr),
                                                             destroy_ptr(other.destroy_ptr),
                                                             type_ptr(other.type_ptr) {
  if (move_ptr) {
    fptr = move_ptr(other.fptr, buffer);
    other.fptr = nullptr;
  }

  other.invoke_ptr = nullptr;
  other.copy_ptr = nullptr;
  other.move_ptr = nullptr;
  other.destroy_ptr = nullptr;
}

template<typename Ret, typename... Args>
Function<Ret(Args...)>& Function<Ret(Args...)>::operator=(const Function& other) {
  if (&other == this) {
    return *this;
  }

  if (destroy_ptr) {
    destroy_ptr(fptr);
  }

  invoke_ptr = other.invoke_ptr;
  copy_ptr = other.copy_ptr;
  move_ptr = other.move_ptr;
  destroy_ptr = other.destroy_ptr;
  type_ptr = other.type_ptr;

  if (copy_ptr) {
    fptr = copy_ptr(other.fptr, buffer);
  }

  return *this;
}

template<typename Ret, typename... Args>
Function<Ret(Args...)>& Function<Ret(Args...)>::operator=(Function&& other) noexcept {
  if (destroy_ptr) {
    destroy_ptr(fptr);
  }

  invoke_ptr = other.invoke_ptr;
  copy_ptr = other.copy_ptr;
  move_ptr = other.move_ptr;
  destroy_ptr = other.destroy_ptr;
  type_ptr = other.type_ptr;

  if (move_ptr) {
    fptr = move_ptr(other.fptr, buffer);
    other.fptr = nullptr;
  }

  other.invoke_ptr = nullptr;
  other.copy_ptr = nullptr;
  other.move_ptr = nullptr;
  other.destroy_ptr = nullptr;
  other.type_ptr = nullptr;

  return *this;
}

template<typename Ret, typename... Args>
Function<Ret(Args...)>::operator bool() const {
  return fptr;
}

template<typename Ret, typename... Args>
Function<Ret(Args...)>::operator void*() const {
  return fptr;
}

template<typename Ret, typename... Args>
Ret Function<Ret(Args...)>::operator()(Args... args) const {
  if (!fptr) {
    throw std::bad_function_call();
  }
  return invoke_ptr(fptr, std::forward<Args>(args)...);
}

template<typename Ret, typename... Args>
Function<Ret(Args...)>::~Function() {
  if (destroy_ptr) {
    destroy_ptr(fptr);
  }
}

template<typename Ret, typename... Args>
template<typename F>
F* Function<Ret(Args...)>::target() {
  return reinterpret_cast<F*>(fptr);
}

template<typename Ret, typename... Args>
const std::type_info& Function<Ret(Args...)>::target_type() {
  return type_ptr(fptr);
}

template<typename>
struct guide_helper;

template<typename Ret, typename T, typename... Args>
struct guide_helper<Ret (T::*)(Args...)> {
  using type = Ret(Args...);
};

template<typename Ret, typename T, typename... Args>
struct guide_helper<Ret (T::*)(Args...)&> {
  using type = Ret(Args...);
};

template<typename Ret, typename T, typename... Args>
struct guide_helper<Ret (T::*)(Args...) const> {
  using type = Ret(Args...);
};

template<typename Ret, typename T, typename... Args>
struct guide_helper<Ret (T::*)(Args...) const&> {
  using type = Ret(Args...);
};

template<typename Ret, typename... Args>
Function(Ret(*)(Args...)) -> Function<Ret(Args...)>;

template<typename F, typename Signature = typename guide_helper<decltype(&F::operator())>::type>
Function(F) -> Function<Signature>;

template<typename Ret, typename ...Args>
class MoveOnlyFunction<Ret(Args...)> {
  static const size_t BUFFER_SIZE = 16;
  alignas(max_align_t) char buffer[BUFFER_SIZE];

  void* fptr;

  using invoke_ptr_t = Ret(*)(void*, Args...);
  using move_ptr_t = void* (*)(void*, void*);
  using destroy_ptr_t = void (*)(void*);
  using type_ptr_t = const std::type_info& (*)(void*);

  invoke_ptr_t invoke_ptr;
  move_ptr_t move_ptr;
  destroy_ptr_t destroy_ptr;
  type_ptr_t type_ptr;

public:

  MoveOnlyFunction();

  explicit MoveOnlyFunction(nullptr_t);

  template<typename F>
  requires (std::is_invocable_v<F, Args...> &&
            !std::is_same_v<MoveOnlyFunction, std::remove_reference_t<F>>)
  MoveOnlyFunction(F&& func);

  template<typename F>
  requires (std::is_invocable_v<F, Args...> &&
            !std::is_same_v<MoveOnlyFunction, std::remove_reference_t<F>>)
  MoveOnlyFunction& operator=(F&& func);

  MoveOnlyFunction(MoveOnlyFunction&& other) noexcept;

  MoveOnlyFunction& operator=(MoveOnlyFunction&& other) noexcept;

  operator bool() const;

  operator void*() const;

  Ret operator()(Args... args) const;

  ~MoveOnlyFunction();

  template<typename F>
  F* target();

  const std::type_info& target_type();
};

template<typename Ret, typename... Args>
MoveOnlyFunction<Ret(Args...)>::MoveOnlyFunction() : fptr(nullptr), invoke_ptr(nullptr),
                                                     move_ptr(nullptr), destroy_ptr(nullptr),
                                                     type_ptr(nullptr) {}

template<typename Ret, typename... Args>
MoveOnlyFunction<Ret(Args...)>::MoveOnlyFunction(nullptr_t) : MoveOnlyFunction() {}

template<typename Ret, typename... Args>
template<typename F>
requires (std::is_invocable_v<F, Args...> &&
          !std::is_same_v<MoveOnlyFunction<Ret(Args...)>, std::remove_reference_t<F>>)
MoveOnlyFunction<Ret(Args...)>::MoveOnlyFunction(F&& func)
        : invoke_ptr(reinterpret_cast<invoke_ptr_t>(&Function<Ret(Args...)>::template
invoke_fptr<std::remove_pointer_t<std::remove_reference_t<F>>>)),
          move_ptr(reinterpret_cast<move_ptr_t>(&Function<Ret(Args...)>::template
          move_fptr<std::remove_pointer_t<std::remove_reference_t<F>>>)),
          destroy_ptr(reinterpret_cast<destroy_ptr_t>(&Function<Ret(Args...)>::template
          destroy_fptr<std::remove_pointer_t<std::remove_reference_t<F>>>)),
          type_ptr(reinterpret_cast<type_ptr_t>(&Function<Ret(Args...)>::template
          type_fptr<std::remove_pointer_t<std::remove_reference_t<F>>>)) {
  using T = std::remove_pointer_t<std::remove_reference_t<F>>;

  if constexpr (requires(T){ sizeof(T); }) {
    if constexpr (sizeof(T) > BUFFER_SIZE) {
      fptr = new std::remove_reference_t<F>(std::forward<F>(func));
    } else {
      new(buffer) std::remove_reference_t<F>(std::forward<F>(func));
      fptr = buffer;
    }
  } else {
    fptr = reinterpret_cast<void*>(func);
  }
}

template<typename Ret, typename... Args>
template<typename F>
requires (std::is_invocable_v<F, Args...> &&
          !std::is_same_v<MoveOnlyFunction<Ret(Args...)>, std::remove_reference_t<F>>)
MoveOnlyFunction<Ret(Args...)>& MoveOnlyFunction<Ret(Args...)>::operator=(F&& func) {
  if (destroy_ptr) {
    destroy_ptr(fptr);
  }

  using T = std::remove_pointer_t<std::remove_reference_t<F>>;

  invoke_ptr = reinterpret_cast<invoke_ptr_t>(&Function<Ret(Args...)>::template invoke_fptr<T>);
  move_ptr = reinterpret_cast<move_ptr_t>(&Function<Ret(Args...)>::template move_fptr<T>);
  destroy_ptr = reinterpret_cast<destroy_ptr_t>(&Function<Ret(
          Args...)>::template destroy_fptr<T>);
  type_ptr = reinterpret_cast<type_ptr_t>(&Function<Ret(Args...)>::template type_fptr<T>);

  if constexpr (requires(T){ sizeof(T); }) {
    if constexpr (sizeof(T) > BUFFER_SIZE) {
      fptr = new std::remove_reference_t<F>(std::forward<F>(func));
    } else {
      new(buffer) std::remove_reference_t<F>(std::forward<F>(func));
      fptr = buffer;
    }
  } else {
    fptr = reinterpret_cast<void*>(func);
  }

  return *this;
}

template<typename Ret, typename... Args>
MoveOnlyFunction<Ret(Args...)>::MoveOnlyFunction(MoveOnlyFunction&& other) noexcept
        : invoke_ptr(other.invoke_ptr), move_ptr(other.move_ptr), destroy_ptr(other.destroy_ptr),
          type_ptr(other.type_ptr) {
  if (move_ptr) {
    fptr = move_ptr(other.fptr, buffer);
    other.fptr = nullptr;
  }

  other.invoke_ptr = nullptr;
  other.move_ptr = nullptr;
  other.destroy_ptr = nullptr;
  other.type_ptr = nullptr;
}

template<typename Ret, typename... Args>
MoveOnlyFunction<Ret(Args...)>&
MoveOnlyFunction<Ret(Args...)>::operator=(MoveOnlyFunction&& other) noexcept {
  if (destroy_ptr) {
    destroy_ptr(fptr);
  }

  invoke_ptr = other.invoke_ptr;
  move_ptr = other.move_ptr;
  destroy_ptr = other.destroy_ptr;
  type_ptr = other.type_ptr;

  if (move_ptr) {
    fptr = move_ptr(other.fptr, buffer);
    other.fptr = nullptr;
  }

  other.invoke_ptr = nullptr;
  other.move_ptr = nullptr;
  other.destroy_ptr = nullptr;
  other.type_ptr = nullptr;

  return *this;
}

template<typename Ret, typename... Args>
MoveOnlyFunction<Ret(Args...)>::operator bool() const {
  return fptr;
}

template<typename Ret, typename... Args>
MoveOnlyFunction<Ret(Args...)>::operator void*() const {
  return fptr;
}

template<typename Ret, typename... Args>
Ret MoveOnlyFunction<Ret(Args...)>::operator()(Args... args) const {
  if (!fptr) {
    throw std::bad_function_call();
  }
  return invoke_ptr(fptr, std::forward<Args>(args)...);
}

template<typename Ret, typename... Args>
MoveOnlyFunction<Ret(Args...)>::~MoveOnlyFunction() {
  if (destroy_ptr) {
    destroy_ptr(fptr);
  }
}

template<typename Ret, typename... Args>
template<typename F>
F* MoveOnlyFunction<Ret(Args...)>::target() {
  return reinterpret_cast<F*>(fptr);
}

template<typename Ret, typename... Args>
const std::type_info& MoveOnlyFunction<Ret(Args...)>::target_type() {
  return type_ptr(fptr);
}

template<typename Ret, typename... Args>
MoveOnlyFunction(Ret(*)(Args...)) -> MoveOnlyFunction<Ret(Args...)>;

template<typename F, typename Signature = typename guide_helper<decltype(&F::operator())>::type>
MoveOnlyFunction(F) -> MoveOnlyFunction<Signature>;