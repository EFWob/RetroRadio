#ifndef RETRORADIOEXTENSION_H__
#define RETRORADIOEXTENSION_H__
#include <map>

extern String getStringPart(String& value, char delim = ';');
extern bool getPairInt16(String& value, int16_t& x1, int16_t& x2,bool duplicate = false, char delim = ',');

struct cmp_str
{
   bool operator()(char const *a, char const *b) const
   {
      return strcmp(a, b) < 0;
   }
};

class RetroRadioInput {
  public:
    RetroRadioInput(const char* id);
    ~RetroRadioInput();
    const char* getId();
    const char* getName();
    void setName(const char* name);
    int16_t read(bool show = false);
    bool valid();
    void check();
    void setParameters(String parameters);
  protected:
    virtual void setParameter(String param, String value, int32_t ivalue);
    void setValueMap(String value);
    void clearValueMap();
    virtual int16_t physRead();
    std::vector<int16_t> _valueMap;
    bool _valid;
    int16_t _maximum;
  private:
    uint32_t _lastReadTime, _show, _lastShowTime, _debounceTime;
    int16_t _lastRead, _lastUndebounceRead, _delta, _step, _fastStep;
    bool _force, _calibrate;
    char *_id;
    char *_name;
    std::map<const char*, RetroRadioInput *, cmp_str> _listOfInputs;
};


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

#endif
