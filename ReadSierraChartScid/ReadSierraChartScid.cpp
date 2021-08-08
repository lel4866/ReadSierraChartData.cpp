// ReadSierraChartScid.cpp : Defines the entry point for the application.
//
#pragma warning( disable : 26812 26451 6201 6385 6386)
#include "SierraChartFiles/sierrachart.h"
#include "SierraChartFiles/scdatetime.h"
#pragma warning( default : 26812 26451 6201 6385 6386)

#include <ctime>
#include <filesystem>
#include <fstream>
#include "ReadSierraChartScid.h"

using namespace std;
static SCDateTime getUTCTimeFromLocalTime(int year, int month, int day, int hour, int min, int sec);

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
    const chrono::time_zone* curzone = chrono::current_zone();

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
        if (stem.length() < 5)
            continue;
        const char futures_code = toupper(stem[2]);
        if (!IsValidFuturesMonthCode(futures_code))
            continue;
        int end_month = GetMonthFromFuturesCode(futures_code);
        int start_month = end_month - 3;
        int start_year, end_year;
        const string futures_two_digit_year = stem.substr(3, 2);
        start_year = end_year = 2000 + atoi(futures_two_digit_year.c_str());
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

        // open file and position to end of file (ios::ate) so we can compute size
        ifstream datafile;
        datafile.open(path, ios::in | ios::binary | ios::ate);
        if (!datafile.is_open()) {
            cout << "Unable to open data file: " << path << endl;
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

        // create CSV file. Name is {futures_root }{futures_code}{2 digit year}.csv
        const string out_path = datafile_outdir + futures_root + futures_code + futures_two_digit_year + ".csv";
        ofstream csv_ostream(out_path);

        // output header
        csv_ostream << "Date,Time,Price" << endl;

         // only keep ticks between start_date and end_date
        SCDateTime start_dt = getUTCTimeFromLocalTime(start_year, start_month, 9, 18, 0, 0);
        SCDateTime end_dt = getUTCTimeFromLocalTime(end_year, end_month, 9, 18, 0, 0);

        int prev_date{ -1 };
        int prev_time{ -1 };
        s_IntradayRecord record;
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
            if (iTime > utc2030 and iTime < utc2200)
                continue;
            const int iDate = record.DateTime.GetDate();

            // only keep one tick per second
            // if this is first tick of new second, write it to output file
            if (iDate == prev_date and iTime == prev_time)
                continue;

            // write tick to CSV file
            int year{ 0 }, month{ 0 }, day{ 0 }, hour{ 0 }, minute{ 0 }, second{ 0 };
            record.DateTime.GetDateTimeYMDHMS(year, month, day, hour, minute, second);
            csv_ostream << format("{0:02}/{1:02}/{2},{3:02}:{4:02}:{5:02},{6:.2f}\n", month, day, year, hour, minute, second, record.Close);
            //csv_ostream << line << endl;

            prev_date = iDate;
            prev_time = iTime;
        }
        csv_ostream.close();

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

SCDateTime getUTCTimeFromLocalTime(int year, int month, int day, int hour, int min, int sec) {
    std::tm tm = {.tm_sec = sec, .tm_min  = min, .tm_hour = hour, .tm_mday = day, .tm_mon  = month, .tm_year = year-1900};
    tm.tm_isdst = -1; // Use DST value from local time zone
    std::time_t t = std::mktime(&tm);
    std::tm utc = *std::gmtime(&t);
    SCDateTime xxx(utc.tm_year, utc.tm_mon, utc.tm_mday, utc.tm_hour, utc.tm_min, utc.tm_sec);
    return SCDateTime(utc.tm_year, utc.tm_mon, utc.tm_mday, utc.tm_hour, utc.tm_min, utc.tm_sec);
}

