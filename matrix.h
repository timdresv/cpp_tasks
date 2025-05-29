#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#define CPP23 1

class BigInteger {
  enum class Sign {
    NON_NEGATIVE = 1,
    NEGATIVE = -1,
  };

  std::vector<int64_t> number;
  Sign sign = Sign::NON_NEGATIVE;
  static const int32_t kDigits = 9;
  static const int64_t kBase = 1'000'000'000;

  void add(const BigInteger& other);

  void subtract(const BigInteger& other);

  static BigInteger multiply(BigInteger num, int64_t factor);

  BigInteger shiftBit(size_t count) const;

  size_t length() const;

  BigInteger shift(size_t count) const;

  bool isZero() const;

  bool isMinusOne() const;

  static Sign invertSign(Sign sign);

 public:
  BigInteger();

  BigInteger(int32_t num);

  explicit BigInteger(std::string str);

  std::string toString() const;

  explicit operator bool() const;

  bool operator==(const BigInteger& other) const;

  std::strong_ordering operator<=>(const BigInteger& other) const;

  BigInteger& operator++();

  BigInteger& operator--();

  BigInteger operator++(int32_t);

  BigInteger operator--(int32_t);

  BigInteger& operator+=(const BigInteger& other);

  BigInteger& operator-=(const BigInteger& other);

  BigInteger& operator*=(const BigInteger& other);

  BigInteger& operator/=(const BigInteger& divisor);

  BigInteger& operator%=(const BigInteger& divisor);

  void changeSign();
};

BigInteger::Sign BigInteger::invertSign(BigInteger::Sign sign) {
  return sign == Sign::NON_NEGATIVE ? Sign::NEGATIVE : Sign::NON_NEGATIVE;
}

void BigInteger::changeSign() {
  if (isZero()) {
    return;
  }
  sign = invertSign(sign);
}

BigInteger::BigInteger() : sign(Sign::NON_NEGATIVE) {
  number.push_back(0);
}

BigInteger::BigInteger(int32_t num) : sign(Sign::NON_NEGATIVE) {
  if (num == 0) {
    number = {0};
    return;
  }
  if (num < 0) {
    num = -num;
    sign = Sign::NEGATIVE;
  }

  while (num > 0) {
    number.push_back(num % kBase);
    num /= kBase;
  }
}

BigInteger::BigInteger(std::string str) : sign(Sign::NON_NEGATIVE) {
  if (str[0] == '-') {
    str.erase(str.begin());
    sign = Sign::NEGATIVE;
  }

  int32_t rank = 1;
  int32_t digit = 0;
  for (size_t i = str.size(); i-- > 0;) {
    if (rank == kBase) {
      number.push_back(digit);
      digit = 0;
      rank = 1;
    }

    digit += rank * (str[i] - '0');
    rank *= 10;
  }
  number.push_back(digit);
}

std::string BigInteger::toString() const {
  std::string str;
  if (sign == Sign::NEGATIVE) {
    str = "-";
  }
  std::string digit;
  for (size_t i = number.size(); i-- > 0;) {
    digit = std::to_string(number[i]);
    if (i + 1 != number.size()) {
      digit.insert(0, std::string(kDigits - digit.size(), '0'));
    }
    str += digit;
  }
  return str;
}

BigInteger::operator bool() const {
  return !(number.empty() || (number.size() == 1 && number[0] == 0));
}

bool BigInteger::operator==(const BigInteger& other) const {
  return sign == other.sign && number == other.number;
}

std::strong_ordering BigInteger::operator<=>(const BigInteger& other) const {
  if (sign != other.sign) {
    return sign < other.sign ? std::strong_ordering::less
                             : std::strong_ordering::greater;
  }

  if (number.size() != other.number.size()) {
    return (number.size() < other.number.size()) == (sign == Sign::NON_NEGATIVE)
           ? std::strong_ordering::less : std::strong_ordering::greater;
  }

  for (size_t i = number.size(); i-- > 0;) {
    if (number[i] != other.number[i]) {
      return (number[i] < other.number[i]) == (sign == Sign::NON_NEGATIVE)
             ? std::strong_ordering::less : std::strong_ordering::greater;
    }
  }
  return std::strong_ordering::equal;
}

BigInteger& BigInteger::operator++() {
  if (isZero()) {
    *this = 1;
    return *this;
  }

  if (sign == Sign::NON_NEGATIVE) {
    for (int64_t& digit : number) {
      ++digit;
      if (digit == kBase) {
        digit = 0;
        continue;
      }
      return *this;
    }
    number.push_back(1);
    return *this;
  }

  if (isMinusOne()) {
    *this = 0;
    return *this;
  }

  for (size_t i = 0; i < number.size(); ++i) {
    if (number[i] == 0) {
      number[i] = kBase - 1;
      continue;
    }
    --number[i];
    if (number[i] == 0 && i + 1 == number.size()) {
      number.pop_back();
    }
    return *this;
  }
  return *this;
}

BigInteger& BigInteger::operator--() {
  if (isZero()) {
    *this = -1;
    return *this;
  }
  sign = invertSign(sign);
  ++*this;
  if (isZero()) {
    return *this;
  }
  sign = invertSign(sign);
  return *this;
}

BigInteger BigInteger::operator++(int32_t) {
  BigInteger result = *this;
  ++*this;
  return result;
}

BigInteger BigInteger::operator--(int32_t) {
  BigInteger result = *this;
  --*this;
  return result;
}

void BigInteger::add(const BigInteger& other) {
  int64_t rest = 0;
  size_t old_size = number.size();
  size_t max_size = std::max(number.size(), other.number.size());
  number.resize(max_size + 1);
  for (size_t i = 0; i < max_size + 1; ++i) {
    if (i < old_size) {
      rest += number[i];
    }

    if (i < other.number.size()) {
      rest += other.number[i];
    }

    number[i] = rest % kBase;
    rest /= kBase;
  }
  for (size_t i = number.size(); i-- > 0;) {
    if (number[i] != 0) {
      number.resize(i + 1);
      return;
    }
  }
  *this = 0;
}

void BigInteger::subtract(const BigInteger& other) {
  Sign res_sign = sign;

  Sign diff_sign;
  sign = other.sign;
  if ((*this < other) == (sign == Sign::NON_NEGATIVE)) {
    diff_sign = Sign::NEGATIVE;
    res_sign =
        (res_sign == Sign::NEGATIVE ? Sign::NON_NEGATIVE : Sign::NEGATIVE);
  } else {
    diff_sign = Sign::NON_NEGATIVE;
  }

  sign = res_sign;
  int64_t rest = 0;
  size_t old_size = number.size();
  size_t max_size = std::max(number.size(), other.number.size());
  number.resize(max_size + 1);
  for (size_t i = 0; i < max_size + 1; ++i) {
    if (i < old_size) {
      rest += number[i] * (diff_sign == Sign::NON_NEGATIVE ? 1 : -1);
    }

    if (i < other.number.size()) {
      rest -= other.number[i] * (diff_sign == Sign::NON_NEGATIVE ? 1 : -1);
    }

    if (rest < 0) {
      number[i] = kBase + rest;
      rest = -1;
    } else {
      number[i] = rest % kBase;
      rest /= kBase;
    }
  }

  for (size_t i = number.size(); i-- > 0;) {
    if (number[i] != 0) {
      number.resize(i + 1);
      return;
    }
  }
  *this = 0;
}

BigInteger& BigInteger::operator+=(const BigInteger& other) {
  if (isZero()) {
    *this = other;
    return *this;
  }
  if (other == 0) {
    return *this;
  }

  if (sign == other.sign) {
    add(other);
  } else {
    subtract(other);
  }
  return *this;
}

BigInteger& BigInteger::operator-=(const BigInteger& other) {
  sign = invertSign(sign);
  *this += other;
  if (isZero()) {
    return *this;
  }
  sign = invertSign(sign);
  return *this;
}

BigInteger BigInteger::multiply(BigInteger num, int64_t factor) {
  if (factor == 0) {
    return 0;
  }

  int64_t rest = 0;
  for (int64_t& bit : num.number) {
    rest += bit * factor;
    bit = rest % kBase;
    rest /= kBase;
  }

  if (rest != 0) {
    num.number.push_back(rest);
  }
  return num;
}

BigInteger BigInteger::shiftBit(size_t count) const {
  BigInteger result = *this;
  std::vector<int64_t> zeros(count);
  result.number.insert(result.number.begin(), zeros.begin(), zeros.end());
  return result;
}

BigInteger& BigInteger::operator*=(const BigInteger& other) {
  if (isZero() || other == 0) {
    *this = 0;
    return *this;
  }

  sign = (sign == other.sign ? Sign::NON_NEGATIVE : Sign::NEGATIVE);
  BigInteger result = 0;
  for (size_t i = 0; i < other.number.size(); ++i) {
    result += multiply(*this, other.number[i]).shiftBit(i);
  }
  *this = result;
  return *this;
}

BigInteger& BigInteger::operator/=(const BigInteger& divisor) {
  Sign res_sign = (sign == divisor.sign ? Sign::NON_NEGATIVE : Sign::NEGATIVE);
  sign = Sign::NON_NEGATIVE;
  BigInteger div = divisor;
  div.sign = Sign::NON_NEGATIVE;

  if (*this < div) {
    *this = 0;
    return *this;
  }
  if (*this == div) {
    *this = 1;
    sign = res_sign;
    return *this;
  }

  size_t len;
  size_t mod_len = div.length();
  size_t zero_count;
  BigInteger shift_mod;
  BigInteger tmp_mod;
  BigInteger factor;
  BigInteger result;
  while (div <= *this) {
    len = length();
    shift_mod = div.shift(len - mod_len);
    zero_count = len - mod_len;
    if (*this < shift_mod) {
      shift_mod = div.shift(len - mod_len - 1);
      zero_count = len - mod_len - 1;
    }

    factor = 1;
    tmp_mod = shift_mod;
    while (tmp_mod < *this) {
      tmp_mod += shift_mod;
      ++factor;
    }

    if (tmp_mod == *this) {
      result += factor.shift(zero_count);
      *this = result;
      sign = res_sign;
      return *this;
    }

    tmp_mod -= shift_mod;
    --factor;
    result += factor.shift(zero_count);
    *this -= tmp_mod;
  }
  *this = result;
  sign = res_sign;
  return *this;
}

BigInteger& BigInteger::operator%=(const BigInteger& divisor) {
  BigInteger dividend = *this;
  dividend /= divisor;
  dividend *= divisor;
  *this -= dividend;
  return *this;
}

size_t BigInteger::length() const {
  return (number.size() - 1) * kDigits +
      std::to_string(number.back()).size();
}

BigInteger BigInteger::shift(size_t count) const {
  BigInteger result = *this;
  result *= BigInteger("1" + std::string(count % kDigits, '0'));
  std::vector<int64_t> zeros(count / kDigits);
  result.number.insert(result.number.begin(), zeros.begin(), zeros.end());
  return result;
}

bool BigInteger::isZero() const {
  return number.size() == 1 && number[0] == 0;
}

bool BigInteger::isMinusOne() const {
  return number.size() == 1 && number[0] == 1 && sign == Sign::NEGATIVE;
}

BigInteger operator+(const BigInteger& num1, const BigInteger& num2) {
  BigInteger result(num1);
  result += num2;
  return result;
}

BigInteger operator-(const BigInteger& num1, const BigInteger& num2) {
  BigInteger result(num1);
  result -= num2;
  return result;
}

BigInteger operator*(const BigInteger& num1, const BigInteger& num2) {
  BigInteger result(num1);
  result *= num2;
  return result;
}

BigInteger operator/(const BigInteger& num1, const BigInteger& num2) {
  BigInteger result(num1);
  result /= num2;
  return result;
}

BigInteger operator%(const BigInteger& num1, const BigInteger& num2) {
  BigInteger result(num1);
  result %= num2;
  return result;
}

BigInteger operator-(const BigInteger& num) {
  BigInteger result = num;
  if (result != 0) {
    result.changeSign();
  }
  return result;
}

BigInteger operator "" _bi(unsigned long long num) {
  BigInteger result(std::to_string(num));
  return result;
}

std::ostream& operator<<(std::ostream& os, const BigInteger& num) {
  return os << num.toString();
}

std::istream& operator>>(std::istream& is, BigInteger& num) {
  std::string str;
  is >> str;
  num = BigInteger(str);
  return is;
}

class Rational {
  BigInteger numerator;

  BigInteger denominator;

  static BigInteger gcd(BigInteger first, BigInteger second);

  void reduce();

 public:
  Rational();

  Rational(BigInteger num, BigInteger denom);

  Rational(BigInteger num);

  Rational(int32_t num, int32_t denom);

  Rational(int32_t num);

  std::string toString() const;

  std::string asDecimal(int32_t precision) const;

  explicit operator double() const;

  bool operator==(const Rational& other) const;

  std::strong_ordering operator<=>(const Rational& other) const;

  Rational& operator+=(const Rational& other);

  Rational& operator-=(const Rational& other);

  Rational& operator*=(const Rational& other);

  Rational& operator/=(const Rational& other);

  Rational operator-() const;
};

void Rational::reduce() {
  if (denominator < 0) {
    numerator.changeSign();
    denominator.changeSign();
  }
  BigInteger gcd_num = gcd(numerator, denominator);
  numerator /= gcd_num;
  denominator /= gcd_num;
}

Rational::Rational(BigInteger num, BigInteger denom)
    : numerator(num), denominator(denom) {
  reduce();
}

Rational::Rational() : Rational(0, 1) {}

Rational::Rational(BigInteger num) : numerator(num), denominator(1) {
  reduce();
}

Rational::Rational(int32_t num, int32_t denom)
    : numerator(num), denominator(denom) {
  reduce();
}

Rational::Rational(int32_t num) : numerator(num), denominator(1) {
  reduce();
}

std::string Rational::toString() const {
  std::string result = numerator.toString();
  if (denominator != 1) {
    result += "/" + denominator.toString();
  }
  return result;
}

std::string Rational::asDecimal(int32_t precision = 0) const {
  BigInteger
      shift_num = numerator * BigInteger("1" + std::string(precision, '0'));
  if (shift_num < 0) {
    shift_num.changeSign();
  }

  BigInteger fraction = shift_num / denominator;
  std::string result = fraction.toString();
  if (static_cast<int32_t>(result.size()) <= precision) {
    result =
        std::string(static_cast<size_t>(precision) + 1 - result.size(), '0') +
            result;
  }

  if (precision != 0) {
    result.insert(result.end() - precision, '.');
  }
  if (numerator < 0 && fraction != 0) {
    result.insert(0, 1, '-');
  }
  return result;
}

Rational::operator double() const {
  return std::stod(asDecimal(15));
}

bool Rational::operator==(const Rational& other) const {
  return numerator == other.numerator && denominator == other.denominator;
}

std::strong_ordering Rational::operator<=>(const Rational& other) const {
  return numerator * other.denominator <=> other.numerator * denominator;
}

Rational& Rational::operator+=(const Rational& other) {
  BigInteger gcd_num = gcd(denominator, other.denominator);
  BigInteger left_factor = other.denominator / gcd_num;
  BigInteger right_factor = denominator / gcd_num;
  numerator = numerator * left_factor + other.numerator * right_factor;
  denominator = denominator * left_factor;
  reduce();
  return *this;
}

Rational& Rational::operator-=(const Rational& other) {
  numerator.changeSign();
  *this += other;
  numerator.changeSign();
  reduce();
  return *this;
}

Rational& Rational::operator*=(const Rational& other) {
  numerator *= other.numerator;
  denominator *= other.denominator;
  reduce();
  return *this;
}

Rational& Rational::operator/=(const Rational& other) {
  numerator *= other.denominator;
  denominator *= other.numerator;
  reduce();
  return *this;
}

Rational Rational::operator-() const {
  Rational result(*this);
  result.numerator.changeSign();
  return result;
}

Rational operator+(const Rational& num1, const Rational& num2) {
  Rational result(num1);
  result += num2;
  return result;
}

Rational operator-(const Rational& num1, const Rational& num2) {
  Rational result(num1);
  result -= num2;
  return result;
}

Rational operator*(const Rational& num1, const Rational& num2) {
  Rational result(num1);
  result *= num2;
  return result;
}

Rational operator/(const Rational& num1, const Rational& num2) {
  Rational result(num1);
  result /= num2;
  return result;
}

std::istream& operator>>(std::istream& is, Rational& num) {
  int32_t int_num;
  is >> int_num;
  num = int_num;
  return is;
}

BigInteger Rational::gcd(BigInteger first, BigInteger second) {
  if (first < 0) {
    first.changeSign();
  }
  if (second < 0) {
    second.changeSign();
  }
  BigInteger tmp;
  while (second != 0) {
    first %= second;
    tmp = second;
    second = first;
    first = tmp;
  }
  return first;
}

constexpr int isPrime(int number) {
  for (int i = 2; i * i <= number; ++i) {
    if (number % i == 0) {
      return false;
    }
  }
  return true;
}

template<size_t N>
class Residue {
  size_t num_;

  static Residue<N> pow(const Residue<N>& number, int exp) {
    Residue<N> result(1);
    for (int i = 0; i < exp; ++i) {
      result *= number;
    }
    return result;
  }

 public:
  Residue() = default;

  Residue(int num);

  explicit operator int() const;

  bool operator==(const Residue<N>& other) const;

  Residue<N>& operator+=(const Residue<N>& other);

  Residue<N>& operator-=(const Residue<N>& other);

  Residue<N>& operator*=(const Residue<N>& other);

  Residue<N>& operator/=(const Residue<N>& other);
};

template<size_t N>
Residue<N>::Residue(int num) {
  int res = num % static_cast<int>(N);
  num_ = res >= 0 ? res : N + res;
}

template<size_t N>
Residue<N>::operator int() const {
  return num_;
}

template<size_t N>
bool Residue<N>::operator==(const Residue<N>& other) const {
  return num_ == other.num_;
}

template<size_t N>
Residue<N>& Residue<N>::operator+=(const Residue<N>& other) {
  num_ = (num_ + other.num_) % N;
  return *this;
}

template<size_t N>
Residue<N>& Residue<N>::operator-=(const Residue<N>& other) {
  num_ = (N + num_ - other.num_) % N;
  return *this;
}

template<size_t N>
Residue<N>& Residue<N>::operator*=(const Residue<N>& other) {
  num_ = (num_ * other.num_) % N;
  return *this;
}

template<size_t N>
Residue<N>& Residue<N>::operator/=(const Residue<N>& other) {
  static_assert(isPrime(N));
  Residue<N> opposite = pow(other, N - 2);
  num_ = (num_ * opposite.num_) % N;
  return *this;
}

template<size_t N>
Residue<N> operator+(const Residue<N>& num1, const Residue<N>& num2) {
  Residue<N> tmp(num1);
  tmp += num2;
  return tmp;
}

template<size_t N>
Residue<N> operator-(const Residue<N>& num1, const Residue<N>& num2) {
  Residue<N> tmp(num1);
  tmp -= num2;
  return tmp;
}

template<size_t N>
Residue<N> operator*(const Residue<N>& num1, const Residue<N>& num2) {
  Residue<N> tmp(num1);
  tmp *= num2;
  return tmp;
}

template<size_t N>
Residue<N> operator/(const Residue<N>& num1, const Residue<N>& num2) {
  Residue<N> tmp(num1);
  tmp /= num2;
  return tmp;
}

template<size_t N>
std::ostream& operator<<(std::ostream& os, const Residue<N>& num) {
  os << static_cast<int>(num);
  return os;
}
std::ostream& operator<<(std::ostream& os, const Rational& num) {
  os << static_cast<double>(num);
  return os;
}

template<size_t N, size_t M, typename Field=Rational>
class Matrix {
  std::vector<std::vector<Field>> matrix_{N, std::vector<Field>(M)};

  static Matrix<N, N, Field> unityMatrix();

 public:
  Matrix() = default;

  Matrix(std::initializer_list<std::vector<Field>> args);

  bool operator==(const Matrix& other) const = default;

  Matrix& operator+=(const Matrix& other);

  Matrix& operator-=(const Matrix& other);

  Matrix& operator*=(const Field& factor);

  Matrix& operator*=(const Matrix<M, M, Field> other);

  Field det() const;

  Matrix<M, N, Field> transposed() const;

  size_t rank() const;

  void invert();

  Matrix<N, N, Field> inverted() const;

  Field trace() const;

  std::array<Field, M> getRow(size_t index) const;

  std::array<Field, N> getColumn(size_t index) const;

  Field& operator[](size_t index1, size_t index2);

  const Field& operator[](size_t index1, size_t index2) const;
};

template<size_t N, size_t M, typename Field>
Matrix<N, N, Field> Matrix<N, M, Field>::unityMatrix() {
  static_assert(N == M);
  Matrix<N, N, Field> result;
  for (size_t i = 0; i < N; ++i) {
    result.matrix_[i][i] = 1;
  }
  return result;
}

template<size_t N, size_t M, typename Field>
Matrix<N, M, Field>::Matrix(std::initializer_list<std::vector<Field>> args)
    : matrix_(args) {
}

template<size_t N, size_t M, typename Field>
Field& Matrix<N, M, Field>::operator[](size_t index1, size_t index2) {
  return matrix_[index1][index2];
}

template<size_t N, size_t M, typename Field>
const Field& Matrix<N, M, Field>::operator[](size_t index1,
                                             size_t index2) const {
  return matrix_[index1][index2];
}

template<size_t N, size_t M, typename Field>
Matrix<N, M, Field>& Matrix<N, M, Field>::operator+=(const Matrix<N, M,
                                                                  Field>& other) {
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      matrix_[i][j] += other.matrix_[i][j];
    }
  }
  return *this;
}

template<size_t N, size_t M, typename Field>
Matrix<N, M, Field>& Matrix<N, M, Field>::operator-=(const Matrix<N, M,
                                                                  Field>& other) {
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      matrix_[i][j] -= other.matrix_[i][j];
    }
  }
  return *this;
}

template<size_t N, size_t M, typename Field>
Matrix<N, M, Field>& Matrix<N, M, Field>::operator*=(const Field& factor) {
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      matrix_[i][j] *= factor;
    }
  }
  return *this;
}

template<size_t N, size_t M, typename Field>
Matrix<N, M, Field>& Matrix<N, M, Field>::operator*=(const Matrix<M, M,
                                                                  Field> other) {
  Matrix<N, M, Field> result;
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      for (size_t k = 0; k < M; ++k) {
        result[i, j] += matrix_[i][k] * other[k, j];
      }
    }
  }
  *this = result;
  return *this;
}

template<size_t N, size_t M, typename Field>
Field Matrix<N, M, Field>::det() const {
  static_assert(N == M);
  std::vector<std::vector<Field>> matrix = matrix_;
  Field result = 1;
  for (size_t i = 0; i < N; ++i) {
    size_t non_zero = i;
    for (size_t j = i; j < N; ++j) {
      if (matrix[j][i] != 0) {
        non_zero = j;
        break;
      }
    }

    if (matrix[non_zero][i] == 0) {
      return 0;
    }
    if (i != non_zero) {
      std::swap(matrix[i], matrix[non_zero]);
      result *= -1;
    }
    result *= matrix[i][i];

    for (size_t j = i + 1; j < N; ++j) {
      matrix[i][j] /= matrix[i][i];
    }
    for (size_t j = i + 1; j < N; ++j) {
      for (size_t k = i + 1; k < N; ++k) {
        matrix[j][k] -= matrix[j][i] * matrix[i][k];
      }
    }
  }
  return result;
}

template<size_t N, size_t M, typename Field>
Matrix<M, N, Field> Matrix<N, M, Field>::transposed() const {
  Matrix<M, N, Field> result;
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      result[j, i] = matrix_[i][j];
    }
  }
  return result;
}

template<size_t N, size_t M, typename Field>
size_t Matrix<N, M, Field>::rank() const {
  if (N > M) {
    return transposed().rank();
  }
  std::vector<std::vector<Field>> matrix = matrix_;
  size_t result = std::max(N, M);
  std::vector<bool> not_used(N, true);
  for (size_t i = 0; i < M; ++i) {
    size_t non_zero = N;
    for (size_t j = 0; j < N; ++j) {
      if (not_used[j] && matrix[j][i] != 0) {
        non_zero = j;
        break;
      }
    }

    if (non_zero != N) {
      not_used[non_zero] = false;
      for (size_t j = i + 1; j < M; ++j) {
        matrix[non_zero][j] /= matrix[non_zero][i];
      }
      for (size_t j = 0; j < N; ++j) {
        if (j != non_zero) {
          for (size_t k = i + 1; k < M; ++k) {
            matrix[j][k] -= matrix[non_zero][k] * matrix[j][i];
          }
        }
      }
    } else {
      --result;
    }
  }
  return result;
}

template<size_t N, size_t M, typename Field>
void Matrix<N, M, Field>::invert() {
  static_assert(N == M);
  Matrix<N, N, Field> result = *this;
  *this = Matrix<N, N, Field>::unityMatrix();
  for (size_t i = 0; i < N; ++i) {
    size_t non_zero = i;
    for (size_t j = i; j < N; ++j) {
      if (result.matrix_[j][i] != 0) {
        non_zero = j;
        break;
      }
    }
    
    if (i != non_zero) {
      std::swap(matrix_[i], matrix_[non_zero]);
      std::swap(result.matrix_[i], result.matrix_[non_zero]);
    }

    Field factor = result.matrix_[i][i];
    for (size_t j = 0; j < N; ++j) {
      matrix_[i][j] /= factor;
      result.matrix_[i][j] /= factor;
    }
    for (size_t j = 0; j < N; ++j) {
      if (j != i) {
        factor = result.matrix_[j][i];
        for (size_t k = 0; k < N; ++k) {
          matrix_[j][k] -= matrix_[i][k] * factor;
          result.matrix_[j][k] -= result.matrix_[i][k] * factor;
        }
      }
    }
  }
}

template<size_t N, size_t M, typename Field>
Matrix<N, N, Field> Matrix<N, M, Field>::inverted() const {
  Matrix<N, N, Field> result(*this);
  result.invert();
  return result;
}

template<size_t N, size_t M, typename Field>
Field Matrix<N, M, Field>::trace() const {
  static_assert(N == M);
  Field result = 0;
  for (size_t i = 0; i < N; ++i) {
    result += matrix_[i][i];
  }
  return result;
}

template<size_t N, size_t M, typename Field>
std::array<Field, M> Matrix<N, M, Field>::getRow(size_t index) const {
  std::array<Field, M> row;
  std::copy(matrix_[index].begin(), matrix_[index].begin() + M, row.begin());
  return row;
}

template<size_t N, size_t M, typename Field>
std::array<Field, N> Matrix<N, M, Field>::getColumn(size_t index) const {
  std::array<Field, N> column;
  for (size_t i = 0; i < N; ++i) {
    column[i] = matrix_[i][index];
  }
  return column;
}

template<size_t N, size_t M, typename Field>
Matrix<N, M, Field> operator+(const Matrix<N, M, Field>& matrix1,
                              const Matrix<N, M, Field>& matrix2) {
  Matrix<N, M, Field> result = matrix1;
  result += matrix2;
  return result;
}

template<size_t N, size_t M, typename Field>
Matrix<N, M, Field> operator-(const Matrix<N, M, Field>& matrix1,
                              const Matrix<N, M, Field>& matrix2) {
  Matrix<N, M, Field> result = matrix1;
  result -= matrix2;
  return result;
}

template<size_t N, size_t M, typename Field>
Matrix<N, M, Field> operator*(const Matrix<N, M, Field>& matrix,
                              const Field& factor) {
  Matrix<N, M, Field> result = matrix;
  result *= factor;
  return result;
}

template<size_t N, size_t M, typename Field>
Matrix<N, M, Field> operator*(const Field& factor,
                              const Matrix<N, M, Field>& matrix) {
  Matrix<N, M, Field> result = matrix;
  result *= factor;
  return result;
}

template<size_t N, size_t M, size_t K, typename Field>
Matrix<N, K, Field> operator*(const Matrix<N, M, Field>& matrix1,
                              const Matrix<M, K, Field>& matrix2) {
  Matrix<N, K, Field> result;
  for (size_t i = 0; i < N; ++i) {
    for (size_t k = 0; k < K; ++k) {
      for (size_t j = 0; j < M; ++j) {
        result[i, k] += matrix1[i, j] * matrix2[j, k];
      }
    }
  }
  return result;
}

template<size_t N, typename Field=Rational>
using SquareMatrix = Matrix<N, N, Field>;