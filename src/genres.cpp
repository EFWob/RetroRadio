#include <genres.h>
#include <AddRR.h>
#include <SD.h>

//#undef BOARD_HAS_PSRAM
//#define ROOT_GENRES "/_____gen.res"
//#define ROOT_GENRES "/g/en/r/e/s"
Genres genres("/_____gen.res/genres");

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
              ",\"isSD\":" + String(_genres->_isSD?1:0) +
              ",\"path\": \"" + String(_genres->_nameSpace) + "\"" +
              ",\"open\":" + String(_genres->_begun?1:0) +
               "}";
}

Genres::Genres(const char *name) {
    if (NULL == name)
        name = "default";
    nameSpace(name);
    config._genres = this;
//    memcpy((void *)&_nameSpace, (void *)name, 8);
//    _nameSpace[8] = 0;
}

Genres::~Genres() {
    if (_nameSpace)
        free(_nameSpace);
}

void Genres::nameSpace(const char *name) {
    if (_nameSpace)
        free(_nameSpace);
    _isSD = false;
    if (name[2] == ':')
        if ((name[0] == 's')||(name[0] == 'S'))
            if (_isSD = (name[1] == 'd') || (name[1] == 'D'))
                name = name + 3;
    _nameSpace = strdup(name);
    if (_wasBegun)
    {
        _begun = false;
        this->begin();
    }
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
    _wasBegun = true;
    if (_begun)                 // Already open?
        return true;
    if (_isSD)
        _fs = &SD;
    else
        _fs = &LITTLEFS;
    if (_fs == &LITTLEFS)
    {
        if (!(_begun = LITTLEFS.begin()))
            {
            if (!(_begun = this->format()))
                {
                dbgprint("LITTLEFS formatting failed! Can not play from genres!");
                }
             }
        else
            _begun = openGenreIndex();
    }
    else    
        _begun = openGenreIndex();
    if (!_begun)
        _fs = NULL;
    return _begun;
}       

String Genres::fileName(const char *name) {
//    String ret = String(ROOT_GENRES) + String("/") + String(_nameSpace);
    String ret = String(_nameSpace);  
    if (name)
        ret = ret + "/" + name;
    return ret;
}

bool Genres::openGenreIndex(bool onlyAppend) {
bool res = false;
    //doprint("openGenreIndex started!")
    if (NULL != _psramList)
    {    
        free(_psramList);
        _psramList = NULL;
    }
    if (NULL != _gplaylist)
    {
        free(_gplaylist);
        _gplaylist = NULL;
    }
    if (!onlyAppend)
    {
        _cacheIdx = 0;
        memset(_idx, 0, sizeof(_idx));
    }
    uint16_t _lastKnownGenres = _knownGenres;
    _knownGenres = 0;
    if (!_fs)
        return false;
//    if (!(res = (0 == strlen(ROOT_GENRES))))
    if (!(res = (0 == strlen(_nameSpace))))
    {
//        char s[strlen(ROOT_GENRES) + 1];
//        strcpy(s, ROOT_GENRES);
        char s[strlen(_nameSpace) + 1];
        strcpy(s, _nameSpace);
        char *p = s;
        while (p) 
        {
            p = strchr(p + 1, '/');
            if (p) {
                *p = 0;
            }
            res = _fs->exists(s);
            if (!res)
            {
                dbgprint("Creating ROOT-directory '%s' for genres", s);
                res = _fs->mkdir(s);
            }
            if (!res)
                p = NULL;
            else if (p)
                *p = '/';
        }
        /*
        res = _fs->exists(ROOT_GENRES);
        if (!res)
        {
            dbgprint("Creating ROOT-directory '%s' for genres", ROOT_GENRES);
            res = _fs->mkdir(ROOT_GENRES);
        }
        */
    }
    /*
    if (res)
    {
        String fName = fileName();
        const char *s = fName.c_str();
        res = _fs->exists(s);
        if (!res)
        {
            dbgprint("Creating LITTLEFS directory '%s'", s);
            res = _fs->mkdir(s);
        }
    }
    */
    if (res)
    {
        dbgprint("LITTLEFS success opening directory '%s'", fileName().c_str());
        File idxFile;
        size_t size = 0;
        String fName = fileName("list");
        const char* s= fName.c_str();
        if ( _fs->exists(s) )
        {
         idxFile = _fs->open(s);
         size = idxFile.size();
        }
        if (size > 0)
        {
            _psramList = (uint8_t *)gmalloc(idxFile.size());
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
                if (!onlyAppend || (idx == _lastKnownGenres))
                {
                    _idx[idx].count = 0xffff;
                    _idx[idx++].idx = i++;
                }
                else 
                {
                    i++;
                    idx++;
                }
                if (skip >= 0)
                    i = i + skip;
            }
            _knownGenres = idx;
            if (onlyAppend)
                cacheStep();
        }
        else
            dbgprint("ListOfGenres file has size 0");
        idxFile.close();
    }
    else
    {
        dbgprint("LITTLEFS could not create directory '%s'", fileName().c_str());
    }
    return res;
}

String Genres::getName(int id)
{
    char genreName[256];
    genreName[0] = 0;
    if (_fs && _begun && (id > 0) && (id <= _knownGenres))
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
            File file = _fs->open(fileName("list").c_str(), "r");//"/genres/list", "r");
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
    if (_fs && _begun && (len>0))
    {
        File idxFile;
        if (!_psramList)    
            idxFile = _fs->open(fileName("list").c_str());//"/genres/list");
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
int res;
    if (_fs == NULL)
        return 0;
    res  = findGenre(s);
    if (res < 0)
        res = 0;
    else if (res > 0)
        cleanGenre(res, deleteLinks);
    else if (_knownGenres < MAX_GENRES)
    {
        File idxFile = _fs->open(fileName("list").c_str(), "a");//"/genres/list", "a");
        if (!idxFile)
        {
            dbgprint("Could not open '%s' for adding", fileName("list").c_str());
            res = 0;
        
        }
        else
        {
            bool valid;
            int l = strlen(s);
            if (l > 255)
                l = 255;
            dbgprint("Adding to idxFile, current size is %d", idxFile.size());
            if ( (valid = (idxFile.write((uint8_t)l) == 1)) )
                valid = (l == idxFile.write((const uint8_t *)s, l));
            idxFile.close();
//            if (valid)
//                _knownGenres++;
//            else
            if (!valid)
            {
                res = 0;
                //openGenreIndex();
            }
            else if (openGenreIndex(true))
            {
                if ( res = findGenre(s) )
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
    if (_fs && (id > 0) && (id < MAX_GENRES))
    {
        char path[30];
        if (_idx[id - 1].count != 0)
        {
            if (_idx[id - 1].count != 0xfffe)
                if (_gplaylist)
                {
                    free(_gplaylist);
                    _gplaylist = NULL;
                }
        }
        _idx[id - 1].count = 0;
        sprintf(path, "%d/idx", id);
        const char* s = fileName(path).c_str();
        if ( (ret = _fs->exists(s)) )
        {
            _fs->remove(s);
            sprintf(path, "%d/urls", id);
            s = fileName(path).c_str();
            _fs->remove(s);
            sprintf(path, "%d/links", id);
            s = fileName(path).c_str();
            _fs->remove(s);
        }

/*
        sprintf(path, "/genres/%d/idx", id);
        if ( (ret = _fs->exists(path)) )
        {
            _fs->remove(path);
            sprintf(path, "/genres/%d/urls", id);
            _fs->remove(path);
            sprintf(path, "/genres/%d/links", id);
            _fs->remove(path);
        }
*/
    }
    return ret;
}


bool Genres::addChunk(int id, const char *s, char delimiter) {
bool res = false;
    if (_fs && (id > 0) && (id <= _knownGenres) && (s != NULL))
        if (*s)
        {
        File urlFile, idxFile;
        uint16_t added = 0;
        char path[30];
        uint32_t count;
        String fnameStr;
        const char *fname;
            sprintf(path, "%d/idx", id);
            fnameStr = fileName(path);
            fname = fnameStr.c_str();
            idxFile = _fs->open(fname, "a");
            sprintf(path, "%d/urls", id);
            fnameStr = fileName(path);
            fname = fnameStr.c_str();
            urlFile = _fs->open(fname, "a");
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
                        added++;
                    }
                    if (s1)
                        s = s1 + 1;
                    else
                        s = NULL;
                }
            }
            idxFile.flush();
            count = idxFile.size() / 4;
            if (count >= 0xffff)
                count = 0xfffe;
            _idx[id - 1].count = count;
            if ((count == added) && (NULL != _gplaylist))
                addToPlaylist(id);
            idxFile.close();
            urlFile.close();
        }
    return res;        
}   


bool Genres::add(int id, const char *s) {
    bool res = false;
    if (_fs && (id > 0) && (id <= _knownGenres))
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
        res = true;
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
        const char* fname;
        sprintf(path, "%d/urls", id);
        String fnameStr = fileName(path);
        fname = fnameStr.c_str();
        File urlFile = _fs->open(fname, "a");
        File idxFile;
        if (!urlFile)
        {
            dbgprint("Can not open file '%s' to append URL '%s'", fname, s);
            return false;
        }
        sprintf(path, "%d/idx", id);
        fname = fileName(path).c_str();
        idxFile = _fs->open(fname, "a");
        if (!idxFile)
        {
            dbgprint("Can not open index-file '%s'.", fname);
            urlFile.close();
            return false;
        }
        size_t idx = urlFile.size();
//    _addIdx = _addIdx + strlen(s) + 1;
        urlFile.write((const uint8_t *)s, strlen(s) + 1);
        urlFile.close();
        idxFile.write((uint8_t *)&idx, sizeof(idx));
        idxFile.flush();
        uint32_t count = idxFile.size() / 4;
        if (count < 0xfffe)
            count++;
        else
            count = 0xfffe;
        if ((count == 1) && (_gplaylist))
            addToPlaylist(id);
        _idx[id - 1].count = count;
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

bool Genres::deleteAll() {
    if (_fs)
    {
        deleteDir(fileName().c_str());
        if ( !(_begun = openGenreIndex()) )
            _fs = NULL;
    }
    return _fs;
}

void Genres::ls() {
    listDir(fileName().c_str());    
}

void Genres::test() {
    if (_fs == &LITTLEFS)
    doprint("Genre Filesystem total bytes: %ld, used bytes: %ld (free=%ld)",
        LITTLEFS.totalBytes(), LITTLEFS.usedBytes(), LITTLEFS.totalBytes() - LITTLEFS.usedBytes());
    else if (_fs)
        doprint("No information available for used Genre Filesystem!");
    else  
        doprint("Genre Filesystem is not mounted!");
}


String Genres::playList() {
    String res;
    
    if (_gplaylist != NULL)
        res = String(_gplaylist);
    else
    {
        bool notFirst = false;
        Serial.println("Building genre playlist from scratch!");
        for(int i = 1;i <= _knownGenres;i++)
            if (count(i) > 0)
            {
                if (notFirst)
                    res = res + "," + getName(i);
                else
                {
                    res = getName(i);
                    notFirst = true;
                }
            }
        if ( (_gplaylist = (char *)gmalloc(res.length() + 1)) )
            memcpy(_gplaylist, res.c_str(), res.length() + 1);
    }
    return res;
}

void Genres::listDir(const char * dirname){
    doprint("Listing directory: %s", dirname);

    if (!_fs)
    {
        doprint("Genre Filesystem is not mounted!");
        return;
    }
    File root = _fs->open(dirname);
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
    String rootDir = fileName("");
    const char* rootDirName = rootDir.c_str();
    int rootDirLen = strlen(rootDirName);
    while(file)
    {
        if(file.isDirectory())
        {
            const char *s;
            if ( (s = strstr(file.name(), rootDirName)) )
                s = s + rootDirLen;
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

void Genres::deleteDir(const char * dirname){
char s[100];
    dbgprint("Deleting directory: %s", dirname);
    if (!_fs)
    {
        dbgprint("Genre Filesystem is not mounted!");
        return;
    }
    File root = _fs->open(dirname);
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
        strcpy(s, file.name());
        bool isDir = file.isDirectory();
        file = root.openNextFile();
        if(isDir)
        {
            deleteDir(s);    
        } 
        else 
        {
            dbgprint("Delete file '%s'", s);
            _fs->remove(s);
        }
    }
    _fs->rmdir(dirname);
    //doprint("Done listing directory: %s", dirname);
}



void Genres::lsJson(Print& client, uint8_t lsmode)
{
    client.print("[");
    for(int id=1, total=0;id <= _knownGenres;id++)
    {
        //char path[30];
        //sprintf(path, "/genres/%d/idx", id);
        if ( count(id) > 0 )
        {
            //Serial.printf("\r\nPath=%s exists!\r\n", path);
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
//        else   
//        {
//            Serial.printf("\r\nPath=%s does not exist!\r\n", path);
//            _idx[id - 1].count = 0;
//        }
    }
    client.println();
    client.println("]");
}

uint16_t Genres::count(int id) {
uint32_t res = 0;
//char path[30];
String fnameStr;
const char *fname;

    if (_fs && (id > 0) && (id <= _knownGenres))
    {
        if ((_cacheIdx == 0) && (id == 1))
        {
            String fnameStr = fileName("idx");
            const char *fname = fnameStr.c_str();
            //strcpy(path, "/genres/idx");
            if ( _fs->exists(fname) )
            {
                File file = _fs->open(fname, "r");
                size_t fSize = file.size();
                res = fSize / sizeof(_idx[0]);
                if (res == _knownGenres)
                {
                    file.read((uint8_t *)&_idx, sizeof(_idx[0]) * _knownGenres);
                    file.close();
                    _cacheIdx = _knownGenres;
                }
                else
                {
                    file.close();
                    _fs->remove(fname);
                }
            }
        }
        if (_idx[id-1].count != 0xffff)
            res = _idx[id - 1].count;
        else
        {
            char pathId[20];
            sprintf(pathId, "%d/idx", id);
            fnameStr = fileName(pathId);
            fname = fnameStr.c_str();
            if ( _fs->exists(fname) )
            {
                File file = _fs->open(fname, "r");
                size_t fSize = file.size();
                res = fSize / 4;
                file.close();
            }
            if (res <= 0xfffe)
                _idx[id - 1].count = res;
            else
                _idx[id - 1].count = 0xfffe;
            if (_cacheIdx == id - 1)
            {
                _cacheIdx = id;
                if (id == _knownGenres)
                {
                    //strcpy(path, "/genres/idx");
                    fnameStr = fileName("idx");
                    fname = fnameStr.c_str();
                    File file = _fs->open(fname, "w");
                    file.write((uint8_t *)&_idx, sizeof(_idx[0]) * _knownGenres);
                    file.close();
                }
            }
        }
    }
    if (res >= 0xfffe)
        res = 0;
    return res;
}

void Genres::cacheStep()
{
    if (_knownGenres > _cacheIdx) {
        count(_cacheIdx + 1);
        if (((_cacheIdx % 10) == 0) || (_cacheIdx == _knownGenres)) 
            dbgprint("CacheStep: %d (knownGenres: %d)", _cacheIdx, _knownGenres);
    }
}

String Genres::getUrl(int id, uint16_t number, bool cutName) {
String res = "";
    if (_fs && (count(id) > number))
    {
        char path[20];
        char fileNameIdx[30], fileNameUrls[30];
        File fileIdx, fileUrls;
        sprintf(path, "/genres/%d", id);
        sprintf(fileNameIdx, "%s/idx", path);
        sprintf(fileNameUrls, "%s/urls", path);
        fileIdx = _fs->open(fileNameIdx, "r");
        fileUrls = _fs->open(fileNameUrls, "r");
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
    if (_fs && _begun && (id > 0) && (id <=_knownGenres))
    {
        char path[30];
        File linkFile;
        sprintf(path, "/genres/%d/links", id);
        linkFile = _fs->open(path, "w");
        if (linkFile)
            linkFile.close();
    }
}

void Genres::addLinks(int id, const char* moreLinks){
    if (_fs && _begun && (id > 0) && (id <=_knownGenres) && (strlen(moreLinks)))
    {
        char path[30];
        File linkFile;
        sprintf(path, "/genres/%d/links", id);
        linkFile = _fs->open(path, "a");
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
    if (_fs && _begun && (id > 0) && (id <=_knownGenres))
    {
        char path[30];
        sprintf(path, "/genres/%d/links", id);
        if ( _fs->exists(path) )
        {
            File linkFile = _fs->open(path, "r");
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
    String fnameStr;
    const char *fname;
    if (_begun && _fs)
    {
    char s0[20];
    char s[50];
        if (_idx[id - 1].count > 0)
        {
            if (_gplaylist)
            {
                free(_gplaylist);
                _gplaylist = NULL;
            }
            _idx[id - 1].count = 0;
            Serial.println("gplaylist deleted!");
        }
        //TODO delete specific files for Genre
        sprintf(s0, "%d", id);
        fnameStr = fileName(s0);
        fname = fnameStr.c_str();
        File root = _fs->open(fname);
        if (!root) {
            dbgprint("No content is stored in LITTLEFS for genre with id: %d", id);
            dbgprint("Creating directory %s", fname);
            _fs->mkdir(fname);
        }
        else if (!root.isDirectory()) {
            dbgprint("PANIC because unexpected: File with name %s is not a directory!", fname);
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
                    dbgprint("Removing '%s'=%d", s, _fs->remove(s));
                free(s);
            }        
            //dbgprint("Removing directory '%s'=%d", s0, LITTLEFS.rmdir(s0));
            root.close();
        }
        root = _fs->open(fname); // Directory should be open now..
        /*
        if (root && (root.isDirectory()))
        {

            sprintf(s, "%s/idx", s0);
            File file = _fs->open(s, "w");
            if (file)
            {
//                uint8_t buf[5];
//                memset(buf, 0, 5);
//                file.write(buf, 5);
                file.close();
            }
            sprintf(s, "%s/urls", s0);
            file = _fs->open(s, "w");
            if (file)
            {
                file.close();
            }
        }
        else
        {
            dbgprint("PANIC because unexpected: Directory with name %s not found!", s0);
        }
        */
        root.close();

    }
    else 
        dbgprint("LITTLEFS is not mounted (for deleting genre with id: %d", id);
}

bool Genres::format(bool quick)
{
bool ret = false;
    if (_fs == &LITTLEFS)
    {
        if (quick)
        {
            File f;
            dbgprint("Quick Formatting LITTLEFS for genre!");
            f = _fs->open("/genres/list", "w");
            ret = f;
            f.close();
            if (ret)
                _begun = ret = openGenreIndex();
        }
        else
        { 
            dbgprint("Formatting LITTLEFS for genre, that might take some time!");
            if (LITTLEFS.format())
                if (LITTLEFS.begin())
                    _begun = ret = openGenreIndex();
        }
    }
    else if (_fs)
    {
        dbgprint("Deleting everything from filesystem for genre, that might take some time!");
        ret = deleteAll();
    }
    if (ret)
        dbgprint("Success formatting LITTLEFS!");
    else
        dbgprint("Error formatting LITTLEFS!");
    return ret;
}


void *Genres::gmalloc(size_t size)
{
    void *p = NULL;           
#if defined(BOARD_HAS_PSRAM)
    p = (uint8_t *)ps_malloc(size);
#endif
    if (!p)
        if (ESP.getFreeHeap() > size)
            if (ESP.getFreeHeap() - size > 80000)
                p = malloc(size);
    return p;
}

void Genres::addToPlaylist(int idx){
    if ((idx > 0) && (idx < _knownGenres) && (NULL != _gplaylist))
    {
        String name = getName(idx);
        if (name.length() > 0)
        {
            if (_gplaylist[0] != 0)
                name = "," + name;
            char *p = (char *)gmalloc(name.length() + strlen(_gplaylist) + 2);
            if (p)
            {
                strcpy(p, _gplaylist);
                strcat(p, name.c_str());
                free(_gplaylist);
                _gplaylist = p;
                Serial.printf("Added new genre to _gplaylist: %s\r\n", _gplaylist);
            }
            else
            {
                free(_gplaylist);
                _gplaylist = NULL;

            }

        }
    }

}