#ifndef __GENRES_H_
#define __GENRES_H_

class Genres {
    public:
        bool begin();
        
    protected:
        bool _begun = false;
        bool openGenreIndex();
};


extern Genres genres;

#endif