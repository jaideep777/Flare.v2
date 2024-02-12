#ifndef FLARE_FLARE_STREAM_H
#define FLARE_FLARE_STREAM_H

#include <ios>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>

#include "utils.h"
#include "time_math.h"

namespace flare{

/// A set of indices that locates a given time the provided files.
struct StreamIndex{
	size_t idx = 0;   ///< Index within the concatenated times vector (full index).
	size_t f_idx = 0; ///< Index of the file containing the time at times[idx].
	size_t t_idx = 0; ///< Index within the file's time vector corresponding to times[idx].

	inline void set(size_t _idx, size_t _f_idx, size_t _t_idx){
		idx = _idx; f_idx = _f_idx; t_idx = _t_idx;
	}

	inline bool operator==(const StreamIndex& rhs) const{
        return (idx == rhs.idx) && 
		       (f_idx == rhs.f_idx) && 
		       (t_idx == rhs.t_idx);
    }
};

class Stream{
	public:
	StreamIndex current_index;    ///< Index of current record.
	// double current_time;       ///< Current time.

	protected:
	std::vector<std::string> filenames; ///< Names of files containing temporal data.

	std::vector<double> times;        ///< combined times vector concatenated from all files
	std::vector<size_t> file_indices; ///< For each t = times[i], this stores the index of the file which contains t
	std::vector<size_t> t_indices;    ///< For each t = times[i], this stores the index within the file's time vector that corresponds to t
	std::vector<size_t> idx_f0;       ///< full index corresponding to the first entry in file. Used for convenience

	std::string unit_str;   ///< full string representation of time unit (e.g., "days since yyyy-mm-dd hh:mm:ss")
	std::string tunit = ""; ///< time unit in file (e.g., "days" or "months")
	double tstep = 0;       ///< interval between data frames [days] 
	double tscale = 1;      ///< multiplier to convert time from file's unit to 'days'
	std::tm t_base = {};    ///< epoch (base time) used in file
	double DeltaT = 0;      ///< Total duration represented in combined times vector

	std::vector<std::string> tnames = {"time", "t"}; ///< Names to try when searcing for the time dimension in file

	public:
	inline virtual ~Stream(){
	}

	inline virtual void reset(){
		times.clear();
		file_indices.clear();
		t_indices.clear();
		filenames.clear();
		idx_f0.clear();
	}

	/// @brief Specify the name of the time dimension in file
	/// @param tname Name of the time dimension
	/// 
	inline void set_tname(const std::string& tname){
		tnames = {tname};
	}

	/// @brief Opens a stream using just a time vector and time unit.
	/// @param tvec The vector of time values.
	/// @param _tunit The time unit.
	/// This function creates a stream from a single time vector, file index is always 0.
	/// This function is not made virtual because we do not expect specialized streams to override it.
	inline void open(const std::vector<double>& tvec, std::string _tunit){
		reset();
		
		idx_f0.push_back(times.size());

		times.insert(times.end(), tvec.begin(), tvec.end());
		file_indices.resize(times.size(), 0);
		
		std::vector<size_t> idxes(tvec.size());
		std::iota(idxes.begin(), idxes.end(), 0);

		t_indices.insert(t_indices.end(), idxes.begin(), idxes.end());

		unit_str = _tunit;
		parse_time_unit(unit_str); // sets tunit, tscale, t_base

		for (auto& t : times) t *= tscale; // convert time vector to "days since base date"

		if (times.size() > 0){
			tstep = (times.back() - times.front())/(times.size()-1); // get timestep in days
			DeltaT = times[times.size()-1] - times[0] + tstep;
		}

	}


	/// @brief Print stream metadata
	/// Allow derived streams to override this as there could be more metadata in them
	inline virtual void print_meta(){
		std::cout << "Stream: \n";
		std::cout << "   time unit: " << unit_str << '\n';
		std::cout << "   DeltaT = " << DeltaT << " days\n";
		std::cout << "   tstep = " << tstep << " days\n";
		std::cout << "   t_base = " << date_to_string(t_base) << "\n";
		std::cout << "   current_index: " << current_index.idx << "[" << current_index.f_idx << "." << current_index.t_idx << "]" << "\n";
		std::cout << "   current_time: " << streamIdx_to_datestring(current_index) << "\n";
		std::cout << "   files: \n";
		for (int i=0; i<filenames.size(); ++i){
			std::cout << "      " << idx_f0[i] << ": " << filenames[i] << '\n';
		}
	}

	inline void print_times(){
		auto f = std::cout.flags();
		std::cout << "         t\tidx\tf_idx\tt_idx\tpretty t\n"; // << times << '\n';
		for (int i=0; i<times.size(); ++i){
			std::cout << std::fixed << std::setw(10) << std::setprecision(4) 
					  << times[i] << '\t' << i << "\t" << file_indices[i] << '\t' << t_indices[i] << '\t' << julian_to_datestring(date_to_julian(t_base) + times[i]) << '\n';
		}
		std::cout << '\n';
		std::cout.flags(f);
	}


	/// @brief            Get the index corresponding to julian day j
	/// @param j          julian day for which to read data
	/// @param periodic   whether data should be extended periodically
	/// @param centred_t  whether t at index represents centre of interval (if true) or start of interval (if false)
	/// @return           index in the stream for which data should be read
	inline StreamIndex julian_to_indices(double j, bool periodic, bool centred_t){

		// convert desired time to file unit (days since tbase)
		double t = j - date_to_julian(t_base); 
		// std::cout << "t in file units = " << t << "\n";

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
		sid.set(idx, file_indices[idx], t_indices[idx]);

		return sid;
	}

	inline std::string streamIdx_to_datestring(const StreamIndex& sid){
		return julian_to_datestring(times[sid.idx] + date_to_julian(t_base));
	}


	/// Advances by a given number of indices.
	/// This method advances cyclically within the time vector by a given number of steps.
	/// @param sid The current StreamIndex.
	/// @param n The number of indices to advance, negative to go backwards.
	/// @param periodic Whether index should move cyclically. Otherwise, it will be clamped at 0 and max.
	/// @return The StreamIndex after advancement.
	inline StreamIndex advance(const StreamIndex& sid, int n, bool periodic){
		StreamIndex sid_next;

		if (periodic) sid_next.idx = utils::positive_mod(int(sid.idx) + n, times.size());
		else          sid_next.idx = std::clamp(int(sid.idx) + n, 0, int(times.size()-1)); 

		sid_next.f_idx = file_indices[sid_next.idx];
		sid_next.t_idx = t_indices[sid_next.idx];
		return sid_next;
	}


	inline virtual void advance_to_time(double j, bool periodic, bool centered_t){
		current_index = julian_to_indices(j, periodic, centered_t);
	} 


	protected:

	inline void parse_time_unit(const std::string& tunit_str){
		// parse time units
		std::string since;
		std::stringstream ss(tunit_str);
		ss >> tunit >> since;

		if (since != "since") throw std::runtime_error("time unit is not in the correct format (<units> since <yyyy-mm-dd> <hh:mm:ss>)");

		if      (tunit == "days")     tscale = 1;
		else if (tunit == "hours")    tscale = 1.0/24.0;
		else if (tunit == "minutes")  tscale = 1.0/24.0/60.0;
		else if (tunit == "seconds")  tscale = 1.0/24.0/3600.0;
		else if (tunit == "months")   tscale = 1.0*365.2425/12;
		else if (tunit == "years")    tscale = 1.0*365.2425;

		if (tunit == "months" || tunit == "years") std::cout << "Warning: using " << tunit << " as time unit. 365.2425 days per year will be assumed. Time points corresponding to exact dates will not be accurate.\n";

		ss.str(tunit_str);
		ss >> std::get_time(&t_base, std::string(tunit + " since %Y-%m-%d %H:%M:%S").c_str());
		t_base.tm_zone = "GMT";
	}

};


} // namespace flare

#endif
