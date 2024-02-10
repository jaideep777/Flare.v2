#ifndef FLARE_FLARE_CSVSTREAM_H
#define FLARE_FLARE_CSVSTREAM_H

#include <fstream>
#include <sstream>
#include <queue>
#include "stream.h"

namespace flare{

// CSV reader code adapted from the accepted answer to this post: 
// https://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c
class CSVRow{
	private:
	std::string       m_line;
	std::vector<int>  m_data;

	public:
	std::string operator[](std::size_t index) const {
		std::string s(&m_line[m_data[index] + 1], m_data[index + 1] -  (m_data[index] + 1));
		s.erase(remove( s.begin(), s.end(), '\"' ),s.end()); // remove quotes from string
		return s;
	}

	std::size_t size() const {
		return m_data.size() - 1;
	}

	void readNextRow(std::istream& str){
		std::getline(str, m_line);
		m_line.erase(remove( m_line.begin(), m_line.end(), '\r' ), m_line.end()); // remove cariage return from string, if present

		m_data.clear();
		m_data.emplace_back(-1);
		std::string::size_type pos = 0;
		while((pos = m_line.find(',', pos)) != std::string::npos){
			m_data.emplace_back(pos);
			++pos;
		}
		// This checks for a trailing comma with no data after it.
		pos   = m_line.size();
		m_data.emplace_back(pos);
	}

	std::string get_line_raw(){
		return m_line;
	}
};


std::istream& operator>>(std::istream& str, CSVRow& data){
    data.readNextRow(str);
    return str;
}   


class CsvStream : public Stream{
	private:
	int t_id = -1;
	StreamIndex current_index;
	std::ifstream csvin;

	public:
	std::vector<std::string> colnames;
	CSVRow current_row;

	public:

	inline void print_meta() override{
		Stream::print_meta();
		std::cout << "   colnames: " << colnames << '\n';
		std::cout << "   t_id: " << t_id << '\n';
		std::cout << "   current_idx: " << current_index.idx << " / " << current_index.f_idx << "." << current_index.t_idx << '\n';
	}

	inline void reset() override{
		Stream::reset();
		current_index.set(0,0,0);
		t_id = -1;
		colnames.clear();
	}

	/// @brief Specializatin of Stream::open() for NetCDF files
	/// @param _filenames list of files to stream from
	inline void open(std::vector<std::string> _filenames, std::string _tunit_str){
		reset();

		filenames = _filenames;
		std::ifstream fin;
		
		// To construct the full time vector, we need to open each file once and obtain its time vector
		for (size_t i_file = 0; i_file<filenames.size(); ++i_file){
			std::string fname = filenames[i_file];
			fin.open(fname.c_str());
			if (!fin) throw std::runtime_error("Could not open file: "+fname);

			// read header
			CSVRow row;
			fin >> row;
			// store header only from 1st row
			if (i_file == 0){
				for (int i=0; i<row.size(); ++i){
					colnames.push_back(row[i]);
				}
				// std::cout << colnames << std::endl;
			}

			// get index of time column (name comparisons are case insensitive)
			for (size_t icol=0; icol<colnames.size(); ++icol){
				std::string col = colnames[icol];
				std::transform(col.begin(), col.end(), col.begin(),
				       [](unsigned char c){ return std::tolower(c); });
				for (auto name : tnames){
					std::transform(name.begin(), name.end(), name.begin(),
						[](unsigned char c){ return std::tolower(c); });

					if (col == name) t_id = icol;
				}
			}
			if (t_id < 0) throw std::runtime_error("Cannot find time column in CSV file: "+fname);

			// read time column for all rows
			int line_num = 0;
			while(fin >> row){
				times.push_back(std::stod(row[t_id]));
				file_indices.push_back(i_file);
				t_indices.push_back(line_num);
				++line_num;
			}

			fin.close();
		}

		// Parse time unit provided
		unit_str = _tunit_str;
		// std::cout << unit_str << std::endl;
		parse_time_unit(unit_str); // sets tunit, tscale, t_base

		if (!std::is_sorted(times.begin(), times.end()))  throw std::runtime_error("NcStream: Combined time vector is not in ascending order. Check the order of files supplied.\n");

		for (auto& t : times) t *= tscale; // convert time vector to "days since base date"

		if (times.size() > 0){
			tstep = (times.back() - times.front())/(times.size()-1); // get timestep in days
			DeltaT = times[times.size()-1] - times[0] + tstep;
		}

		// open first file
		update_file(0);

	}

	void update_file(size_t file_id){
		if (current_index.f_idx == file_id) return; // no need to do anything if file has not changed
		
		csvin.close();
		csvin.open(filenames[file_id]);
		if (!csvin) throw std::runtime_error("Could not open file: "+filenames[file_id]);

		current_index.set(file_id, 0, 0);
	}

	void advance_to_time(double j, bool periodic, bool centered_t){
		StreamIndex new_idx = julian_to_indices(j, periodic, centered_t);
		update_file(new_idx.f_idx);

		std::string line;
		csvin >> current_row; // skip header
		for (int i=0; i<=new_idx.t_idx; ++i){
			csvin >> current_row; // skip t_idx-1 lines so that next line will be desired index
		}
		current_index = new_idx;
	}
};


} // namespace flare

#endif
