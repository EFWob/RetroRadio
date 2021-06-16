#include <genres.h>
#include <AddRR.h>
#include <LITTLEFS.h> 
//#undef BOARD_HAS_PSRAM
Genres genres;

extern void doprint ( const char* format, ... );

void Genres::dbgprint ( const char* format, ... ) {
#if !defined(NOSERIAL)
    if (_verbose)
    {
        static char sbuf[2 * DEBUG_BUFFER_SIZE] ;                // For debug lines
        va_list varArgs ;                                    // For variable number of params
        sbuf[0] = 0 ;
        va_start ( varArgs, format ) ;                       // Prepare parameters
        vsnprintf ( sbuf, sizeof(sbuf), format, varArgs ) ;  // Format the message
        va_end ( varArgs ) ;                                 // End of using parameters
        if ( DEBUG )                                         // DEBUG on?
        {
            Serial.print ( "GENRE: " ) ;                           // Yes, print prefix
        }
        Serial.println ( sbuf ) ;                            // always print the info
        Serial.flush();
    }
 #endif
}

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
    return _begun;
}       


bool Genres::openGenreIndex() {
bool res = LITTLEFS.exists("/genres");
#if defined(BOARD_HAS_PSRAM)
    if (NULL != _psramList)
        free(_psramList);
    _psramList = NULL;
#endif
    memset(_idx, 0, sizeof(_idx));
    _knownGenres = 0;
    if (!res)
    {
        dbgprint("Creating LITTLEFS directory '/genres'");
        res = LITTLEFS.mkdir("/genres");
    }
    if (res)
    {
        dbgprint("LITTLEFS success opening directory '/genres'");
        File idxFile = LITTLEFS.open("/genres/list");
        if (idxFile.size() > 0)
        {
#if defined(BOARD_HAS_PSRAM)
            _psramList = (uint8_t *)ps_malloc(idxFile.size());
            if (_psramList != NULL)
            {
                idxFile.read(_psramList, idxFile.size());
                idxFile.seek(0);
            }
#endif            
            size_t i = 0;
            int idx = 0;
            while (i < idxFile.size() && (idx < MAX_GENRES))
            {
                size_t skip; 
                idxFile.seek(i);
                skip = (size_t)idxFile.read();
                _idx[idx++].idx = i++;
                if (skip >= 0)
                    i = i + skip;
            }
            _knownGenres = idx;
        }
        else
            dbgprint("ListOfGenres file has size 0");
        idxFile.close();
    }
    else
    {
        dbgprint("LITTLEFS could not create directory '/genres'");
    }
    return res;
}

String Genres::getName(int id)
{
    char genreName[256];
    genreName[0] = 0;
    if (_begun && (id > 0) && (id <= _knownGenres))
    {
        id--;
        size_t idx = _idx[id].idx;
        uint16_t l = 0;
        if (_psramList)
        {   
            l = _psramList[idx];
            if (l < 256)
                memcpy(genreName, _psramList + idx + 1, l);
        }
        else
        {
            File file = LITTLEFS.open("/genres/list", "r");
            file.seek(idx);
            l = file.read();
            if (l < 256)
                file.read((uint8_t *)genreName, l);
        }
        if (l < 256)
            genreName[l] = 0;
    }
    return String(genreName);
}

int Genres::findGenre(const char *s)
{
int res = -1;
int len = strlen(s);
    if (len > 255)
        len = 255;
    if (_begun && (len>0))
    {
        File idxFile;
        if (!_psramList)    
            idxFile = LITTLEFS.open("/genres/list");
        res = 0;
        size_t idx = 0;
        for (int i = 0;i < _knownGenres;i++)
        {
            int l;
            if (_psramList) 
            {
                l = _psramList[idx++];
            }
            else
            {
                l = idxFile.seek(idx++);
                l = idxFile.read();
            }
            if (l == len)
            {
                uint8_t genreName[l + 1];
                if (_psramList)
                    memcpy(genreName, _psramList + idx, l);
                else
                    idxFile.read(genreName, l);
                genreName[l] = 0;
                if (0 == strcmp(s, (char *)genreName))
                {
                    res = i + 1;
                    break;
                }
            }
            idx = idx + l;
        }
        idxFile.close();
    }
    return res;
}

int Genres::createGenre(const char *s)
{
int res = findGenre(s);
    if (res < 0)
        res = 0;
    else if (res > 0)
        cleanGenre(res);
    else if (_knownGenres < MAX_GENRES)
    {
        File idxFile = LITTLEFS.open("/genres/list", "a");
        if (!idxFile)
        {
            dbgprint("Could not open /genres/list for adding");
            res = 0;
        
        }
        else
        {
            int l = strlen(s);
            if (l > 255)
                l = 255;
            dbgprint("Adding to idxFile, current size is %d", idxFile.size());
            idxFile.write((uint8_t)l);
            idxFile.write((const uint8_t *)s, l);
            idxFile.close();
            _knownGenres++;
            if (openGenreIndex())
            {
                res = findGenre(s);
                cleanGenre(res);
            }
        }
    }
    else
        res = 0;
    return res;
}


bool Genres::add(int id, const char *s) {
    bool res = true;
    dbgprint("Request to add URL '%s' to Genre with id=%d", (s?s:"<NULL>"), id);
    if (id != _addGenre)
    {
        dbgprint("First stopping add to genre %d", _addGenre);
        stopAdd();
        if (!startAdd(id))
        {
            dbgprint("Could not start adding to genre %d", id);
            res = false;
        }
    }
    if (res)
        if (!s)
        {
            dbgprint("Can not add NULL-String!");
            res = false;
        }
        else if (!strlen(s))
        {
            dbgprint("Can not add empty URL-String!");
            res = false;
        }
    if (!res)
        return false;
    char path[50];
    sprintf(path, "/genres/%d/urls", _addGenre);
    File urlFile = LITTLEFS.open(path, "a");
    if (!urlFile)
    {
        dbgprint("Can not open file '%s' to append URL '%s'", path, s);
        return false;
    }
    _addIdx = _addIdx + strlen(s) + 1;
    urlFile.write((const uint8_t *)s, strlen(s) + 1);
    urlFile.close();
    _addCount++;
    if ((_addCount % URL_CHUNKSIZE) == 0)
    {
        File idxFile;
        sprintf(path, "/genres/%d/idx", _addGenre);
        idxFile = LITTLEFS.open(path, "a");
        idxFile.write((uint8_t)URL_CHUNKSIZE);
        idxFile.write((const uint8_t *)&_addIdx, sizeof(_addIdx));
        idxFile.close();
        _addCount = 0;
    }
    return true;
}

void Genres::stopAdd() {
    if (_addGenre)
    {
        char path[30];
        char fileName[50];
        sprintf(path, "/genres/%d", _addGenre);
        sprintf(fileName, "%s/idx", path);
        if ((_addCount == 0) || ((_addCount % URL_CHUNKSIZE) != 0))
        {
            File idxFile = LITTLEFS.open(fileName, "a");
            idxFile.write((uint8_t)(_addCount % URL_CHUNKSIZE));
            idxFile.close();
        }
        _addGenre = 0;
    }
}

void Genres::ls() {
    listDir("/genres");    
}

void Genres::test() {
    doprint("Genre Filesystem total bytes: %ld, used bytes: %ld (free=%ld)",
        LITTLEFS.totalBytes(), LITTLEFS.usedBytes(), LITTLEFS.totalBytes() - LITTLEFS.usedBytes());
}

void Genres::listDir(const char * dirname){
    doprint("Listing directory: %s", dirname);

    File root = LITTLEFS.open(dirname);
    if(!root)
    {
        doprint("- failed to open directory");
        return;
    }
    if(!root.isDirectory())
    {
        doprint(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file)
    {
        if(file.isDirectory())
        {
            char *s;
            if (s = strstr(file.name(), "/genres/"))
                s = s + 8;
            else
                s = "0";
            doprint("  DIR : %s (GenreName[%s]: %s)", file.name(), s, getName(atoi(s)));
            listDir(file.name());    
        } 
        else 
        {
            doprint("  FILE: %s\tSize: %ld", file.name(), file.size());
        }
        file = root.openNextFile();
    }
    doprint("Done listing directory: %s", dirname);
}


uint16_t Genres::count(int id) {
uint32_t res = 0;
    if ((id > 0) && (id <= _knownGenres))
    {
        char path[30];
        sprintf(path, "/genres/%d/idx", id);
        File file = LITTLEFS.open(path, "r");
        if (file)
        {
            size_t fSize = file.size();
            res = ((fSize - 4) / 5 ) * URL_CHUNKSIZE;
            if (id == _addGenre)
            {
                dbgprint("Currently adding to Genre with id=%d, addCount=%d, fSize=%ld, res=%d", id, _addCount, fSize, res);
                if ((fSize % 5) == 4)
                {
                    res = res + _addCount;
                }
                else
                    res = 0;
            }
            else 
                if ((fSize % 5) == 0)
                {
                    file.seek(fSize - 1);
                    res = res + file.read();
                }
                else
                    res = 0;
            file.close();
        }
    }
    if (res > 0xffff)
        res = 0;
    return res;
}

String Genres::getUrl(int id, uint16_t number) {
String res = "";
    if (count(id) > number)
    {
        char path[20];
        char fileNameIdx[30], fileNameUrls[30];
        File fileIdx, fileUrls;
        sprintf(path, "/genres/%d", id);
        sprintf(fileNameIdx, "%s/idx", path);
        sprintf(fileNameUrls, "%s/urls", path);
        fileIdx = LITTLEFS.open(fileNameIdx, "r");
        fileUrls = LITTLEFS.open(fileNameUrls, "r");
        if (fileIdx && fileUrls)
        {
            size_t seekPosition = (number / URL_CHUNKSIZE) * 5;
            size_t chunkStart;
            size_t chunkSize;
            fileIdx.seek(seekPosition);
            fileIdx.read((uint8_t *)&chunkStart, 4);
            if (seekPosition + 5 < fileIdx.size())
            {
                fileIdx.seek(seekPosition + 5);
                fileIdx.read((uint8_t *)&chunkSize, 4);
            }
            else
                chunkSize = fileUrls.size();
            // chunkSize now contains the start index of next chunk, make it chunkSize with next expression
            number = number % URL_CHUNKSIZE;
            chunkSize = chunkSize - chunkStart;
            dbgprint("Ready to locate url in file, chunkStart=%ld, chunkSize=%ld, idx=%d", chunkStart, chunkSize, number);
            char *s = (char *)malloc(chunkSize);
            if (s)
            {
                fileUrls.seek(chunkStart);
                fileUrls.read((uint8_t *)s, chunkSize);
                char *p = s;
                while (number)
                {
                    number--;
                    p = p + strlen(p) + 1;
                }
                res = String(p);
                free(s);
            }
        }
        fileIdx.close();
        fileUrls.close();
    }
    return res;
}

bool Genres::startAdd(int id) {
bool res = false;
    if ((id > 0) && (id <= _knownGenres))
    {
        char path[30];
        char fileName[50], fileName2[50];
        File idxfile, file2;
        //cleanGenre(id);
        sprintf(path, "/genres/%d", id);
        sprintf(fileName, "%s/idx", path);
        sprintf(fileName2, "%s/new", path);
        idxfile = LITTLEFS.open(fileName, "r");
        file2 = LITTLEFS.open(fileName2, "w");
        size_t l = idxfile.size();
        if (idxfile && file2 && ((l % 5) == 0) && (l >= 5))
        {
            uint32_t knownUrls = ((l / 5) - 1) * URL_CHUNKSIZE;
            size_t l1 = l - 1;
            size_t bufsize = l1>2000?2000:l1;
            uint8_t buf[bufsize];
            while (l1 > 0)
            {
                size_t chunksize = (l1 > bufsize)?bufsize:l1;
                idxfile.read(buf, chunksize);
                file2.write(buf, chunksize);
                l1 = l1 - chunksize;
            }
            _addCount = idxfile.read();
            knownUrls = knownUrls + _addCount;
            idxfile.close();
            file2.close();
            if (LITTLEFS.rename(fileName2, fileName))
            {
                sprintf(fileName, "%s/urls", path);
                file2 = LITTLEFS.open(fileName, "r");
                if (file2)
                {
                    _addIdx = file2.size();
                    _addGenre = id;
                    file2.close();
                    res = true;
                    dbgprint("Success to start add to Genre with id: %d", id);
                    dbgprint("URLs so far: %ld, URL filesize=%ld, subIndex: %d", knownUrls, _addIdx, _addCount);
                }
            }
            else
                dbgprint("Error: could not create new indexfile for genre with id: %d", id);
        }
        else 
            dbgprint("Error: could not start adding to genre with id: %d. Filesystem corrupt!?", id);    
/*        
        sprintf(path, "/genres/%d", _addGenre);
        sprintf(fileName, "%s/idx", path);
        if (!LITTLEFS.mkdir(path))
        {
            dbgprint("Could not create directory '%s'", path);
            return false;
        }
        File idxFile = LITTLEFS.open(fileName, "w");
        dbgprint("Creating index file '%s'=%d", fileName, (bool)idxFile);
        if (idxFile)
        {
            idxFile.write((const uint8_t *)&_addIdx, sizeof(_addIdx));
            idxFile.close();
            if (_verbose)
            {
                idxFile = LITTLEFS.open(fileName, "r");
                dbgprint("Created idxFile with first index entry(0), size is: %d", idxFile.size());
                idxFile.close();
            }
            sprintf(fileName, "%s/urls", path);
            File urlFile = LITTLEFS.open(fileName, "w");
            dbgprint("Creating url file '%s'=%d", fileName, (bool)urlFile);
            if (res = urlFile)
                urlFile.close();
        }
*/
    }
    if (!res)
        _addGenre = 0;
    return res;
}

void Genres::cleanGenre(int id)
{
    if (_begun)
    {
    char s0[20];
    char s[50];
        //TODO delete specific files for Genre
        sprintf(s0, "/genres/%d", id);
        File root = LITTLEFS.open(s0);
        if (!root) {
            dbgprint("No content is stored in LITTLEFS for genre with id: %d", id);
            dbgprint("Creating directory %s", s0);
            LITTLEFS.mkdir(s0);
        }
        else if (!root.isDirectory()) {
            dbgprint("PANIC because unexpected: File with name %s is not a directory!", s0);
            root.close();
            return;
        }
        else
        {
            File file = root.openNextFile();
            while (file)
            {
                char *s = strdup(file.name());
                file = root.openNextFile();
                dbgprint("Removing '%s'=%d", s, LITTLEFS.remove(s));
                free(s);
            }        
            //dbgprint("Removing directory '%s'=%d", s0, LITTLEFS.rmdir(s0));
            root.close();
        }
        root = LITTLEFS.open(s0); // Directory should be open now..
        if (root && (root.isDirectory()))
        {

            sprintf(s, "%s/idx", s0);
            File file = LITTLEFS.open(s, "w");
            if (file)
            {
                uint8_t buf[5];
                memset(buf, 0, 5);
                file.write(buf, 5);
                file.close();
            }
            sprintf(s, "%s/urls", s0);
            file = LITTLEFS.open(s, "w");
            if (file)
            {
                file.close();
            }
        }
        else
        {
            dbgprint("PANIC because unexpected: Directory with name %s not found!", s0);
        }
        root.close();

    }
    else 
        dbgprint("LITTLEFS is not mounted (for deleting genre with id: %d", id);
}

void Genres::format()
{
    if (_begun)
    {
        LITTLEFS.format();
        ESP.restart();
    }
}
