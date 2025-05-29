#include <algorithm>
#include <cstring>
#include <iostream>

class String {
 private:
  char* data_;
  size_t size_;
  size_t capacity_;

  void set_capacity(size_t new_capacity);

  explicit String(size_t size);

 public:
  void swap(String& other);

  void reserve(size_t new_capacity);

  String(const char* arr);

  String(size_t size, char ch);

  String(char ch);

  String();

  String(const String& other);

  String& operator=(const String& str);

  char& operator[](size_t index);
  const char& operator[](size_t index) const;

  size_t length() const;
  size_t size() const;

  size_t capacity() const;

  void push_back(char ch);

  void pop_back();

  char& front();
  const char& front() const;

  char& back();
  const char& back() const;

  String& operator+=(char ch);
  String& operator+=(const String& other);

  size_t find_helper(const String& substr, bool is_rfind) const;
  size_t find(const String& substr) const;
  size_t rfind(const String& substr) const;

  String substr(size_t start, size_t count) const;

  bool empty() const;

  void clear();

  void shrink_to_fit();

  char* data();
  const char* data() const;

  ~String();
};

void String::swap(String& other) {
  std::swap(data_, other.data_);
  std::swap(size_, other.size_);
  std::swap(capacity_, other.capacity_);
}

void String::set_capacity(size_t new_capacity) {
  capacity_ = new_capacity;
  char* arr = new char[capacity_ + 1];
  std::copy(data_, data_ + size_ + 1, arr);
  delete[] data_;
  data_ = arr;
}

void String::reserve(size_t new_capacity) {
  if (new_capacity > capacity_) {
    set_capacity(new_capacity);
  }
}

String::String(size_t size) : data_(new char[size + 1]), size_(size),
                              capacity_(size) {
  data_[size_] = '\0';
}

String::String(const char* arr) : String(std::strlen(arr)) {
  std::copy(arr, arr + capacity_ + 1, data_);
}

String::String(size_t size, char ch) : String(size) {
  std::fill(data_, data_ + size, ch);
}

String::String(char ch) : String(1, ch) {}

String::String() : String(static_cast<size_t>(0)) {}

String::String(const String& other) : String(other.size_) {
  std::copy(other.data_, other.data_ + capacity_ + 1, data_);
}

String& String::operator=(const String& str) {
  if (this == &str) {
    return *this;
  }
  String tmp = str;
  swap(tmp);
  return *this;
}

char& String::operator[](size_t index) {
  return data_[index];
}

const char& String::operator[](size_t index) const {
  return data_[index];
}

size_t String::length() const {
  return size_;
}

size_t String::size() const {
  return size_;
}

size_t String::capacity() const {
  return capacity_;
}

void String::push_back(char ch) {
  if (size_ == capacity_) {
    reserve(std::max(capacity_ * 2, static_cast<size_t>(1)));
  }
  data_[size_] = ch;
  data_[++size_] = '\0';
}

void String::pop_back() {
  data_[--size_] = '\0';
}

char& String::front() {
  return data_[0];
}

const char& String::front() const {
  return data_[0];
}

char& String::back() {
  return data_[size_ - 1];
}

const char& String::back() const {
  return data_[size_ - 1];
}

String& String::operator+=(char ch) {
  push_back(ch);
  return *this;
}

String& String::operator+=(const String& other) {
  if (size_ + other.size_ >= capacity_) {
    reserve((size_ + other.size_) * 2);
  }

  std::copy(other.data_, other.data_ + other.size_, data_ + size_);
  size_ += other.size_;
  data_[size_] = '\0';
  return *this;
}

size_t String::find_helper(const String& substr, bool is_rfind) const {
  if (size_ < substr.size_) {
    return size_;
  }
  for (size_t i = 0; i < size_ - substr.size_; ++i) {
    size_t pos = is_rfind ? size_ - substr.size_ - i : i;
    if (memcmp(data_ + pos, substr.data_, substr.size_) == 0) {
      return pos;
    }
  }
  return size_;
}

size_t String::find(const String& substr) const {
  return find_helper(substr, false);
}

size_t String::rfind(const String& substr) const {
  return find_helper(substr, true);
}

String String::substr(size_t start, size_t count) const {
  String substr(std::min(count, size_ - start));
  std::copy(data_ + start, data_ + start + substr.size_,
            substr.data_);
  substr.data_[substr.size_] = '\0';
  return substr;
}

bool String::empty() const {
  return size_ == 0;
}

void String::clear() {
  data_[0] = '\0';
  size_ = 0;
}

void String::shrink_to_fit() {
  if (size_ < capacity_) {
    set_capacity(size_);
  }
}

char* String::data() {
  return data_;
}

const char* String::data() const {
  return data_;
}

String::~String() {
  delete[] data_;
}

bool operator<(const String& str1, const String& str2) {
  int result = std::memcmp(str1.data(), str2.data(),
                           std::min(str1.size(), str2.size()) + 1);
  return result < 0;
}

bool operator>(const String& str1, const String& str2) {
  return str2 < str1;
}

bool operator<=(const String& str1, const String& str2) {
  return !(str1 > str2);
}

bool operator>=(const String& str1, const String& str2) {
  return !(str1 < str2);
}

bool operator==(const String& str1, const String& str2) {
  return str1.size() == str2.size() &&
      std::memcmp(str1.data(), str2.data(), str1.size()) == 0;
}

bool operator!=(const String& str1, const String& str2) {
  return !(str1 == str2);
}

String operator+(const String& str1, const String& str2) {
  String tmp(str1);
  tmp += str2;
  return tmp;
}

std::ostream& operator<<(std::ostream& os, const String& str) {
  for (size_t i = 0; i < str.size(); ++i) {
    os << str[i];
  }
  return os;
}

std::istream& operator>>(std::istream& is, String& str) {
  char symbol = is.get();
  while (std::isspace(symbol)) {
    symbol = is.get();
  }
  while (symbol != EOF && !std::isspace(symbol)) {
    str.push_back(symbol);
    symbol = is.get();
  }
  return is;
}