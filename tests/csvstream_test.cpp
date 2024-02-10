#include "csvstream.h"
using namespace std;

int main(){

	flare::CsvStream in_stream;

	in_stream.set_tname("Year");
	in_stream.open({"tests/data/CO2_AMB_AmzFACE2000_2010.csv",
	                "tests/data/CO2_AMB_AmzFACE2011_2020.csv",
					"tests/data/CO2_AMB_AmzFACE2021_2030.csv",
	                }, "years since 0000-1-15");
	in_stream.print_meta();
	in_stream.print_times();

	in_stream.advance_to_time(flare::datestring_to_julian("2013-06-01 0:0:0"), true, false);
	in_stream.print_meta();
	cout << '|' << in_stream.rows.back().get_line_raw() << '|' << endl;

	if (in_stream.rows.back().get_line_raw() != "2013,396.5") return 1;

	// {
	// /// TESTING OF T INDEX CALCULATION
	// std::cout << "Calculate time index with periodic l-edged t vector\n\n";
	// double t0 = flare::datestring_to_julian("2002-1-1 00:00:00");
	// for (double t = t0; t < t0+15*12; t+=(365.2524/12)){
	// 	std::cout << flare::julian_to_datestring(t) << ": " 
	// 	          << in_stream.streamIdx_to_datestring(in_stream.julian_to_indices(t, true, false)) << "\n";
	// }
	// std::cout << "\n";

	// std::cout << "Calculate time index with periodic l-edged t vector\n\n";
	// t0 = flare::datestring_to_julian("2005-06-11 00:00:00");
	// for (double t = t0; t < t0+20*12; t+=(365.2524/12)){
	// 	std::cout << flare::julian_to_datestring(t) << ": " << in_stream.streamIdx_to_datestring(in_stream.julian_to_indices(t, true, false)) << "\n";
	// }
	// std::cout << "\n";

	// std::cout << "Calculate time index with periodic l-edged t vector\n\n";
	// t0 = flare::datestring_to_julian("2002-1-2 00:00:00");
	// for (double t = t0; t > t0-15*12; t-=(365.2524/12)){
	// 	std::cout << flare::julian_to_datestring(t) << ": " << in_stream.streamIdx_to_datestring(in_stream.julian_to_indices(t, true, false)) << "\n";
	// }
	// std::cout << "\n";
	// }

	return 0;
}

