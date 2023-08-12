#ifndef FLARE_FLARE_NCSTREAM_H
#define FLARE_FLARE_NCSTREAM_H

#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <netcdf>
#include <chrono>
#include <cmath>
#include <numeric>

#include "ncfilepp.h"
#include "time_math.h"

namespace flare{

struct StreamIndex{
	size_t idx;
	size_t f_idx;
	size_t t_idx;
};

class NcStream{
	public:
	size_t   current_file_index;
	double   current_time;
	NcFilePP current_file;

	private:
	std::vector<std::string> filenames;

	std::vector<double> times;   // combined times vector from all files
	std::vector<size_t> file_indices; // file index corresponding to t in times
	std::vector<size_t> t_indices;    // time index within file corresponding to t in times

	std::string unit_str;
	std::string tunit = ""; // time unit in file
	double tstep;           // interval between data frames [days] 
	double tscale = 1;      // multiplier to convert time from file's unit to 'days'
	std::tm t_base = {};    // epoch used in file
	double DeltaT;          // Total duration represented in combined times vector

	public:

	inline void reset(){
		current_file_index = -1;
		times.clear();
		file_indices.clear();
		t_indices.clear();
	}

	inline void open(std::vector<std::string> _filenames, std::vector<std::string> tnames = {"time", "t"}){
		reset();

		filenames = _filenames;
		
		// To construct the time vector, we need to open each file once and obtain its time vector
		for (size_t i = 0; i<filenames.size(); ++i){
			std::string fname = filenames[i];
			current_file.open(fname, netCDF::NcFile::read);

			std::multimap<std::string, netCDF::NcVar> vars_map_temp = current_file.getVars();

			netCDF::NcVar tVar;
			for (auto p : vars_map_temp){
				std::string name = p.first;  // get variable name
				// convert to lowercase
				std::transform(name.begin(), name.end(), name.begin(),
		               [](unsigned char c){ return std::tolower(c); });
				// check if the name represents time   
				auto it = std::find(tnames.begin(), tnames.end(), name); 
				// if found, read the corresponding var and break
				if (it != tnames.end()){
					tVar = p.second;
					break;
				}
			}

			if (tVar.isNull()) throw std::runtime_error("NcStream: No time dimension in the specified NcFiles\n");

			std::vector<double> tvec(tVar.getDim(0).getSize());
			tVar.getVar(tvec.data());

			times.insert(times.end(), tvec.begin(), tvec.end());
			file_indices.resize(times.size(), i);
			
			std::vector<size_t> idxes(tvec.size());
			std::iota(idxes.begin(), idxes.end(), 0);

			t_indices.insert(t_indices.end(), idxes.begin(), idxes.end());

			std::string _unit_str;
			try{ tVar.getAtt("units").getValues(_unit_str); }
			catch(netCDF::exceptions::NcException &e){ std::cout << "Warning: Time variable does not have a unit\n";}

			// Read and parse time unit from first file. This will be used to check time units in subsequent files
			if (i == 0){
				unit_str = _unit_str;
				parse_time_unit(unit_str); // sets tunit, tscale, t_base
			}
			else{
				if (unit_str != _unit_str) throw std::runtime_error("NcStream: Time units dont match among specfied files\n"); 
			}

			current_file.close();
		}

		if (!std::is_sorted(times.begin(), times.end()))  throw std::runtime_error("NcStream: Combined time vector is not in ascending order. Check the order of files supplied.\n");

		for (auto& t : times) t *= tscale; // convert time vector to "days since base date"

		if (times.size() > 1) tstep = (times[times.size()-1]-times[0])/(times.size()-1); // get timestep in days
		else                  tstep = 0;

		DeltaT = times[times.size()-1] - times[0] + tstep;

		// open first file
		update_file(0);

	}

	void update_file(size_t file_id){
		current_file.close();
		current_file.open(filenames[file_id], netCDF::NcFile::read);
		current_file.readMeta();
		current_file_index = file_id;
	}

	inline void print_meta(){
		std::cout << "Stream: \n";
		std::cout << "   DeltaT = " << DeltaT << " days\n";
		std::cout << "   tstep = " << tstep << " days\n";
		std::cout << "   t_base = " << date_to_string(t_base) << "\n";
	}

	inline void print_times(){
		std::cout << "f_idx\tt_idx\tt\n"; // << times << '\n';
		for (int i=0; i<times.size(); ++i){
			std::cout << file_indices[i] << '\t' << t_indices[i] << '\t' << julian_to_datestring(date_to_julian(t_base) + times[i]) << '\n';
		}
		std::cout << '\n';
	}

	/// @brief            get the index in time vector corresponding to julian day j
	/// @param j          julian day for which to read data
	/// @param periodic   whether data should be extended periodically
	/// @param centred_t  whether t at index represents centre of interval (or start of interval)
	/// @return           index in time vector for which data should be read
	StreamIndex julian_to_indices(double j, bool periodic, bool centred_t){

		// convert desired time to file unit (days since tbase)
		double t = j - date_to_julian(t_base); 

		if (centred_t) t += tstep/2;     //   |----0----|-----1----|----2----|---
                                               //   x--->0    |     1    | shift t (x) by half the interval size
		                                       //   |    x--->0     1    |    2
											   //   |    0  x--->0  1    |    2
											   //   |    0    | x--->1   [note this one just beyound the interval midpoint, when shifted, goes beyond 1, and returns 1 rather than 0]
		
		// Calculate total time range of data in the file, and bring t to principle range if periodic extension is desired
		if (periodic) t = times[0] + utils::positive_fmod(t - times[0], DeltaT);

		// calculate index such that tvec[idx] is just less than t
		auto t_it = std::upper_bound(times.begin(), times.end(), t);
		int idx = t_it - times.begin() - 1;

		// clamp the start and end points. this case will arise only when periodic is false. 
		idx = std::clamp(idx, 0, int(times.size()-1)); 
		
		StreamIndex sid;
		sid.idx = idx;
		sid.f_idx = file_indices[idx];
		sid.t_idx = t_indices[idx];

		return sid;
	}

	std::string streamIdx_to_datestring(const StreamIndex& sid){
		return julian_to_datestring(times[sid.idx] + date_to_julian(t_base));
	}

	private:

	void parse_time_unit(const std::string& unit_str){
		// parse time units
		std::string since;
		std::stringstream ss(unit_str);
		ss >> tunit >> since;

		if (since != "since") throw std::runtime_error("time unit is not in the correct format (<units> since <yyyy-mm-dd> <hh:mm:ss>)");

		if      (tunit == "days")     tscale = 1;
		else if (tunit == "hours")    tscale = 1.0/24.0;
		else if (tunit == "minutes")  tscale = 1.0/24.0/60.0;
		else if (tunit == "seconds")  tscale = 1.0/24.0/3600.0;
		else if (tunit == "months")   tscale = 1.0*365.2425/12;
		else if (tunit == "years")    tscale = 1.0*365.2425;

		std::stringstream ss1(unit_str);
		ss1 >> std::get_time(&t_base, std::string(tunit + " since %Y-%m-%d %H:%M:%S").c_str());
	}

};


} // namespace flare

#endif
