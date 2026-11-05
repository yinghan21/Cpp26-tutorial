#include "geo.h"

#include <format>

double Circle::area() const {
    return kPi * r * r;
}

std::string describe(const Circle& c) {
    return std::format("circle(r={:.1f}) area={:.2f} perimeter={:.2f}",
                       c.r, c.area(), perimeter(c));
}
