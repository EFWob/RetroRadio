#ifndef __GENRES_H_
#define __GENRES_H_
#define MAX_GENRES 1000
//#define URL_CHUNKSIZE 10
#include <Arduino.h>


class Genres {
    public:
        bool begin();
        int findGenre(const char *s);
        int createGenre(const char *s, bool deleteLinks = true);
        bool deleteGenre(int id);
        bool format();
        bool add(int id, const char *s);
        bool addChunk(int id, const char *s, char delimiter);
        void cleanLinks(int id);
        void addLinks(int id, const char* moreLinks);
        String getLinks(int id);
        uint16_t count(int id);
        String getName(int id);
        String getUrl(int id, uint16_t number);
        void ls();
        void lsJson(Print& client, bool fullInfo = true);
        void test();
        void dbgprint ( const char* format, ... );
        void verbose ( int mode);
    protected:
        bool _begun = false;
        uint16_t _knownGenres = 0;
        struct {
            size_t idx;
        } _idx[MAX_GENRES];
        bool openGenreIndex();
        void cleanGenre(int idx, bool deleteLinks = true);
//        void stopAdd();
//        bool startAdd(int id);
        void listDir(const char* dirname);
        uint8_t *_psramList = NULL;
        bool _verbose = true;
//        uint16_t _addCount = 0;
//        size_t _addIdx = 0;
//        int _addGenre = 0;
};


extern Genres genres;

#endif