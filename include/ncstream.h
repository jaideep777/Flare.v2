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
#include "stream.h"

namespace flare{

class NcStream : public Stream{
	public:
	NcFilePP current_file;

	public:

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

		if (times.size() > 0) tstep = (times.back() - times.front())/(times.size()-1); // get timestep in days
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

};


} // namespace flare

#endif
