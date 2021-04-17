#ifndef RETRORADIOEXTENSION_H__
#define RETRORADIOEXTENSION_H__
#if defined(RETRORADIO) 
#include <map>

extern String getStringPart(String& value, char delim = ';');
extern bool getPairInt16(String& value, int16_t& x1, int16_t& x2,bool duplicate = false, char delim = ',');
extern void executeCmds(String commands);//, String value = "");
extern void doInput(String id, String value);
extern void chomp_nvs(String & str, const char *substitute = NULL);
extern String ramgetstr ( const char* key );
extern void ramsetstr ( const char* key, String val );


/*
struct cmp_str
{
   bool operator()(char const *a, char const *b) const
   {
      return strcmp(a, b) < 0;
   }
};
*/

class RetroRadioInputReader {
  public:
    virtual int16_t read() = 0;
    virtual void mode(int mod) = 0;
    virtual String info() {return String("");};
  protected:
    int _mode;
};

enum LastInputType  { NONE, NEAREST, HIT };

class RetroRadioInput {
  public:
    ~RetroRadioInput();
    const char* getId();
    void setId(const char* id);
    const char* getEvent();
    void setEvent(const char* name);
    int read(bool doStart);
    void setParameters(String parameters);
    static RetroRadioInput* get(const char *id);
    static void checkAll();
    static void showAll(const char *p);
    void setReader(String value);
    void showInfo(bool hint);
  protected:
    RetroRadioInput(const char* id);
    void executeCmdsWithSubstitute(String commands, String substitute);


    virtual void setParameter(String param, String value, int32_t ivalue);
    void setValueMap(String value, bool extend = false);
//    void clearValueMap();
    int16_t physRead();
    std::vector<int16_t> _valueMap;
//    bool _valid;
    int16_t _maximum;
  private:
    uint32_t _lastReadTime, _show, _lastShowTime, _debounceTime;
    int16_t _lastRead, _lastUndebounceRead, _delta, _step, _fastStep, _mode;
    LastInputType _lastInputType;
//    bool _calibrate;
    char *_id;
    char *_event;
    static std::vector<RetroRadioInput *> _inputList;
    RetroRadioInputReader *_reader;
//    std::map<const char*, RetroRadioInput *, cmp_str> _listOfInputs;
};


/*
class RetroRadioInputADC:public RetroRadioInput {
  public:
    RetroRadioInputADC(const char* id);
  protected:
    int16_t physRead();
  private:
    int8_t _pin;  
};

class RetroRadioInputTouch:public RetroRadioInput {
  public:
    RetroRadioInputTouch(const char* id);
  protected:
  protected:
    void setParameter(String param, String value, int32_t ivalue);
    int16_t physRead();
  private:
    touch_pad_t _pin;  
    bool _auto;
};
*/
#endif  // RETRORADIO
#endif
