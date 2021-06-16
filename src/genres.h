#ifndef __GENRES_H_
#define __GENRES_H_
#define MAX_GENRES 1000
#define URL_CHUNKSIZE 10
#include <Arduino.h>


class Genres {
    public:
        bool begin();
        int findGenre(const char *s);
        int createGenre(const char *s);
        void format();
        bool add(int id, const char *s);
        uint16_t count(int id);
        String getName(int id);
        String getUrl(int id, uint16_t number);
        void ls();
        void test();
    protected:
        bool _begun = false;
        uint16_t _knownGenres = 0;
        struct {
            size_t idx;
        } _idx[MAX_GENRES];
        bool openGenreIndex();
        void dbgprint ( const char* format, ... );
        void cleanGenre(int idx);
        void stopAdd();
        bool startAdd(int id);
        void listDir(const char* dirname);
        uint8_t *_psramList = NULL;
        bool _verbose = true;
        uint16_t _addCount = 0;
        size_t _addIdx = 0;
        int _addGenre = 0;
};


extern Genres genres;

#endif