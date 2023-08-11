#ifndef FLARE_FLARE_NCSTREAM_H
#define FLARE_FLARE_NCSTREAM_H

#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <netcdf>
#include <chrono>
#include <cmath>

#include "ncfilepp.h"

namespace flare{


class NcStream : public NcFilePP{
	public:
	std::vector<std::string> filenames;

	private:
	size_t current_file_id;
	double current_time;
	std::vector<double> times;

	public:

	inline void open(std::vector<std::string> _filenames){
		filenames = _filenames;

		// To construct the time vector, we need to open each file once and obtain its time vector

	}

	inline void printMeta(){
	}
};


} // namespace flare

#endif
