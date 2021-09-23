//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef RECT_HXX
#define RECT_HXX

#include <cassert>

#include "bspf.hxx"

namespace Common {

/*
  This small class is an helper for position and size values.
*/
struct Point
{
  Int32 x{0};  //!< The horizontal part of the point
  Int32 y{0};  //!< The vertical part of the point

  Point() = default;
  explicit Point(Int32 x1, Int32 y1) : x(x1), y(y1) { }
  explicit Point(const string& p) {
    char c = '\0';
    istringstream buf(p);
    buf >> x >> c >> y;
    if(c != 'x')
      x = y = 0;
  }
  bool operator==(const Point& p) const { return x == p.x && y == p.y; }
  bool operator!=(const Point& p) const { return !(*this == p);        }

  friend ostream& operator<<(ostream& os, const Point& p) {
    os << p.x << "x" << p.y;
    return os;
  }
};

struct Size
{
  uInt32 w{0};  //!< The width part of the size
  uInt32 h{0};  //!< The height part of the size

  Size() = default;
  explicit Size(uInt32 w1, uInt32 h1) : w(w1), h(h1) { }
  explicit Size(const string& s) {
    char c = '\0';
    istringstream buf(s);
    buf >> w >> c >> h;
    if(c != 'x')
      w = h = 0;
  }
  bool valid() const { return w > 0 && h > 0; }

  void clamp(uInt32 lower_w, uInt32 upper_w, uInt32 lower_h, uInt32 upper_h) {
    w = BSPF::clamp(w, lower_w, upper_w);
    h = BSPF::clamp(h, lower_h, upper_h);
  }

  bool operator==(const Size& s) const { return w == s.w && h == s.h; }
  bool operator< (const Size& s) const { return w <  s.w && h <  s.h; }
  bool operator> (const Size& s) const { return w >  s.w || h >  s.h; }
  bool operator!=(const Size& s) const { return !(*this == s); }
  bool operator<=(const Size& s) const { return !(*this >  s); }
  bool operator>=(const Size& s) const { return !(*this <  s); }

  friend ostream& operator<<(ostream& os, const Size& s) {
    os << s.w << "x" << s.h;
    return os;
  }
};

/*
  This small class is an helper for rectangles.
  Note: This implementation is built around the assumption that (top,left) is
  part of the rectangle, but (bottom,right) is not! This is reflected in
  various methods, including contains(), intersects() and others.

  Another very wide spread approach to rectangle classes treats (bottom,right)
  also as a part of the rectangle.

  Conceptually, both are sound, but the approach we use saves many intermediate
  computations (like computing the height in our case is done by doing this:
    height = bottom - top;
  while in the alternate system, it would be
    height = bottom - top + 1;

  When writing code using our Rect class, always keep this principle in mind!
*/
struct Rect
{
  private:
    //!< The point at the top left of the rectangle (part of the rect).
    uInt32 top{0}, left{0};
    //!< The point at the bottom right of the rectangle (not part of the rect).
    uInt32 bottom{0}, right{0};

  public:
    Rect() {}
    explicit Rect(const Size& s) : bottom(s.h), right(s.w) { assert(valid()); }
    Rect(uInt32 w, uInt32 h) : bottom(h), right(w) { assert(valid()); }
    Rect(const Point& p, uInt32 w, uInt32 h)
      : top(p.y), left(p.x), bottom(p.y + h), right(p.x + w) { assert(valid()); }
    Rect(uInt32 x1, uInt32 y1, uInt32 x2, uInt32 y2) : top(y1), left(x1), bottom(y2), right(x2) { assert(valid()); }

    uInt32 x() const { return left; }
    uInt32 y() const { return top;  }
    Point point() const { return Point(x(), y()); }

    uInt32 w() const { return right - left; }
    uInt32 h() const { return bottom - top; }
    Size size() const { return Size(w(), h()); }

    void setWidth(uInt32 aWidth)   { right = left + aWidth;  }
    void setHeight(uInt32 aHeight) { bottom = top + aHeight; }
    void setSize(const Size& size) { setWidth(size.w); setHeight(size.h); }

    void setBounds(uInt32 x1, uInt32 y1, uInt32 x2, uInt32 y2) {
      top = y1;
      left = x1;
      bottom = y2;
      right = x2;
      assert(valid());
    }

    bool valid() const {
      return (left <= right && top <= bottom);
    }

    bool empty() const {
      return top == 0 && left == 0 && bottom == 0 && right == 0;
    }

    void moveTo(uInt32 x, uInt32 y) {
      bottom += y - top;
      right += x - left;
      top = y;
      left = x;
    }

    void moveTo(const Point& p) {
      moveTo(p.x, p.y);
    }

    bool contains(uInt32 x, uInt32 y) const {
      return x >= left && y >= top && x < right && y < bottom;
    }

    // Tests whether 'r' is completely contained within this rectangle.
    // If it isn't, then set 'x' and 'y' such that moving 'r' to this
    // position will make it be contained.
    bool contains(uInt32& x, uInt32& y, const Rect& r) const {
      if(r.left < left)  x = left;
      else if(r.right > right) x = r.left - (r.right - right);
      if(r.top < top)  y = top;
      else if(r.bottom > bottom) y = r.top - (r.bottom - bottom);

      return r.left != x || r.top != y;
    }

    bool operator==(const Rect& r) const {
      return top == r.top && left == r.left && bottom == r.bottom && right == r.right;
    }
    bool operator!=(const Rect& r) const { return !(*this == r); }

    friend ostream& operator<<(ostream& os, const Rect& r) {
      os << r.point() << "," << r.size();
      return os;
    }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const Rect EmptyRect;

}  // End of namespace Common

#endif
