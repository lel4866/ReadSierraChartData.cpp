// ReadSierraChartScid.cpp : Defines the entry point for the application.
//
#pragma warning( disable : 26812 26451 6201 6385 6386)
#include "SierraChartFiles/sierrachart.h"
#include "SierraChartFiles/scdatetime.h"
#pragma warning( default : 26812 26451 6201 6385 6386)

#include <ctime>
#include <istream>
#include <ostream>
#include <fstream>
#include <thread>

#include "ZLibCompressor.h"
#include "ReadSierraChartScid.h"

using namespace std;


void processScidFile(const string futures_root, const filesystem::path path, const string& datafile_outdir);
std::tm* getLocalTimeFromtSCDateTime(const SCDateTime& utcTime);
static SCDateTime getSCDateTimeFromLocalTime(int year, int month, int day, int hour, int min, int sec);

int main()
{
    const string datafile_dir{ "C:/SierraChart/Data/" };
    const string datafile_outdir{ "C:/Users/lel48/SierraChartData/" };
    const string futures_root{ "ES" };
    const chrono::time_zone* curzone = chrono::current_zone();

    // get .scid files in datafile_dir
    vector<thread> threads;
    for (const filesystem::directory_entry& dir_entry : filesystem::directory_iterator(datafile_dir)) {
        if (dir_entry.is_directory())
            continue;
        const filesystem::path path = dir_entry.path();
        if (path.extension() != ".scid")
            continue;

        // filename must start with futures_root and be 5 or more chars in length
        const string stem(path.stem().string());
        if (!stem.starts_with(futures_root))
            continue;
        if (stem.length() < 5)
            continue;

        // start thread to process an .scid file
        cout << "processing: " << path.filename() << endl;
        threads.emplace_back(processScidFile, futures_root, path, datafile_outdir);

        // debug - just do 1 file
        // break;
    }

    // wait for all threads to complete
    for (auto&& thread : threads)
        thread.join();

    return 0;
}

void processScidFile(const string futures_root, const filesystem::path path, const string& datafile_outdir) {
    const string stem(path.stem().string());
    const char futures_code = toupper(stem[2]);
    if (!IsValidFuturesMonthCode(futures_code))
        return;
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
        return;
    }

    // get file size
    size_t size = datafile.tellg();
    datafile.seekg(0, ios::beg); // reset pointer to beginning of file

        // read SierraChart .scid file header
    s_IntradayFileHeader header;
    datafile.read((char*)&header, sizeof(header));
    if (datafile.bad()) {
        cout << "Unable to to read data file header: " << path.filename() << endl;
        return;
    }
    size -= sizeof(header); // set size to just that used for data

    if (size % sizeof(s_IntradayRecord) != 0) {
        cout << "Data set: " << path.filename() << " does not have even number of data records." << endl;
        return;
    }
    const size_t num_recs = size / sizeof(s_IntradayRecord);

    // create ,z file. Name is {futures_root }{futures_code}{2 digit year}.z
    const string out_path = datafile_outdir + futures_root + futures_code + futures_two_digit_year + ".z";

    ZlibCompressor zlc(out_path);

    // output header
    string csv_header{ "Date,Time,Price\n" };
    zlc.save_line_compressed(csv_header);

    // only keep ticks between start_date and end_date
    SCDateTime start_dt = getSCDateTimeFromLocalTime(start_year, start_month, 9, 18, 0, 0);
    SCDateTime end_dt = getSCDateTimeFromLocalTime(end_year, end_month, 9, 18, 0, 0);

    int prev_date{ -1 };
    int prev_time{ -1 };
    s_IntradayRecord record;
    for (int i = 0; i < num_recs; i++) {
        datafile.read((char*)&record, sizeof(s_IntradayRecord));
        if (datafile.bad()) {
            cout << "Unable to to read data file record: " << i << endl;
            return;
        }

        // throw away data earlier than start date/time
        if (record.DateTime < start_dt)
            continue;

        // terminate loop on first data later than end date/time (assumes data is sorted)
        if (record.DateTime >= end_dt)
            break;


        // throw away Saturdays
        std::tm* localTime = getLocalTimeFromtSCDateTime(record.DateTime); // get local time of current record
        if (localTime->tm_wday == 6)
            continue;

        // throw away data between 4:30pm and 6pm ET
        if ((localTime->tm_hour == 16 && localTime->tm_min >= 30) or localTime->tm_hour == 17)
            continue;

        const int iDate = record.DateTime.GetDate();
        const int iTime = record.DateTime.GetTimeInSeconds();

        // only keep one tick per second
        // if this is first tick of new second, write it to output file
        if (iDate == prev_date and iTime == prev_time)
            continue;

        // write tick to CSV file
        string csv = format("{0:02}/{1:02}/{2},{3:02}:{4:02}:{5:02},{6:.2f}\n",
            localTime->tm_mon + 1, localTime->tm_mday + 1, localTime->tm_year + 1900, localTime->tm_hour, localTime->tm_min, localTime->tm_sec, record.Close);

        zlc.save_line_compressed(csv);

        prev_date = iDate;
        prev_time = iTime;
    }
}

// SCDateTime is UTC time
SCDateTime getSCDateTimeFromLocalTime(int year, int month, int day, int hour, int min, int sec) {
    std::tm stm = {.tm_sec = sec, .tm_min  = min, .tm_hour = hour, .tm_mday = day, .tm_mon  = month-1, .tm_year = year-1900};
    stm.tm_isdst = -1; // Use DST value from local time zone
    std::time_t t = std::mktime(&stm);
    std::tm* utc = std::gmtime(&t);
    SCDateTime xxx(utc->tm_year+1900, utc->tm_mon+1, utc->tm_mday+1, utc->tm_hour, utc->tm_min, utc->tm_sec);
    return SCDateTime(utc->tm_year+1900, utc->tm_mon+1, utc->tm_mday+1, utc->tm_hour, utc->tm_min, utc->tm_sec);
}

// SCDateTime is UTC time
std::tm* getLocalTimeFromtSCDateTime(const SCDateTime& utcTime) {
    time_t t = utcTime.ToUNIXTime();
    return std::localtime(&t);
}


