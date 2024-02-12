#include "csvstream.h"
using namespace std;

int main(){

	flare::CsvStream in_stream;

	in_stream.open({"tests/data/MetData_AmzFACE_Monthly_2000_2015_PlantFATE.csv"}, 
					"years since 0000-1-1");
	in_stream.print_meta();
	in_stream.print_times();

	in_stream.advance_to_time(flare::datestring_to_julian("2002-01-04"), true, false);
	in_stream.print_meta();
	cout << "Data:\n";
	// cout << '|' << in_stream.current_row.get_line_raw() << '|' << endl;
	for (int i=0; i<in_stream.current_row.size(); ++i) cout << in_stream.current_row[i] << "\t";
	cout << '\n';
	
	ofstream fout("csvstream_met.txt");
	double t0 = flare::datestring_to_julian("1921-01-04");
	double tf = flare::datestring_to_julian("2081-12-31");
	for (double t = t0; t <= tf; t += 365.2425/12.0/1.0){
		in_stream.advance_to_time(t, true, false);
		fout << t << "\t" << flare::decimal_year(flare::julian_to_date(t)) << "\t";
		for (int i=0; i<in_stream.current_row.size(); ++i) fout << in_stream.current_row[i] << "\t";
		fout << '\n';
	}
	fout.close();


	return 0;
}

