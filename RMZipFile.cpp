#include "RMZipFile.h"
using namespace std;

RMZipFile::RMZipFile()
{
}

int RMZipFile::ExtractRMsFromZip(const char* FileName)
{
    int NumPages = 0;
    zip* archive = zip_open(FileName, 0, NULL);
    // Step 2: Get the total number of files in the zip
    // archive
    zip_int64_t numFiles = zip_get_num_entries(archive, 0);

    // Step 3: Loop through each file and print its contents
    for (int ZipIndex = 0; ZipIndex < numFiles; ++ZipIndex) {
        struct zip_stat fileInfo;
        zip_stat_init(&fileInfo);

        if (zip_stat_index(archive, ZipIndex, 0, &fileInfo) == 0) {
            // see what sort of file it is, and if we want to try and parse it
            DoLog(fileInfo.name);
            size_t i;
            for (i = strlen(fileInfo.name); i > 0; i--)
                if (fileInfo.name[i] == '.') 
                    break;
                        
            if (i > 0) {
                if (!strcmp(&fileInfo.name[i], ".rm")) 
                {
                    // Step 4: Extract and print file contents
                    RMPage* Page = new RMPage();
                    Page->Load(zip_fopen_index(archive, ZipIndex, 0));
                    Pages.push_back(Page);
                    NumPages++;
                }

            }

        }
    }

    // Close the zip archive
    zip_close(archive);
    DoLog("Done!");

    return NumPages;
}

void RMZipFile::DrawPage(HDC hDC, int Page)
{
    RMPage* P = Pages[Page];
    P->DrawPage(hDC);
}