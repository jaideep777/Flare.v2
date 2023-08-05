#include "flare.h"

int main(){

	flare::NcFilePP in_file;
	in_file.open("tests/data/gpp.2000-2015.nc", netCDF::NcFile::read);
	in_file.readMeta();
	in_file.printMeta();

	flare::GeoCube<float> v;
	v.readMeta(in_file);

	v.setIndices(v.lat_idx, 230, 1);
	v.setIndices(v.lon_idx, 161, 2);
	v.readBlock(0,1);
	v.print(true);

	// just a test of coord bounding
	std::reverse(v.coords[v.lat_idx].begin(), v.coords[v.lat_idx].end());
	v.setCoordBounds(v.lon_idx, 75, 101);
	v.setCoordBounds(v.lat_idx, 60, 80);

	v.setCoordBounds(v.lon_idx, 75.25, 101.75);
	v.setCoordBounds(v.lat_idx, 60.25, 80.75);

	v.setCoordBounds(v.lon_idx, 111.25, 131.75);
	v.setCoordBounds(v.lat_idx, -60.75, 12.25);
	std::reverse(v.coords[v.lat_idx].begin(), v.coords[v.lat_idx].end());
	// ~~~

	v.setCoordBounds(v.lat_idx, 18.5, 18.5);
	v.setCoordBounds(v.lon_idx, 77.25, 77.25);
	v.readBlock(0, 2);
	v.print(true);


	/// TESTING OF T INDEX CALCULATION
	std::cout << "Calculate time index with periodic centered t vector\n\n";
	double t0 = flare::datestring_to_julian("2000-01-01 00:00:00");
	for (double t = t0; t < t0+90; ++t){
		std::cout << flare::julian_to_datestring(t) << ": " << v.t_index_to_datestring(v.julian_to_index(t, true, true)) << "\n";
	}
	std::cout << "\n";
	t0 = flare::datestring_to_julian("2016-01-01 00:00:00");
	for (double t = t0; t < t0+60; ++t){
		std::cout << flare::julian_to_datestring(t) << ": " << v.t_index_to_datestring(v.julian_to_index(t, true, true)) << "\n";
	}
	std::cout << "\n";
	t0 = flare::datestring_to_julian("1999-01-01 00:00:00"); // expect 2015-M-D
	for (double t = t0; t < t0+60; ++t){
		std::cout << flare::julian_to_datestring(t) << ": " << v.t_index_to_datestring(v.julian_to_index(t, true, true)) << "\n";
	}

	std::cout << "\n\nCalculate time index with periodic non-centered t vector\n\n";
	t0 = flare::datestring_to_julian("2000-01-01 00:00:00");
	for (double t = t0; t < t0+90; ++t){
		std::cout << flare::julian_to_datestring(t) << ": " << v.t_index_to_datestring(v.julian_to_index(t, true, false)) << "\n";
	}
	std::cout << "\n";
	t0 = flare::datestring_to_julian("2016-01-01 00:00:00");
	for (double t = t0; t < t0+60; ++t){
		std::cout << flare::julian_to_datestring(t) << ": " << v.t_index_to_datestring(v.julian_to_index(t, true, false)) << "\n";
	}
	std::cout << "\n";
	t0 = flare::datestring_to_julian("1999-01-01 00:00:00"); // expect 2015-M-D
	for (double t = t0; t < t0+60; ++t){
		std::cout << flare::julian_to_datestring(t) << ": " << v.t_index_to_datestring(v.julian_to_index(t, true, false)) << "\n";
	}

	std::cout << "\n\nCalculate time index with non-periodic non-centered t vector\n\n";
	t0 = flare::datestring_to_julian("2000-01-01 00:00:00");
	for (double t = t0; t < t0+90; ++t){
		std::cout << flare::julian_to_datestring(t) << ": " << v.t_index_to_datestring(v.julian_to_index(t, false, false)) << "\n";
	}
	std::cout << "\n";
	t0 = flare::datestring_to_julian("2016-01-01 00:00:00");
	for (double t = t0; t < t0+60; ++t){
		std::cout << flare::julian_to_datestring(t) << ": " << v.t_index_to_datestring(v.julian_to_index(t, false, false)) << "\n";
	}
	std::cout << "\n";
	t0 = flare::datestring_to_julian("1999-01-01 00:00:00"); // expect 2015-M-D
	for (double t = t0; t < t0+60; ++t){
		std::cout << flare::julian_to_datestring(t) << ": " << v.t_index_to_datestring(v.julian_to_index(t, false, false)) << "\n";
	}

	std::cout << "\n\nCalculate time index with non-periodic centered t vector\n\n";
	t0 = flare::datestring_to_julian("2000-01-01 00:00:00");
	for (double t = t0; t < t0+90; ++t){
		std::cout << flare::julian_to_datestring(t) << ": " << v.t_index_to_datestring(v.julian_to_index(t, false, true)) << "\n";
	}
	std::cout << "\n";
	t0 = flare::datestring_to_julian("2016-01-01 00:00:00");
	for (double t = t0; t < t0+60; ++t){
		std::cout << flare::julian_to_datestring(t) << ": " << v.t_index_to_datestring(v.julian_to_index(t, false, true)) << "\n";
	}
	std::cout << "\n";
	t0 = flare::datestring_to_julian("1999-01-01 00:00:00"); // expect 2015-M-D
	for (double t = t0; t < t0+60; ++t){
		std::cout << flare::julian_to_datestring(t) << ": " << v.t_index_to_datestring(v.julian_to_index(t, false, true)) << "\n";
	}

	return 0;
} 

