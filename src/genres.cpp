#include <genres.h>
#include <AddRR.h>
#include <LITTLEFS.h> 
//#undef BOARD_HAS_PSRAM
Genres genres;

extern void doprint ( const char* format, ... );


String GenreConfig::asJson() {
  String ret;
  const char *host = _rdbs?_rdbs:"de";
  ret = String(host);
  if ((0 == strcmp(host,"de")) || (0 == strcmp(host,"fr")) || (0 == strcmp(host,"nl")))
      ret = ret + "1.api.radio-browser.info";
  return  "{\"rdbs\": \"" + ret + "\"" +
              ",\"noname\": " + String(_noNames?1:0) +
              ",\"showid\": " + String(_showId?1:0) +
               "}";
}


void Genres::dbgprint ( const char* format, ... ) {
#if !defined(NOSERIAL)
    if (config.verbose())
    {
        static char sbuf[2 * DEBUG_BUFFER_SIZE] ;                // For debug lines
        va_list varArgs ;                                    // For variable number of params
        sbuf[0] = 0 ;
        va_start ( varArgs, format ) ;                       // Prepare parameters
        vsnprintf ( sbuf, sizeof(sbuf), format, varArgs ) ;  // Format the message
        va_end ( varArgs ) ;                                 // End of using parameters
        if ( DEBUG )                                         // DEBUG on?
        {
            Serial.print ( "D: GENRE: " ) ;                           // Yes, print prefix
        }
        else    
            Serial.print ("GENRE: ");
        Serial.println ( sbuf ) ;                            // always print the info
        Serial.flush();
    }
 #endif
}

/*
void Genres::verbose ( int mode ) {
#if !defined(NOSERIAL)
    _verbose = mode;
 #endif
}
*/

bool Genres::begin() {
    if (_begun)                 // Already open?
        return true;
    if (!LITTLEFS.begin())
    {
        if (!(_begun = this->format()))
        {
            dbgprint("LITTLEFS formatting failed! Can not play from genres!");
            return false;
        }
    }
    else
        _begun = openGenreIndex();
    return _begun;
}       


bool Genres::openGenreIndex() {
bool res = LITTLEFS.exists("/genres");
    if (NULL != _psramList)
        free(_psramList);
    _psramList = NULL;
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
#else
            if (ESP.getFreeHeap() > idxFile.size())
                if (ESP.getFreeHeap() - idxFile.size() > 80000)
                    _psramList = (uint8_t *)ps_malloc(idxFile.size());
#endif            
            if (_psramList != NULL)
            {
                idxFile.read(_psramList, idxFile.size());
                idxFile.seek(0);
            }
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
            {
                memcpy(genreName, _psramList + idx + 1, l);
                genreName[l] =  0;
            }
//            dbgprint("Got name from PSRAMLIST: %s", genreName);
        }
        else
        {
            File file = LITTLEFS.open("/genres/list", "r");
            file.seek(idx);
            l = file.read();
            if (l < 256) {
                file.read((uint8_t *)genreName, l);
                genreName[l] =  0;
            }
//            dbgprint("Got name from File: %s", genreName);
            file.close();
        }
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

int Genres::createGenre(const char *s, bool deleteLinks)
{
int res = findGenre(s);
    if (res < 0)
        res = 0;
    else if (res > 0)
        cleanGenre(res, deleteLinks);
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
                cleanGenre(res, true);
            }
        }
    }
    else
        res = 0;
    return res;
}


bool Genres::deleteGenre(int id)
{
bool ret = false;
    if ((id > 0) && (id < MAX_GENRES))
    {
        char path[30];
        sprintf(path, "/genres/%d/idx", id);
        if (ret = LITTLEFS.exists(path))
        {
            LITTLEFS.remove(path);
            sprintf(path, "/genres/%d/urls", id);
            LITTLEFS.remove(path);
            sprintf(path, "/genres/%d/links", id);
            LITTLEFS.remove(path);
        }
    }
    return ret;
}


bool Genres::addChunk(int id, const char *s, char delimiter) {
bool res = false;
    if ((id > 0) && (id <= _knownGenres) && (s != NULL))
        if (*s)
        {
        File urlFile, idxFile;
        char path[30];
            sprintf(path, "/genres/%d/idx", id);
            idxFile = LITTLEFS.open(path, "a");
            sprintf(path, "/genres/%d/urls", id);
            urlFile = LITTLEFS.open(path, "a");
            if (idxFile && urlFile)
            {
                res = true;
                size_t idx = urlFile.size();
                while(s)
                {
                    char *s1 = strchr(s, delimiter);
                    int l = s1?(s1 - s):strlen(s);
                    if (l > 0)
                    {
                        idxFile.write((uint8_t *)&idx, 4);
                        idx = idx + l + 1;
                        urlFile.write((uint8_t *)s, l);
                        urlFile.write(0);
                    }
                    if (s1)
                        s = s1 + 1;
                    else
                        s = NULL;
                }
            }
            idxFile.close();
            urlFile.close();
        }
}   


bool Genres::add(int id, const char *s) {
    bool res = true;
    if ((id > 0) && (id <= _knownGenres))
    {
        dbgprint("Request to add URL '%s' to Genre with id=%d", (s?s:"<NULL>"), id);
/*
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
*/  
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
        sprintf(path, "/genres/%d/urls", id);
        File urlFile = LITTLEFS.open(path, "a");
        File idxFile;
        if (!urlFile)
        {
            dbgprint("Can not open file '%s' to append URL '%s'", path, s);
            return false;
        }
        sprintf(path, "/genres/%d/idx", id);
        idxFile = LITTLEFS.open(path, "a");
        if (!idxFile)
        {
            dbgprint("Can not open index-file '%s'.", path);
            urlFile.close();
            return false;
        }
        size_t idx = urlFile.size();
//    _addIdx = _addIdx + strlen(s) + 1;
        urlFile.write((const uint8_t *)s, strlen(s) + 1);
        urlFile.close();
        idxFile.write((uint8_t *)&idx, sizeof(idx));
        idxFile.close();
//    _addCount++;
/*
    if ((_addCount % URL_CHUNKSIZE) == 0)
    {
        File idxFile;
        sprintf(path, "/genres/%d/idx", _addGenre);
        idxFile = LITTLEFS.open(path, "a");
        idxFile.write((uint8_t)URL_CHUNKSIZE);
        idxFile.write((const uint8_t *)&_addIdx, sizeof(_addIdx));
        idxFile.close();
        _addCount = 0;
*/  
    }
    return true;
}

/*
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
*/


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
            doprint("  DIR : %s (GenreName[%s]: %s)", file.name(), s, getName(atoi(s)).c_str());
            listDir(file.name());    
        } 
        else 
        {
            doprint("  FILE: %s\tSize: %ld", file.name(), file.size());
        }
        file = root.openNextFile();
    }
    //doprint("Done listing directory: %s", dirname);
}



void Genres::lsJson(Print& client, uint8_t lsmode)
{
    client.print("[");
    for(int id=1, total=0;id <= _knownGenres;id++)
    {
        char path[30];
        sprintf(path, "/genres/%d/idx", id);
        if (LITTLEFS.exists(path))
        {
            if (total++)
                client.print(',');
            client.println();
            client.printf("{\"id\": %d, \"name\": \"%s\"", id, getName(id).c_str());

            if (0 == (lsmode & LSMODE_SHORT))
            {
                uint16_t numUrls = count(id);
                client.printf(", \"presets\": %d, \"mode\": \"<valid>\"", numUrls);
            }
            if (0 != (lsmode & LSMODE_WITHLINKS))
            {
                String links = getLinks(id);
                client.printf(", \"links\": \"%s\"", links.c_str());
            }
            client.print('}');
        }
    }
    client.println();
    client.println("]");
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
            res = fSize / 4;
            /*
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
            */
            file.close();
        }
    }
    if (res > 0xffff)
        res = 0;
    return res;
}

String Genres::getUrl(int id, uint16_t number, bool cutName) {
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
            size_t seekPosition = number * 4;
            size_t chunkStart;
            size_t chunkSize;
            fileIdx.seek(seekPosition);
            fileIdx.read((uint8_t *)&chunkStart, 4);
            if (seekPosition + 4 < fileIdx.size())
            {
                fileIdx.read((uint8_t *)&chunkSize, 4);
            }
            else
                chunkSize = fileUrls.size();
            // chunkSize now contains the start index of next chunk, make it chunkSize with next expression
            //number = number % URL_CHUNKSIZE;
            chunkSize = chunkSize - chunkStart;
            dbgprint("Ready to locate url in file, chunkStart=%ld, chunkSize=%ld, idx=%d", chunkStart, chunkSize, number);
            char *s = (char *)malloc(chunkSize);
            if (s)
            {
                fileUrls.seek(chunkStart);
                fileUrls.read((uint8_t *)s, chunkSize);
/*
                char *p = s;
                while (number)
                {
                    number--;
                    p = p + strlen(p) + 1;
                }
*/
                char *p;
                if (cutName)
                    if (NULL != (p = strchr(s, '#')))
                        *p = 0;
                res = String(s);
                free(s);
            }
        }
        fileIdx.close();
        fileUrls.close();
    }
    return res;
}

/*
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
    }
    if (!res)
        _addGenre = 0;
    return res;
}
*/

void Genres::cleanLinks(int id){
    if (_begun && (id > 0) && (id <=_knownGenres))
    {
        char path[30];
        File linkFile;
        sprintf(path, "/genres/%d/links", id);
        linkFile = LITTLEFS.open(path, "w");
        if (linkFile)
            linkFile.close();
    }
}

void Genres::addLinks(int id, const char* moreLinks){
    if (_begun && (id > 0) && (id <=_knownGenres) && (strlen(moreLinks)))
    {
        char path[30];
        File linkFile;
        sprintf(path, "/genres/%d/links", id);
        linkFile = LITTLEFS.open(path, "a");
        if (linkFile)
        {
            dbgprint("Add link to genre[%d]=%s (len=%d) (filesize so far: %ld", 
                    id, moreLinks, strlen(moreLinks), linkFile.size());
            if (linkFile.size() > 0)
                linkFile.write(',');
            linkFile.write((uint8_t *)moreLinks, strlen(moreLinks));
            linkFile.close();
        }
    }
}

String Genres::getLinks(int id) {
String ret ="";
    if (_begun && (id > 0) && (id <=_knownGenres))
    {
        char path[30];
        File linkFile;
        sprintf(path, "/genres/%d/links", id);
        linkFile = LITTLEFS.open(path, "r");
        if (linkFile)
        {
            char s[linkFile.size() + 1];
            linkFile.read((uint8_t *)s, linkFile.size());
            s[linkFile.size()] = 0;
            linkFile.close();
            ret = String(s);
            dbgprint("Returning links for genre[%d]=%s", id, ret.c_str());
        }
    }
    return ret;
}

void Genres::cleanGenre(int id, bool deleteLinks)
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
                if (deleteLinks || (strstr(s, "links") == NULL))
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
//                uint8_t buf[5];
//                memset(buf, 0, 5);
//                file.write(buf, 5);
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

bool Genres::format()
{
bool ret = false;
        dbgprint("Formatting LITTLEFS for genre, that might take some time!");
        if (LITTLEFS.format())
            if (LITTLEFS.begin())
                _begun = ret = openGenreIndex();
    return ret;
}
