#ifndef __GENRES_H_
#define __GENRES_H_
#include <LITTLEFS.h> 

#define MAX_GENRES 1000
//#define URL_CHUNKSIZE 10
#include <Arduino.h>

#define LSMODE_DEFAULT   0
#define LSMODE_SHORT     0b1
#define LSMODE_WITHLINKS 0b10

class Genres;

class GenreConfig {
friend class Genres;
    public:
        void noName(bool noName) {_noName = noName;};
        void showId(bool showId) {_showId = showId;};
        void rdbs(const char* s) {if (_rdbs) free(_rdbs);_rdbs = NULL; if (s) _rdbs = strdup(s);};
    //    void useSD(bool useSD);// {if (_genres) _genres->_isSD = useSD;};
        void verbose(bool mode) { _verbose = mode;};
        bool verbose() { return _verbose;};
        void toNVS();
        void info();
        String asJson();
    protected:
        Genres *_genres = NULL; 
        char *_rdbs = NULL;
        bool _noName = false;
        bool _showId = false;
        bool _verbose = true;
        bool _useSD = false;
};

class Genres {
friend class GenreConfig;
    public:
        Genres(const char* name = NULL);
        ~Genres();
        void nameSpace(const char *name);
        bool begin();
        int findGenre(const char *s);
        int createGenre(const char *s, bool deleteLinks = true);
        bool deleteGenre(int id);
        bool format(bool quick = false);
        bool deleteAll();
        bool add(int id, const char *s);
        bool addChunk(int id, const char *s, char delimiter);
        void cleanLinks(int id);
        void addLinks(int id, const char* moreLinks);
        String getLinks(int id);
        uint16_t count(int id);
        String getName(int id);
        String getUrl(int id, uint16_t number, bool cutName = true);
        String playList();
        void ls();
        void lsJson(Print& client, uint8_t lsmode = LSMODE_DEFAULT);
        void test();
        void dbgprint ( const char* format, ... );
        void cacheStep();
        //void verbose ( int mode);
        GenreConfig config;
    protected:
        bool _begun = false;
        bool _wasBegun = false;
        char* _nameSpace = NULL;
        bool _isSD = false;
        FS *_fs = NULL;
        uint16_t _knownGenres = 0;
        uint16_t _cacheIdx = 0;
        struct {
            size_t idx;
            uint16_t count;
        } _idx[MAX_GENRES + 1];
        bool openGenreIndex(bool onlyAppend = false);
        void cleanGenre(int idx, bool deleteLinks = true);
        void addToPlaylist(int idx);
        void deleteDir(const char* dirname);
        String fileName(const char* name = NULL);
//        void stopAdd();
//        bool startAdd(int id);
        void listDir(const char* dirname);
        void *gmalloc(size_t size);
        uint8_t *_psramList = NULL;
        char *_gplaylist = NULL;
//        bool _verbose = true;
//        uint16_t _addCount = 0;
//        size_t _addIdx = 0;
//        int _addGenre = 0;
};


extern Genres genres;

#endif