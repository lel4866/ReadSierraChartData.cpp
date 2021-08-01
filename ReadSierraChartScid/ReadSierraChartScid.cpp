// ReadSierraChartScid.cpp : Defines the entry point for the application.
//
#pragma warning( disable : 26812 26451 6201 6385 6386)
#include "SierraChartFiles/sierrachart.h"
#include "SierraChartFiles/scdatetime.h"
#pragma warning( default : 26812 26451 6201 6385 6386)

#include <fstream>
#include "ReadSierraChartScid.h"

using namespace std;

// C++20 test
template<typename T>
concept Addable = requires(T a, T b)
{
	a + b;
};
template<Addable T>
T f(T a, T b) {
	return a + b;
}

int main()
{
	const string datafile_dir{ "C:/SierraChart/Data/" };
	const string datafile_fn{ "ESU21-CME.scid" };
	const string datafile_fullfn = datafile_dir + datafile_fn;

	static_assert(sizeof(s_IntradayRecord) % sizeof(double) == 0, "s_IntradayRecord must be double aligned");

	// open file and position 
	ifstream datafile;
	datafile.open(datafile_fullfn, ios::in | ios::binary | ios::ate);
	if (!datafile.is_open()) {
		cout << "Unable to open data file: " << datafile_fullfn << endl;
		return -1;
	}

	// get file size
	size_t size = datafile.tellg();
	datafile.seekg(0, ios::beg); // reset pointer to beginning of file

	// read SierraChart scid file header
	s_IntradayFileHeader header;
	datafile.read((char*)&header, sizeof(header));
	if (datafile.bad()) {
		cout << "Unable to to read data file header: " << datafile_fullfn << endl;
		return -1;
	}
	size -= sizeof(header);

	// read SierraChart scid records
	if (size % sizeof(s_IntradayRecord) != 0) {
		// do not have even # of records
		cout << "Do not have even # of records: Space for records:" << size << endl;
		return -1;
	}
	size_t num_recs = size / sizeof(s_IntradayRecord);
	vector<s_IntradayRecord> records(num_recs);
	datafile.read((char*)records.data(), size);
#if 0
	size_t n{ 0 };
	for(; n<num_recs; n++) {
		datafile.read((char*)&records[n], sizeof(s_IntradayRecord));
		if (datafile.bad()) {
			cout << "Unable to to read data file header: " << datafile_fullfn << endl;
			return -1;
		}
	}
#endif

	// find first element on 9 June 6pm (18)
	s_IntradayRecord abc;
	static const SCDateTime xxx{ 2021, JUNE, 9, 22, 0, 0 };
	auto iterator = ranges::find_if(records, [](auto r)->bool {return r.DateTime >= xxx;});
	auto num_elements_to_remove = std::distance(begin(records), iterator);
	abc = *iterator;
	int yy, mm, dd, hh, minute, sec;
	abc.DateTime.GetDateTimeYMDHMS(yy, mm, dd, hh, minute, sec);
	records.erase(begin(records), iterator);
	auto c = f(4, 5);
	auto a = sizeof(s_IntradayFileHeader);
	cout << "Hello CMake." << endl;
	return 0;
}
