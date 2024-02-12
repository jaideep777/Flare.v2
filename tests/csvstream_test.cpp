#include "csvstream.h"
using namespace std;

int main(){

	flare::CsvStream in_stream;

	in_stream.set_tname("Year");
	in_stream.open({"tests/data/CO2_AMB_AmzFACE2000_2010.csv",
	                "tests/data/CO2_AMB_AmzFACE2011_2020.csv",
					"tests/data/CO2_AMB_AmzFACE2021_2030.csv",
	                }, "decimal year");
	in_stream.print_meta();
	in_stream.print_times();

	in_stream.advance_to_time(flare::datestring_to_julian("2013-06-01 0:0:0"), true, false);
	in_stream.print_meta();

	cout << '|' << in_stream.current_row.get_line_raw() << '|' << endl;
	if (in_stream.current_row.get_line_raw() != "2013,396.5") return 1;

	in_stream.advance_to_time(flare::datestring_to_julian("2013-07-15 0:0:0"), true, false);
	in_stream.advance_to_time(flare::datestring_to_julian("2013-12-15 0:0:0"), true, false);
	in_stream.advance_to_time(flare::datestring_to_julian("2014-04-15 0:0:0"), true, false);

	cout << '|' << in_stream.current_row.get_line_raw() << '|' << endl;
	if (in_stream.current_row.get_line_raw() != "2014,399") return 1;

	in_stream.advance_to_time(flare::datestring_to_julian("2029-04-15 0:0:0"), true, false);

	cout << '|' << in_stream.current_row.get_line_raw() << '|' << endl;
	if (in_stream.current_row.get_line_raw() != "2029,414.2") return 1;

	// **** Test whether decimal year as unit works for random odd date ****

	flare::CsvStream in_stream1;

	in_stream1.set_tname("Year");
	in_stream1.open({"tests/data/CO2_AMB_AmzFACE1951-1953.csv"}, 
	                "decimal year");
	in_stream1.print_meta();
	in_stream1.print_times();

	in_stream1.advance_to_time(flare::datestring_to_julian("1952-06-01 0:0:0"), true, false);
	in_stream1.print_meta();

	cout << '|' << in_stream1.current_row.get_line_raw() << '|' << endl;
	if (in_stream1.current_row.get_line_raw() != "1952,360") return 1;


	return 0;
}

