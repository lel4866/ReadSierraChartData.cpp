# ReadSierraChartSCID
Read Sierra Chart(tm) SCID stock/futures data files

This program reads Sierra Chart .scid files from C:/SierraChart/Data directory and writes filtered CSV files to the local SierraChartData directory.

The data directory is saved in my SierraChartData repository on GitHub. Unlike each .scid file, which contains tick data for the entire contract, 
the CSV files only contain at most 1 tick per second starting from the 2200 UTC on the 9th of the first active month to 2200 UTC on the 9th of the expiration month,
a total of 3 months.

Each file name is of the form {futures prefix}{futures month code}{2 digit year}.scid
Each file has tick data for the entire contract. It is a binary file whose format is specified in Sierra Chart documentation.
Basically each file consists of a header and a number of data records. I have copied the header files from the Sierra Chart directory.


