#ifndef FLARE_FLARE_NCFILEPP_H
#define FLARE_FLARE_NCFILEPP_H

#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <netcdf>
#include <chrono>
#include <cmath>

#include "utils.h"

namespace flare{


class NcFilePP : public netCDF::NcFile {
	public:
	std::multimap <std::string, netCDF::NcVar> vars_map;     // variable name --> NcVar map (for data variables)
	std::map      <std::string, netCDF::NcVar> coords_map;   // variable name --> NcVar map (for coordinate variables)
	std::map      <std::string, std::string> coordunits_map;
	std::map      <std::string, std::vector<double>> coordvalues_map;

	inline void readMeta(){
		// get all variable in the file in a name --> variable map
		vars_map = this->getVars();
		
		std::map<std::string, netCDF::NcGroup> coords_map_temp = this->getCoordVars();
		// fill coords map from obtained variables
		for (auto p : coords_map_temp){
			coords_map[p.first] = p.second.getVar(p.first);
		} 

		// remove coord vars from all variables, so only data variables are left
		for (auto p : coords_map){
			vars_map.erase(p.first);
		}

		// read values and units for coordinate vars
		for (auto p : coords_map){
			std::string s;
			try{
				p.second.getAtt("units").getValues(s);
				coordunits_map[p.first] = s;
			}
			catch(...){
				std::cout << "Warning: variable " << p.first << " has no unit.";
				coordunits_map[p.first] = "";
			}

			std::vector<double> coordvals(p.second.getDim(0).getSize());
			p.second.getVar(coordvals.data());
			coordvalues_map[p.first] = coordvals;
		}

	}

	inline void printMeta(){
		std::cout << "NC file >" << "\n";
		std::cout << "   coords:\n";
		for (auto p : coords_map) std::cout << "      " << p.first << " (" << coordunits_map[p.first] << ")\n";
		std::cout << "   vars (excluding coords):\n";
		for (auto p : vars_map) std::cout << "      " << p.first << "\n";
		std::cout << "   coord values:\n";
		for (auto p : coordvalues_map){
			std::cout << "      " << p.first << ": ";
			if (p.second.size() <= 6) std::cout << p.second;
			else{
				std::cout << p.second.size() << " | " << p.second[0] << " " << p.second[1] << " " << p.second[2] << " ... " 
				          << p.second[p.second.size()-3] << " " << p.second[p.second.size()-2] << " " << p.second[p.second.size()-1] << "\n";
			}
		}
		std::cout << "~~\n";
	}
};


} // namespace flare

#endif
