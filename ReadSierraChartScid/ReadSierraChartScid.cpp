// ReadSierraChartScid.cpp : Defines the entry point for the application.
//
#pragma warning( disable : 26812 26451 6201 6385 6386)
#include "SierraChartFiles/sierrachart.h"
#include "SierraChartFiles/scdatetime.h"
#pragma warning( default : 26812 26451 6201 6385 6386)

#include <filesystem>
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
    const string datafile_outdir{ "C:/Users/lel48/SierraChartData/" };
    const string datafile_fn{ "ESU21-CME.scid" };
    const string futures_root{ "ES" };

    // get .scid files in datafile_dir
    for (const filesystem::directory_entry& dir_entry : filesystem::directory_iterator(datafile_dir)) {
        if (dir_entry.is_directory())
            continue;
        const filesystem::path path = dir_entry.path();
        if (path.extension() != ".scid")
            continue;

        // get start month, end month, start year, end year from futures file name
        const string stem(path.stem().string());
        if (!stem.starts_with(futures_root))
            continue;
        if (stem.length() != 5)
            continue;
        const char futures_code = stem[2];
        if (!IsValidFuturesMonthCode(futures_code))
            continue;
        int start_month = JANUARY;
        int end_month = GetMonthFromFuturesCode(futures_code);
        int start_year, end_year;
        start_year = end_year = 2000 + atoi(stem.c_str() + 3);
        switch (futures_code) {
        case 'H':
            start_month = 12;
            start_year = end_year - 1;
            break;
        case 'Z':
            end_month = 3;
            end_year++;
            break;
        }

        // open file and position 
        ifstream datafile;
        datafile.open(path.filename().string(), ios::in | ios::binary | ios::ate);
        if (!datafile.is_open()) {
            cout << "Unable to open data file: " << path.filename() << endl;
            return -1;
        }

        // get file size
        size_t size = datafile.tellg();
        datafile.seekg(0, ios::beg); // reset pointer to beginning of file

        // read SierraChart .scid file header
        s_IntradayFileHeader header;
        datafile.read((char*)&header, sizeof(header));
        if (datafile.bad()) {
            cout << "Unable to to read data file header: " << path.filename() << endl;
            return -1;
        }
        size -= sizeof(header); // set size to just that used for data

        if (size % sizeof(s_IntradayRecord) != 0) {
            cout << "Data set: " << path.filename() << " does not have even number of data records." << endl;
            return -1;
        }
        const size_t num_recs = size / sizeof(s_IntradayRecord);

        s_IntradayRecord record;
        const SCDateTime start_dt{ start_year, start_month, 9, 22, 0, 0 };
        const SCDateTime end_dt{ end_year, end_month, 9, 22, 0, 0 };

        int prev_date{ -1 };
        int prev_time{ -1 };
        for (int i = 0; i < num_recs; i++) {
            datafile.read((char*)&record, sizeof(s_IntradayRecord));
            if (datafile.bad()) {
                cout << "Unable to to read data file record: " << i << endl;
                return -1;
            }
            if (record.DateTime < start_dt)
                continue;
            if (record.DateTime >= end_dt)
                break;

            // throw away data between 4:30pm (8:30p UTC, 2030 UTC) and 6pm
            constexpr int utc2030{ 21 * 60 * 60 + 30 * 60 };
            constexpr int utc2200{ 21 * 60 * 60 + 30 * 60 };
            const int iTime = record.DateTime.GetTimeInSeconds();
            if (iTime > utc2030 or iTime < utc2200)
                continue;
            const int iDate = record.DateTime.GetDate();

            // if this is first tick of new second, write it to output file
            if (iDate == prev_date and iTime == prev_time)
                continue;

            prev_date = iDate;
            prev_time = iTime;
        }

#if 0
        size_t n{ 0 };
        for (; n < num_recs; n++) {
            datafile.read((char*)&records[n], sizeof(s_IntradayRecord));
            if (datafile.bad()) {
                cout << "Unable to to read data file header: " << datafile_fullfn << endl;
                return -1;
            }
        }
#endif
    }

    return 0;
}
