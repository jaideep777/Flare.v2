#ifndef FLARE_FLARE_GEOCUBE_H
#define FLARE_FLARE_GEOCUBE_H

#include <tensor.h>
#include "ncfilepp.h"
#include "time_math.h"

namespace flare{

template <class T>
class GeoCube : public Tensor<T> {
	public:
	std::string name;       // Variable name
	std::string unit;
	std::vector <std::string> dimnames;
	int unlim_idx = -1;
	float scale_factor = 1.0, add_offset = 0.0; 
	std::vector<std::vector<double>> coords;
	std::vector<std::vector<double>> coords_trimmed;

	int lon_idx, lat_idx, t_idx;

	// data for standardizing dimension names (kept public to be editable)
	std::vector<std::string>   t_names_try = {"time"};
	std::vector<std::string> lev_names_try = {"lev", "level", "z"};
	std::vector<std::string> lat_names_try = {"lat", "latitude", "y"};
	std::vector<std::string> lon_names_try = {"lon", "longitude", "x"};

	private:
	netCDF::NcVar ncvar;
	std::vector <size_t> dimsizes;
	std::vector <std::string> dimunits;

	std::map<std::string, std::string> renaming_map; // map used for renaming dimensions to standard names

	std::vector <size_t> starts, counts;
	std::vector <ptrdiff_t> strides;

	std::string tunit = ""; // time unit in file
	double tstep;           // interval between data frames [days] 
	double tscale = 1;      // multiplier to convert time from file's unit to 'days'
	std::tm t_base = {};    // epoch used in file

	public:
	void readMeta(NcFilePP &in_file, std::string varname = ""){
		// get the named variable, or the first variable in the file
		if (varname != "") ncvar = in_file.vars_map.find(varname)->second;
		else ncvar = in_file.vars_map.begin()->second;

		// get variable name and dimensions
		name = ncvar.getName();
		std::vector <netCDF::NcDim> ncdims = ncvar.getDims();

		// get names and sizes of dimensions for this variable
		for (auto d : ncdims){
			dimnames.push_back(d.getName());
			dimsizes.push_back(d.getSize());
		}

		// copy the coordinate vectors that were read during file reading
		for (auto s : dimnames){
			coords.push_back(in_file.coordvalues_map.at(s));
		}
		coords_trimmed = coords;

		// ~~ standardize dimension names ~~
		// 1. create renaming map
		for (auto s :   t_names_try) renaming_map[s] = "time";
		for (auto s : lev_names_try) renaming_map[s] = "lev";
		for (auto s : lat_names_try) renaming_map[s] = "lat";
		for (auto s : lon_names_try) renaming_map[s] = "lon";

		for (auto& dname : dimnames){
			// 2. convert to lowecase
			std::transform(dname.begin(), dname.end(), dname.begin(),
						[](unsigned char c){ return std::tolower(c); });
			// 3. rename to standard name
			if (renaming_map.find(dname) != renaming_map.end()){
				dname = renaming_map[dname];
			}
		}

		// ~~ Get the dimension indices. i.e., which index in ncdim std::vector is lat, lon, etc
		lon_idx = std::find(dimnames.begin(), dimnames.end(), "lon") - dimnames.begin();
		lat_idx = std::find(dimnames.begin(), dimnames.end(), "lat") - dimnames.begin();
		if (lon_idx >= dimsizes.size() || lat_idx >= dimsizes.size()) throw std::runtime_error("Lat or Lon not found"); 
		
		// get unlimited dimension id
		for (int i=0; i<ncdims.size(); ++i) if (ncdims[i].isUnlimited()) unlim_idx = i;
	
		// by default, set to read entire geographic extent at 1 timeslice 
		starts.clear(); starts.resize(ncdims.size(), 0);   // set all starts to 0
		counts = dimsizes;                                 // set all counts to dimension size (all elements)
		strides.clear(); strides.resize(ncdims.size(), 1); // set all strides to 1
		if (unlim_idx >=0) counts[unlim_idx] = 1;          // If there's an unlimited dimension, set that count to 1

		// Get basic variable attributes
		// missing value
		try{ ncvar.getAtt("missing_value").getValues(&this->missing_value);}
		catch(netCDF::exceptions::NcException &e){
			try{ ncvar.getAtt("_FillValue").getValues(&this->missing_value);}
			catch(netCDF::exceptions::NcException &e){ std::cout << "Missing/Fill value not found. Setting to NaN\n";}
		}

		// unit
		try{ ncvar.getAtt("units").getValues(unit); }
		catch(netCDF::exceptions::NcException &e){ std::cout << "Warning: Variable does not have a unit\n";}

		// scale factor
		try{ ncvar.getAtt("scale_factor").getValues(&scale_factor); }
		catch(netCDF::exceptions::NcException &e){ scale_factor = 1.f;}
		
		// offset
		try{ ncvar.getAtt("add_offset").getValues(&add_offset);}
		catch(netCDF::exceptions::NcException &e){ add_offset = 0.f;}

		// if the variable has a time dimension, then parse time units 
		auto t_it = std::find(dimnames.begin(), dimnames.end(), "time");
		if (t_it != dimnames.end()){
			t_idx = t_it - dimnames.begin();
			parse_time_unit(in_file);
		}
		else{
			t_idx = -1;
			std::cout << "Warning: Variable does not have a time dimension\n";
		}
	}


	void print(bool b_values = false){
		std::cout << "Var: " << name << " (" << unit << ")\n";
		std::cout << "   dim names: " << dimnames;
		std::cout << "   dim sizes (original): " << dimsizes;
		std::cout << "      lat axis = " << lat_idx << "\n";
		std::cout << "      lon axis = " << lon_idx << "\n";
		std::cout << "      unlimited axis = ";
		if (unlim_idx < 0) std::cout << "NA\n";
		else std::cout << dimnames[unlim_idx] << " (" << unlim_idx << ")\n";
		std::cout << "   missing value = " << this->missing_value << "\n";
		std::cout << "   scale factor = " << scale_factor << "\n";
		std::cout << "   add offset = " << add_offset << "\n";
		std::cout << "   tbase = " << std::put_time(&t_base, "%Y-%m-%d %H:%M:%S %Z") << "\n";
		std::cout << "   tscale = " << tscale << " (" << tunit << ")\n";
		std::cout << "   tstep = " << tstep << " days" << "\n";
		std::cout << "   dimensions:\n";
		for (int i=0; i<dimnames.size(); ++i){
			std::cout << "      " << dimnames[i] << ": ";
			if (coords_trimmed[i].size() <= 6) std::cout << coords_trimmed[i];
			else{
				std::cout << coords_trimmed[i].size() << " | " << coords_trimmed[i][0] << " " << coords_trimmed[i][1] << " " << coords_trimmed[i][2] << " ... " 
				          << coords_trimmed[i][coords_trimmed[i].size()-3] << " " << coords_trimmed[i][coords_trimmed[i].size()-2] << " " << coords_trimmed[i][coords_trimmed[i].size()-1] << "\n";
			}
		}

		Tensor<T>::print("   ", b_values);
	}

	
	void setCoordBounds(size_t axis, float lo, float hi){
		double descending = *coords[axis].rbegin() < *coords[axis].begin(); // check if the coordinate values are descending
		// get the start and end coordinate values that are just outside the lo-hi range
		size_t start=0, end=0;
		if (!descending){
			for (int i=0; i< coords[axis].size(); ++i) if (coords[axis][i] < lo) start = i; 
			for (int i=coords[axis].size()-1; i>=0; --i) if (coords[axis][i] > hi) end = i; 
		}
		else{
			for (int i=0; i< coords[axis].size(); ++i) if (coords[axis][i] > hi) start = i; 
			for (int i=coords[axis].size()-1; i>=0; --i) if (coords[axis][i] < lo) end = i; 
		}
		starts[axis] = start;
		counts[axis] = end - start + 1;

		coords_trimmed[axis].assign(coords[axis].begin()+start, coords[axis].begin()+end+1);

		// std::cout << "axis = " << dimnames[axis] << '\n';
		// std::cout << "Start: " << start << " " << coords[axis][start] << '\n';
		// std::cout << "End: " << end << " " << coords[axis][end] << '\n';
		// if (!descending) std::cout << "<<< " << coords[axis][start] << " | " << lo <<  " " << hi << " | " << coords[axis][end] << "\n";
		// else std::cout << ">>> " << coords[axis][start] << " | " << hi <<  " " << lo << " | " << coords[axis][end] << "\n";
	}

	void setIndices(size_t axis, size_t _start, size_t _count, ptrdiff_t _stride = 1){
		starts[axis]  = _start;
		counts[axis]  = _count;
		strides[axis] = _stride;
	}

	void readBlock(size_t unlim_start, size_t unlim_count){
		if (unlim_idx >= 0){
			starts[unlim_idx] = unlim_start;
			counts[unlim_idx] = unlim_count;
		}
		std::cout << "Resizing tensor to: " << counts;
		this->resize(counts);
		ncvar.getVar(starts, counts, strides, this->vec.data());
	}

	void readBlock(double julian_day, bool periodic, bool centred_t){
		if (t_idx > 0){
			starts[t_idx] = julian_to_index(julian_day, periodic, centred_t);
			counts[t_idx] = 1;
		}
		std::cout << "Resizing tensor to: " << counts;
		this->resize(counts);
		ncvar.getVar(starts, counts, strides, this->vec.data());
	}


	/// @brief            get the index in time vector corresponding to julian day j
	/// @param j          julian day for which to read data
	/// @param periodic   whether data should be extended periodically
	/// @param centred_t  whether t at index represents centre of interval (or start of interval)
	/// @return           index in time vector for which data should be read
	size_t julian_to_index(double j, bool periodic, bool centred_t){
		if (t_idx < 0) throw std::runtime_error("julian_to_index: no time vector in file");

		auto& tvec = coords_trimmed[t_idx];

		// convert desired time to file unit (days since tbase)
		double t = j - date_to_julian(t_base); 

		if (centred_t) t += tstep/2;     //   |----0----|-----1----|----2----|---
                                               //   x--->0    |     1    | shift t (x) by half the interval size
		                                       //   |    x--->0     1    |    2
											   //   |    0  x--->0  1    |    2
											   //   |    0    | x--->1   [note this one just beyound the interval midpoint, when shifted, goes beyond 1, and returns 1 rather than 0]
		
		// Calculate total time range of data in the file, and bring t to principle range if periodic extension is desired
		double DeltaT = tvec[tvec.size()-1] - tvec[0] + tstep;
		if (periodic) t = tvec[0] + utils::positive_fmod(t - tvec[0], DeltaT);

		// calculate index such that tvec[idx] is just less than t
		auto t_it = std::lower_bound(tvec.begin(), tvec.end(), t);
		int idx = t_it - tvec.begin() - 1;

		// clamp the start and end points. this case will arise only when periodic is false. 
		idx = std::clamp(idx, 0, int(tvec.size()-1)); 
		
		return idx;
	}


	std::string t_index_to_datestring(int i){
		double j = coords_trimmed[t_idx][i] + date_to_julian(t_base);
		return julian_to_datestring(j);
	}

	private:

	void parse_time_unit(NcFilePP &in_file){
		// parse time units
		std::string since;
		std::stringstream ss(in_file.coordunits_map["time"]);
		ss >> tunit >> since;

		if (since != "since") throw std::runtime_error("time unit is not in the correct format (<units> since <yyyy-mm-dd> <hh:mm:ss>)");

		if      (tunit == "days")   tscale = 1;
		else if (tunit == "hours")  tscale = 1.0/24.0;
		else if (tunit == "months") tscale = 1.0*365.2425/12;

		tstep = 0;
		auto& tvec = coords_trimmed[t_idx];
		for (auto& t : tvec) t *= tscale; // convert time vector to "days since base date"

		if (tvec.size() > 1) tstep = (tvec[tvec.size()-1]-tvec[0])/(tvec.size()-1); // get timestep in days

		std::stringstream ss1(in_file.coordunits_map["time"]);
		ss1 >> std::get_time(&t_base, std::string(tunit + " since %Y-%m-%d %H:%M:%S").c_str());
	
		// std::cout << std::put_time(&t_base, "%Y-%m-%d %H:%M:%S %Z") << '\n';
		// std::cout << "days since 1970-1-1: " << (std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::from_time_t(std::mktime(&t_base)).time_since_epoch())).count() << "\n";
		// std::cout << "julian days (since -4173/11/24: " << (std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::from_time_t(std::mktime(&t_base)).time_since_epoch()) + std::chrono::hours{58574100}).count() << "\n";
	}


};

} // namespace flare

#endif
