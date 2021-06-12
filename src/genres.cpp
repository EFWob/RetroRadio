#include <genres.h>
#include <AddRR.h>
#include <LITTLEFS.h> 

Genres genres;

bool Genres::begin() {
    if (_begun)                 // Already open?
        return true;
    if (!LITTLEFS.begin())
    {
        dbgprint("LITTLEFS Mount failed: try to format, that will take some time!");
        if (!LITTLEFS.format())
        {
            dbgprint("LITTLEFS formatting failed! Can not play from genres!");
            return false;
        }
        if (!LITTLEFS.begin())
        {
            dbgprint("LITTLEFS formatting was OK, but begin() failed! Can not play from genres!");            
            return false;
        }
    }
    _begun = openGenreIndex();
}       


bool Genres::openGenreIndex() {
bool res = LITTLEFS.exists("/genres");
    if (!res)
    {
        dbgprint("Creating LITTLEFS directory '/genres'");
        res = LITTLEFS.mkdir("/genres");
    }
    if (res)
    {
        dbgprint("LITTLEFS success opening directory '/genres'");
    }
    else
    {
        dbgprint("LITTLEFS could not create directory '/genres'");
    }
    return res;
}
