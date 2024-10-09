#include <iostream>
#include <zip.h>
#include <string>

using namespace std;

int main() {
    // Open the zip file

    string zipFileName = "/data/shared_data/LOGAN_YACHT_microbe/ref_db/merged_GTDB_BVBRC_GenBank_ALL_k31.sig.zip";

    int err = 0;
    zip* zipFile = zip_open(zipFileName, 0, &err);
    if (zipFile == nullptr) {
        std::cerr << "Failed to open zip file." << std::endl;
        return 1;
    }

    // Get the number of files in the zip archive
    int numFiles = zip_get_num_entries(zipFile, 0);
    cout << "Number of files: " << numFiles << endl;

    // show name of first 10 files
    cout << "First 10 files: " << endl;
    for (int i = 0; i < 10; i++) {
        const char* name = zip_get_name(zipFile, i, 0);
        cout << name << endl;
    }

    // Close the zip file
    zip_close(zipFile);
    return 0;
}
