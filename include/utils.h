#ifndef FLARE_FLARE_UTILS_H
#define FLARE_FLARE_UTILS_H

#include <iostream>
#include <vector>
#include <cmath>

// print a std::vector via ofstream
// prints: size | v1 v2 v3 ...
template <class T>
std::ostream& operator << (std::ostream &os, const std::vector<T> &v) {
	//os << std::setprecision(12);
	os << v.size() << " | ";
	for (const auto &x : v) {
		os << x << ' ';
	}
	// os << '\n';
	return os;
}

namespace flare{
namespace utils{

inline double positive_fmod(double x, double period){
    double res = fmod(x, period);
    if (res < 0) res += period;
    return res;
}


} // namespace utils
} // namespace flare

#endif