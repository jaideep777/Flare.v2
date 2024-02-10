#include "stream.h"
using namespace std;

int main(){

	flare::Stream in_stream;

	vector<double> times(24);
	std::iota(times.begin(), times.end(), 12);

	in_stream.open(times, "months since 2001-1-1");
	in_stream.print_meta();
	in_stream.print_times();

	{
	/// TESTING OF T INDEX CALCULATION
	std::cout << "Calculate time index with periodic l-edged t vector\n\n";
	double t0 = flare::datestring_to_julian("2002-1-1 00:00:00");
	for (double t = t0; t < t0+15*12; t+=(365.2524/12)){
		std::cout << flare::julian_to_datestring(t) << ": " 
		          << in_stream.streamIdx_to_datestring(in_stream.julian_to_indices(t, true, false)) << "\n";
	}
	std::cout << "\n";

	std::cout << "Calculate time index with periodic l-edged t vector\n\n";
	t0 = flare::datestring_to_julian("2005-06-11 00:00:00");
	for (double t = t0; t < t0+20*12; t+=(365.2524/12)){
		std::cout << flare::julian_to_datestring(t) << ": " << in_stream.streamIdx_to_datestring(in_stream.julian_to_indices(t, true, false)) << "\n";
	}
	std::cout << "\n";

	std::cout << "Calculate time index with periodic l-edged t vector\n\n";
	t0 = flare::datestring_to_julian("2002-1-2 00:00:00");
	for (double t = t0; t > t0-15*12; t-=(365.2524/12)){
		std::cout << flare::julian_to_datestring(t) << ": " << in_stream.streamIdx_to_datestring(in_stream.julian_to_indices(t, true, false)) << "\n";
	}
	std::cout << "\n";
	}

	// {
	// /// TESTING OF T INDEX CALCULATION
	// std::cout << "Calculate time index with periodic centered t vector\n\n";
	// double t0 = flare::datestring_to_julian("2010-12-30 00:00:00");
	// for (double t = t0; t < t0+4; t+=0.125/4){
	// 	std::cout << flare::julian_to_datestring(t) << ": " << in_stream.streamIdx_to_datestring(in_stream.julian_to_indices(t, true, true)) << "\n";
	// }
	// std::cout << "\n";
	// }


	return 0;
}

