#include <cmath>
#include <iostream>
#include <vector>

namespace details {
  const double kEps = 0.000000000001;

  bool is_equal(double first, double second) {
    return std::fabs(first - second) < kEps;
  }

  bool is_smaller_or_equal(double first, double second) {
    return first - second < kEps;
  }

  bool is_bigger_or_equal(double first, double second) {
    return second - first < kEps;
  }
}

struct Point {
  double x = 0;
  double y = 0;

  static Point midpoint(const Point& point1, const Point& point2);

  static double distance(const Point& point1, const Point& point2);
};

Point Point::midpoint(const Point& point1, const Point& point2) {
  return {(point1.x + point2.x) / 2, (point1.y + point2.y) / 2};
}

double Point::distance(const Point& point1, const Point& point2) {
  return std::hypot(point1.x - point2.x, point1.y - point2.y);
}

bool operator==(const Point& point1, const Point& point2) {
  return details::is_equal(point1.x, point2.x) &&
      details::is_equal(point1.y, point2.y);
}

class Line {
  Point point1_;
  Point point2_;

 public:
  Line();

  Line(const Point& point1, const Point& point2);

  Line(double factor, double shift);

  Line(const Point& point, double factor);

  bool operator==(const Line& other) const;

  static Point get_intersect(const Line& line1, const Line& line2);

  Line get_perpendicular(const Point& point) const;

  Line get_bisector_perpendicular() const;
};

Line::Line() = default;

Line::Line(const Point& point1, const Point& point2)
    : point1_(point1), point2_(point2) {}

Line::Line(double factor, double shift) : point1_{0, shift},
                                          point2_{1, factor + shift} {}

Line::Line(const Point& point, double factor)
    : point1_(point), point2_{point.x + 1, point.y + factor} {}

bool Line::operator==(const Line& other) const {
  if (details::is_equal(point1_.x, point2_.x) ||
      details::is_equal(other.point1_.x, other.point2_.x)) {
    return details::is_equal(point1_.x, other.point1_.x) &&
        details::is_equal(point1_.x, point2_.x) &&
        details::is_equal(other.point1_.x, other.point2_.x);
  }

  double factor = (point1_.y - point2_.y) / (point1_.x - point2_.x);
  double other_factor =
      (other.point1_.y - other.point2_.y) / (other.point1_.x - other.point2_.x);

  return details::is_equal(factor, other_factor) &&
      details::is_equal(point1_.y - factor * point1_.x,
                        other.point1_.y - other_factor * other.point1_.x);
}

Point Line::get_intersect(const Line& line1, const Line& line2) {
  double x1 = line1.point1_.x;
  double y1 = line1.point1_.y;
  double x2 = line1.point2_.x;
  double y2 = line1.point2_.y;
  double x3 = line2.point1_.x;
  double y3 = line2.point1_.y;
  double x4 = line2.point2_.x;
  double y4 = line2.point2_.y;
  double denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
  if (details::is_equal(denominator, 0)) {
    return {INFINITY, INFINITY};
  }
  double numerator_x =
      (x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4);
  double numerator_y =
      (x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4);
  return {numerator_x / denominator, numerator_y / denominator};
}

Line Line::get_perpendicular(const Point& point) const {
  if (!details::is_equal(point1_.y, point2_.y)) {
    double factor = -(point1_.x - point2_.x) / (point1_.y - point2_.y);
    return Line{point, factor};
  } else {
    return Line{point, {point.x, point.y + 1}};
  }
}

Line Line::get_bisector_perpendicular() const {
  Point center = Point::midpoint(point1_, point2_);
  return get_perpendicular(center);
}

class Shape {
 public:
  virtual double perimeter() const = 0;

  virtual double area() const = 0;

  virtual bool isEqual(const Shape& other) const = 0;

  virtual bool isCongruentTo(const Shape& another) const = 0;

  virtual bool isSimilarTo(const Shape& another) const = 0;

  virtual bool containsPoint(const Point& point) const = 0;

  virtual void rotate(const Point& center, double angle) = 0;

  virtual void reflect(const Point& center) = 0;

  virtual void reflect(const Line& axis) = 0;

  virtual void scale(const Point& center, double coefficient) = 0;

  virtual ~Shape() = default;
};

bool operator==(const Shape& shape1, const Shape& shape2) {
  return shape1.isEqual(shape2);
}

class Polygon : public Shape {
 protected:
  std::vector<Point> vertices_;

  bool equal(const std::vector<Point>& other_vertices, size_t offset) const;

  bool similar(const std::vector<Point>& other_vertices, size_t offset,
               bool withCongruent) const;

  static double angle(const Point& p1, const Point& p2, const Point& p3);

  static bool isRayIntersectSide(Point ray_point, Point point1, Point point2);

  bool isSimilar(const Polygon* polygon, bool withCongruent) const;

 public:
  explicit Polygon(const std::vector<Point>& vertices);

  template<typename ...T>
  Polygon(T... args);

  size_t verticesCount() const;

  const std::vector<Point>& getVertices() const;

  bool isConvex() const;

  double perimeter() const override;

  double area() const override;

  bool isEqual(const Shape& other) const override;

  bool isCongruentTo(const Shape& other) const override;

  bool isSimilarTo(const Shape& other) const override;

  bool containsPoint(const Point& point) const override;

  void rotate(const Point& center, double angle) override;

  void reflect(const Point& center) override;

  void reflect(const Line& axis) override;

  void scale(const Point& center, double coefficient) override;
};

Polygon::Polygon(const std::vector<Point>& vertices) : vertices_(vertices) {}

template<typename ...T>
Polygon::Polygon(T... args) : vertices_({args...}) {}

size_t Polygon::verticesCount() const {
  return vertices_.size();
}

const std::vector<Point>& Polygon::getVertices() const {
  return vertices_;
}

bool Polygon::isConvex() const {
  Point vector1{vertices_[0].x - vertices_[vertices_.size() - 1].x,
                vertices_[0].y - vertices_[vertices_.size() - 1].y};
  Point vector2{vertices_[1].x - vertices_[vertices_.size() - 1].x,
                vertices_[1].y - vertices_[vertices_.size() - 1].y};
  bool isPositive =
      details::is_bigger_or_equal(vector1.x * vector2.y - vector1.y * vector2.x,
                                  0);

  for (size_t j = 2; j < vertices_.size() - 1; ++j) {
    vector2 = Point{vertices_[j].x - vertices_[vertices_.size() - 1].x,
                    vertices_[j].y - vertices_[vertices_.size() - 1].y};

    if (details::is_bigger_or_equal(
        vector1.x * vector2.y - vector1.y * vector2.x, 0) !=
        isPositive) {
      return false;
    }
  }

  for (size_t i = 1; i < vertices_.size(); ++i) {
    vector1 = Point{vertices_[i].x - vertices_[i - 1].x,
                    vertices_[i].y - vertices_[i - 1].y};

    for (size_t j = i + 1; j < vertices_.size(); ++j) {
      vector2 = Point{vertices_[j].x - vertices_[i - 1].x,
                      vertices_[j].y - vertices_[i - 1].y};

      if (details::is_bigger_or_equal(
          vector1.x * vector2.y - vector1.y * vector2.x, 0) !=
          isPositive) {
        return false;
      }
    }
  }
  return true;
}

double Polygon::perimeter() const {
  double result = Point::distance(vertices_.back(), vertices_[0]);
  for (size_t i = 1; i < vertices_.size(); ++i) {
    result += Point::distance(vertices_[i - 1], vertices_[i]);
  }
  return result;
}

double Polygon::area() const {
  double result = (vertices_[0].x - vertices_.back().x) *
      (vertices_[0].y + vertices_.back().y) / 2;
  for (size_t i = 1; i < vertices_.size(); ++i) {
    result += (vertices_[i].x - vertices_[i - 1].x) *
        (vertices_[i].y + vertices_[i - 1].y) / 2;
  }
  return result;
}

bool Polygon::equal(const std::vector<Point>& other_vertices,
                    size_t offset) const {
  for (size_t j = 0; j < vertices_.size(); ++j) {
    if (vertices_[j] != other_vertices[(j + offset) % vertices_.size()]) {
      return false;
    }
  }
  return true;
}

bool Polygon::isEqual(const Shape& other) const {
  const Polygon* polygon = dynamic_cast<const Polygon*>(&other);
  if (polygon) {
    if (vertices_.size() != polygon->vertices_.size()) {
      return false;
    }
    for (size_t i = 0; i < vertices_.size(); ++i) {
      std::vector<Point> reversed_vertices(polygon->vertices_.size());
      std::reverse_copy(polygon->vertices_.begin(), polygon->vertices_.end(),
                        reversed_vertices.begin());
      if (equal(polygon->vertices_, i) || equal(reversed_vertices, i)) {
        return true;
      }
    }
  }
  return false;
}

double Polygon::angle(const Point& p1, const Point& p2, const Point& p3) {
  double distance1 = Point::distance(p1, p2);
  double distance2 = Point::distance(p3, p2);
  double result = acos(
      ((p1.x - p2.x) * (p3.x - p2.x) + (p1.y - p2.y) * (p3.y - p2.y)) /
          distance1 / distance2);
  return result;
}

bool Polygon::similar(const std::vector<Point>& other_vertices,
                      size_t offset, bool withCongruent) const {
  for (size_t j = 0; j < vertices_.size(); ++j) {
    if (withCongruent) {
      double distance1 = Point::distance(vertices_[j],
                                         vertices_[(j + 1) % vertices_.size()]);
      double distance2 =
          Point::distance(other_vertices[(j + offset) % vertices_.size()],
                          other_vertices[(j + offset + 1) % vertices_.size()]);
      if (!details::is_equal(distance1, distance2)) {
        return false;
      }
    }
    double angle1 = angle(vertices_[j],
                          vertices_[(j + 1) % vertices_.size()],
                          vertices_[(j + 2) % vertices_.size()]);
    double angle2 = angle(other_vertices[(j + offset) % other_vertices.size()],
                          other_vertices[(j + offset + 1) %
                              other_vertices.size()],
                          other_vertices[(j + offset + 2) %
                              other_vertices.size()]);
    if (!details::is_equal(angle1, angle2)) {
      return false;
    }
  }
  return true;
}

bool Polygon::isSimilar(const Polygon* polygon,
                        bool withCongruent = false) const {
  if (vertices_.size() != polygon->vertices_.size()) {
    return false;
  }
  if (vertices_.size() <= 2) {
    return details::is_equal(Point::distance(vertices_[0], vertices_.back()),
                             Point::distance(polygon->vertices_[0],
                                             polygon->vertices_.back()));
  }
  for (size_t i = 0; i < vertices_.size(); ++i) {
    std::vector<Point> reversed_vertices(polygon->vertices_.size());
    std::reverse_copy(polygon->vertices_.begin(), polygon->vertices_.end(),
                      reversed_vertices.begin());
    if (similar(polygon->vertices_, i, withCongruent) ||
        similar(reversed_vertices, i, withCongruent)) {
      return true;
    }
  }
  return false;
}

bool Polygon::isCongruentTo(const Shape& other) const {
  const Polygon* polygon = dynamic_cast<const Polygon*>(&other);
  return polygon && isSimilar(polygon, true);
}

bool Polygon::isSimilarTo(const Shape& other) const {
  const Polygon* polygon = dynamic_cast<const Polygon*>(&other);
  return polygon && isSimilar(polygon);
}

bool Polygon::isRayIntersectSide(Point ray_point, Point point1, Point point2) {
  Line side = Line{point1, point2};
  Line ray = Line{ray_point, {ray_point.x + 1, ray_point.y}};
  Point intersect = Line::get_intersect(ray, side);
  return
      details::is_bigger_or_equal(ray_point.y, std::min(point1.y, point2.y)) &&
          details::is_smaller_or_equal(ray_point.y,
                                       std::max(point1.y, point2.y)) &&
          details::is_bigger_or_equal(intersect.x, ray_point.x) &&
          !details::is_equal(intersect.y, std::max(point1.y, point2.y));
}

bool Polygon::containsPoint(const Point& point) const {
  int count = 0;
  for (size_t i = 0; i < vertices_.size(); ++i) {
    Point p1 = i != 0 ? vertices_[i - 1] : vertices_.back();
    Point p2 = vertices_[i];

    if (point == p1) {
      return true;
    }
    if (details::is_equal(p1.y, p2.y)) {
      if (details::is_bigger_or_equal(point.x, std::min(p1.x, p2.x)) &&
          details::is_smaller_or_equal(point.x, std::max(p1.x, p2.x)) &&
          details::is_equal(point.y, p1.y)) {
        return true;
      }
      continue;
    }

    if (isRayIntersectSide(point, p1, p2)) {
      ++count;
    }
  }
  return count % 2 == 1;
}

void Polygon::rotate(const Point& center, double angle) {
  for (auto& point : vertices_) {
    point = {center.x + (point.x - center.x) * cos(angle) -
                 (point.y - center.y) * sin(angle),
             center.y + (point.x - center.x) * sin(angle) +
                 (point.y - center.y) * cos(angle)};
  }
}

void Polygon::reflect(const Point& center) {
  for (auto& point : vertices_) {
    point = {2 * center.x - point.x, 2 * center.y - point.y};
  }
}

void Polygon::reflect(const Line& axis) {
  Point center;
  for (auto& point : vertices_) {
    center = Line::get_intersect(axis, axis.get_perpendicular(point));
    point = {2 * center.x - point.x, 2 * center.y - point.y};
  }
}

void Polygon::scale(const Point& center, double coefficient) {
  for (auto& point : vertices_) {
    point = {center.x + coefficient * (point.x - center.x),
             center.y + coefficient * (point.y - center.y)};
  }
}

class Ellipse : public Shape {
 protected:
  Point focus1_;
  Point focus2_;
  double semi_major_axis_;
  double semi_minor_axis_;

 public:
  Ellipse(const Point& focus1, const Point& focus2, double distance);

  double eccentricity() const;

  virtual Point center() const;

  std::pair<Point, Point> focuses() const;

  std::pair<Line, Line> directrices() const;

  double semiMajorAxis() const;

  double perimeter() const override;

  double area() const override;

  bool isEqual(const Shape& other) const override;

  bool isCongruentTo(const Shape& other) const override;

  bool isSimilarTo(const Shape& other) const override;

  bool containsPoint(const Point& point) const override;

  void rotate(const Point& center, double angle) override;

  void reflect(const Point& center) override;

  void reflect(const Line& axis) override;

  void scale(const Point& center, double coefficient) override;
};

Ellipse::Ellipse(const Point& focus1, const Point& focus2, double distance)
    : focus1_(focus1), focus2_(focus2) {
  semi_major_axis_ = distance / 2;
  semi_minor_axis_ = sqrt((pow(semi_major_axis_, 2) -
      pow(Point::distance(focus1_, focus2_) / 2, 2)));
}

double Ellipse::eccentricity() const {
  return Point::distance(focus1_, focus2_) / 2 / semi_major_axis_;
}

Point Ellipse::center() const {
  return Point::midpoint(focus1_, focus2_);
}

std::pair<Point, Point> Ellipse::focuses() const {
  return {focus1_, focus2_};
}

std::pair<Line, Line> Ellipse::directrices() const {
  Point center_p(center());
  double eccentr = eccentricity();

  Point vertex1 = {(focus1_.x - center_p.x) / eccentr / eccentr,
                   (focus1_.y - center_p.y) / eccentr / eccentr};
  Point vertex2 = {(focus2_.x - center_p.x) / eccentr / eccentr,
                   (focus2_.y - center_p.y) / eccentr / eccentr};

  if (details::is_equal(focus1_.y, focus2_.y)) {
    return {Line(vertex1, {vertex1.x, vertex1.y + 1}),
            Line(vertex2, {vertex2.x, vertex2.y + 1})};
  }

  double factor = (focus1_.x - focus2_.x) / (focus1_.y - focus2_.y);
  return {Line(vertex1, factor), Line(vertex2, factor)};
}

double Ellipse::semiMajorAxis() const {
  return semi_major_axis_;
}

double Ellipse::perimeter() const {
  return M_PI * (3 * (semi_major_axis_ + semi_minor_axis_) - sqrt(
      (3 * semi_major_axis_ + semi_minor_axis_) *
          (semi_major_axis_ + 3 * semi_minor_axis_)));
}

double Ellipse::area() const {
  return M_PI * semi_major_axis_ * semi_minor_axis_;
}

bool Ellipse::isEqual(const Shape& other) const {
  const Ellipse* ellipse = dynamic_cast<const Ellipse*>(&other);
  return ellipse && focus1_ == ellipse->focus1_ &&
      focus2_ == ellipse->focus2_ &&
      details::is_equal(semi_major_axis_, ellipse->semi_major_axis_);
}

bool Ellipse::isCongruentTo(const Shape& other) const {
  const Ellipse* ellipse = dynamic_cast<const Ellipse*>(&other);
  return ellipse &&
      details::is_equal(semi_minor_axis_, ellipse->semi_minor_axis_) &&
      details::is_equal(semi_major_axis_, ellipse->semi_major_axis_);
}

bool Ellipse::isSimilarTo(const Shape& other) const {
  const Ellipse* ellipse = dynamic_cast<const Ellipse*>(&other);
  return ellipse &&
      details::is_equal(semi_minor_axis_ / ellipse->semi_minor_axis_,
                        semi_major_axis_ / ellipse->semi_major_axis_);
}

bool Ellipse::containsPoint(const Point& point) const {
  return details::is_smaller_or_equal(
      Point::distance(point, focus1_) + Point::distance(point, focus2_),
      semi_major_axis_ * 2);
}

void Ellipse::rotate(const Point& center, double angle) {
  focus1_ = {center.x + (focus1_.x - center.x) * cos(angle) -
                 (focus1_.y - center.y) * sin(angle),
             center.y + (focus1_.x - center.x) * sin(angle) +
                 (focus1_.y - center.y) * cos(angle)};
  focus2_ = {center.x + (focus2_.x - center.x) * cos(angle) -
                 (focus2_.y - center.y) * sin(angle),
             center.y + (focus2_.x - center.x) * sin(angle) +
                 (focus2_.y - center.y) * cos(angle)};
}

void Ellipse::reflect(const Point& center) {
  focus1_ = {2 * center.x - focus1_.x, 2 * center.y - focus1_.y};
  focus2_ = {2 * center.x - focus2_.x, 2 * center.y - focus2_.y};
}

void Ellipse::reflect(const Line& axis) {
  Point center = Line::get_intersect(axis, axis.get_perpendicular(focus1_));
  focus1_ = {2 * center.x - focus1_.x, 2 * center.y - focus1_.y};
  center = Line::get_intersect(axis, axis.get_perpendicular(focus2_));
  focus2_ = {2 * center.x - focus2_.x, 2 * center.y - focus2_.y};
}

void Ellipse::scale(const Point& center, double coefficient) {
  focus1_ = {center.x + coefficient * (focus1_.x - center.x),
             center.y + coefficient * (focus1_.y - center.y)};
  focus2_ = {center.x + coefficient * (focus2_.x - center.x),
             center.y + coefficient * (focus2_.y - center.y)};
  semi_major_axis_ *= coefficient;
  semi_minor_axis_ *= coefficient;
}

class Circle : public Ellipse {
 public:
  Circle(const Point& center, double radius);

  double radius() const;

  Point center() const override;

  double perimeter() const override;

  double area() const override;

  bool containsPoint(const Point& point) const override;

  void rotate(const Point& center, double angle) override;

  void reflect(const Point& center) override;

  void reflect(const Line& axis) override;

  void scale(const Point& center, double coefficient) override;
};

Circle::Circle(const Point& center, double radius)
    : Ellipse(center, center, 2 * radius) {}

double Circle::radius() const {
  return semi_major_axis_;
}

Point Circle::center() const {
  return focus1_;
}

double Circle::perimeter() const {
  return 2 * M_PI * semi_major_axis_;
}

double Circle::area() const {
  return M_PI * pow(semi_major_axis_, 2);
}

bool Circle::containsPoint(const Point& point) const {
  return details::is_smaller_or_equal(Point::distance(point, focus1_),
                                      semi_major_axis_);
}

void Circle::rotate(const Point& center, double angle) {
  focus1_ = focus2_ = {center.x + (focus1_.x - center.x) * cos(angle) -
                           (focus1_.y - center.y) * sin(angle),
                       center.y + (focus1_.x - center.x) * sin(angle) +
                           (focus1_.y - center.y) * cos(angle)};
}

void Circle::reflect(const Point& center) {
  focus1_ = focus2_ = {2 * center.x - focus1_.x, 2 * center.y - focus1_.y};
}

void Circle::reflect(const Line& axis) {
  Point center = Line::get_intersect(axis, axis.get_perpendicular(focus1_));
  focus1_ = focus2_ = {2 * center.x - focus1_.x, 2 * center.y - focus1_.y};
}

void Circle::scale(const Point& center, double coefficient) {
  focus1_ = focus2_ = {center.x + coefficient * (focus1_.x - center.x),
                       center.y + coefficient * (focus1_.y - center.y)};
  semi_major_axis_ = semi_minor_axis_ *= coefficient;
}

class Rectangle : public Polygon {
 public:
  Rectangle(const Point& point1, const Point& point2, double factor);

  Point center();

  std::pair<Line, Line> diagonals();
};

Rectangle::Rectangle(const Point& point1, const Point& point2, double factor)
    : Polygon(std::vector<Point>(4)) {
  Point vector = {point2.x - point1.x, point2.y - point1.y};

  double angle = atan(std::max(factor, 1 / factor));
  double sin_value = sin(angle);
  double cos_value = cos(angle);

  vector = {vector.x * cos_value - vector.y * sin_value,
            vector.x * sin_value + vector.y * cos_value};

  vector = {vector.x * cos_value, vector.y * cos_value};

  vertices_[0] = point1;
  vertices_[2] = point2;
  vertices_[1] = {point1.x + vector.x, point1.y + vector.y};
  vertices_[3] = {point2.x - vector.x, point2.y - vector.y};
}

Point Rectangle::center() {
  return Point::midpoint(vertices_[0], vertices_[2]);
}

std::pair<Line, Line> Rectangle::diagonals() {
  return {Line(vertices_[0], vertices_[2]), Line(vertices_[1], vertices_[3])};
}

class Square : public Rectangle {
 public:
  Square(const Point& point1, const Point& point2);

  Circle circumscribedCircle();

  Circle inscribedCircle();
};

Square::Square(const Point& point1, const Point& point2)
    : Rectangle(point1, point2, 1) {}

Circle Square::circumscribedCircle() {
  double radius = Point::distance(vertices_[0], vertices_[2]);
  return Circle{center(), radius};
}

Circle Square::inscribedCircle() {
  double radius = Point::distance(vertices_[0], vertices_[2]) / sqrt(2);
  return Circle{center(), radius};
}

class Triangle : public Polygon {
 public:
  Triangle(const Point& point1, const Point& point2, const Point& point3);

  Circle circumscribedCircle();

  Circle inscribedCircle();

  Point centroid();

  Point orthocenter();

  Line EulerLine();

  Circle ninePointsCircle();
};

Triangle::Triangle(const Point& point1,
                   const Point& point2,
                   const Point& point3)
    : Polygon(point1, point2, point3) {}

Circle Triangle::circumscribedCircle() {
  Line side1(vertices_[0], vertices_[1]);
  Line side2(vertices_[1], vertices_[2]);
  Point center = Line::get_intersect(side1.get_bisector_perpendicular(),
                                     side2.get_bisector_perpendicular());
  double radius = Point::distance(center, vertices_[0]);
  return Circle{center, radius};
}

Circle Triangle::inscribedCircle() {
  Point
      vector1{vertices_[1].x - vertices_[0].x, vertices_[1].y - vertices_[0].y};
  Point
      vector2{vertices_[1].x - vertices_[2].x, vertices_[1].y - vertices_[2].y};
  Point
      vector3{vertices_[2].x - vertices_[0].x, vertices_[2].y - vertices_[0].y};

  double len1 = Point::distance(vertices_[0], vertices_[1]);
  double len2 = Point::distance(vertices_[1], vertices_[2]);
  double len3 = Point::distance(vertices_[2], vertices_[0]);

  Point bi_vector1{vector1.x * len2 + vector2.x * len1,
                   vector1.y * len2 + vector2.y * len1};
  Point bi_vector2{vector1.x * len3 + vector3.x * len1,
                   vector1.y * len3 + vector3.y * len1};

  Line bisector1{vertices_[1], {vertices_[1].x + bi_vector1.x,
                                vertices_[1].y + bi_vector1.y}};
  Line bisector2{vertices_[0], {vertices_[0].x + bi_vector2.x,
                                vertices_[0].y + bi_vector2.y}};

  double radius = sqrt(
      (len1 + len2 - len3) * (len1 + len3 - len2) * (len3 + len2 - len1) /
          (len1 + len2 + len3) / 4);

  return Circle{Line::get_intersect(bisector1, bisector2), radius};
}

Point Triangle::centroid() {
  Line median1(vertices_[0], Point::midpoint(vertices_[1], vertices_[2]));
  Line median2(vertices_[1], Point::midpoint(vertices_[2], vertices_[0]));

  return Line::get_intersect(median1, median2);
}

Point Triangle::orthocenter() {
  Line height1 =
      Line{vertices_[0], vertices_[1]}.get_perpendicular(vertices_[2]);
  Line height2 =
      Line{vertices_[1], vertices_[2]}.get_perpendicular(vertices_[0]);

  return Line::get_intersect(height1, height2);
}

Line Triangle::EulerLine() {
  return Line{centroid(), orthocenter()};
}

Circle Triangle::ninePointsCircle() {
  Point center = Point::midpoint(circumscribedCircle().center(), orthocenter());

  double radius = circumscribedCircle().radius() / 2;

  return Circle{center, radius};
}
