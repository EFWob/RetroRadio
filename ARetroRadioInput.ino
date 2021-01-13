#include "ARetroRadioExtension.h"

//**************************************************************************************************
//                                          D O P R I N T                                          *
//**************************************************************************************************
// Copy of dbgprint. Only difference: will always print but only prefix "D: ", if DEBGUG==1        *
//**************************************************************************************************
char* doprint ( const char* format, ... )
{
  static char sbuf[DEBUG_BUFFER_SIZE] ;                // For debug lines
  va_list varArgs ;                                    // For variable number of params

  va_start ( varArgs, format ) ;                       // Prepare parameters
  vsnprintf ( sbuf, sizeof(sbuf), format, varArgs ) ;  // Format the message
  va_end ( varArgs ) ;                                 // End of using parameters
  if ( DEBUG )                                         // DEBUG on?
  {
    Serial.print ( "D: " ) ;                           // Yes, print prefix
  }
  Serial.println ( sbuf ) ;                            // always print the info
  return sbuf ;                                        // Return stored string
}


//**************************************************************************************************
//                          G E T S T R I N G P A R T                                              *
//**************************************************************************************************
// Splits the input String value at the delimiter identified by parameter delim.                   *
//  - Returns left part of the string, chomped and lowercased                                      *
//  - Input String will contain the remaining part after delimiter (not altered)                   *
//  - will return the input string if delimiter is not found
//**************************************************************************************************
String getStringPart(String& value, char delim) {
int idx = value.indexOf(delim);
String ret = value;
  if (idx < 0) {
    value = "";
  } else {
    ret = ret.substring(0, idx);
    value = value.substring(idx + 1);
  }
  chomp(ret);
  ret.toLowerCase();
  return ret;
}

//**************************************************************************************************
//                          G E T P A I R I N T 1 6                                                *
//**************************************************************************************************
// Extracts a pair of int16 values from String                                                     *
//  - Returns true, if two values are found (will be stored in parameters x1, x2)                  *
//  - Also true, if only one is found but parameter duplicate is set to true (x2 will set to x1).  *
//  - Remaining characters are ignored.                                                            *
//**************************************************************************************************


bool getPairInt16(String& value, int16_t& x1, int16_t& x2,bool duplicate, char delim) {
bool ret = false;
  chomp(value);
  if ((value.c_str()[0] == '-') || isdigit(value.c_str()[0])) {
    int idx = value.indexOf(delim);
    x1 = atoi(value.c_str());
    if (ret = (idx > 0)) {
      x2 = atoi(value.substring(idx + 1).c_str());
    } else if (ret = duplicate) 
        x2 = x1;
  }
  return ret;
}


//**************************************************************************************************
// Class: RetroRadioInput                                                                          *
//**************************************************************************************************
// Generic class for Input (from GPIOs normally)                                                   *
//  - Needs subclasses to implement different input types (Digital, Analog, Touch...)              *
//  - Subclass must implement int physRead(); function to return current value of input             *
//  - Class RetroRadioInput provides common functionality like debouncing and mapping the read     *
//    value from physical value to application defined values                                      *
//**************************************************************************************************


//**************************************************************************************************
// Class: RetroRadioInput.Constructor                                                              *
//**************************************************************************************************
//  - Each application has an ID that remains constant during object lifetime (though application  *
//    constructing the object is responsible for checking the validity/uniqueness of that id if    *
//    so desired).                                                                                 *
//  - Each object also has a name, that is set equal to id at construction but can be changed      *
//    by application. That name is used to generate events at changes on input, again the          *
//    application layer is responsible for validity.                                               *
//**************************************************************************************************
RetroRadioInput::RetroRadioInput(const char* id):_valid(false), _show(0), _lastShowTime(0),
                          _debounceTime(0), _force(false), _calibrate(false), _delta(1), _step(0), _fastStep(0) {
  if (_name = strdup(id))
    _id = strdup(id);
  _maximum = _lastRead = _lastUndebounceRead = INT16_MIN;                 // _maximum holds the highest value ever read, _lastRead the 
                                                    // last read value (after mapping. Is below 0 if no valid read so far)
  setParameters("@/input");
}

//**************************************************************************************************
// Class: RetroRadioInput.getId()                                                                  *
//**************************************************************************************************
//  - Returns the id of the RetroRadioInput                                                        *
//**************************************************************************************************
const char* RetroRadioInput::getId() {
  return _id;
}

//**************************************************************************************************
// Class: RetroRadioInput.getName()                                                                  *
//**************************************************************************************************
//  - Returns the name of the RetroRadioInput                                                        *
//**************************************************************************************************
const char* RetroRadioInput::getName() {
  return _name;
}

//**************************************************************************************************
// Class: RetroRadioInput.setName(const char* name)                                                *
//**************************************************************************************************
//  - Sets the name of the RetroRadioInput object. Defaults to _id, if (NULL == name)              *
//**************************************************************************************************
void RetroRadioInput::setName(const char* name) {
  if (_name)
    free(_name);
  if (name)
    _name = strdup(name);
  else
    _name = strdup(_id);
}

//**************************************************************************************************
// Class: RetroRadioInput.setParameters(String value)                                              *
//**************************************************************************************************
//  - Sets one or more parameters of the Input objects                                             *
//  - Multiple parameters can be seperated by ';'                                                  *
//  - If a parameter starts with '@' it is considered to reference an NVS-key (w/o leading '@') to * 
//    read parameter(s) from                                                                       *
//**************************************************************************************************
void RetroRadioInput::setParameters(String params) {
  static int recursion = 0;
  doprint("SetParameters for input%s=%s", getId(), params.c_str());
  recursion++;
  if (recursion > 5)
    return;
  chomp(params);
  while(params.length()) {                              // anything left
    String value = getStringPart(params, ';');          // separate next parameter
    int ivalue;
    String param = getStringPart(value, '.');         // see if value is set
    if (param.c_str()[0] == '@') {                       // reference to NVS?
      setParameters(nvsgetstr(param.c_str() + 1));             // if yes, read NVS from key
    } else {
      ivalue = atoi(value.c_str());                       // usally value is an int number 
      if (param.length() > 0)
        setParameter(param, value, ivalue);
    }
  }
  recursion--;
}


//**************************************************************************************************
// Class: RetroRadioInput.setParameter(String param, String value, int32_t ivalue)                 *
//**************************************************************************************************
//  - Actually sets one parameter                                                                  *
//  - param and value are no further processed (no expansion of NVS etc...)                        *
//  - ivalue is the int32_t representation of value                                                *
//  - virtual, can be overidden by subclasses                                                      *
//**************************************************************************************************
void RetroRadioInput::setParameter(String param, String value, int32_t ivalue) {
  if (param == "show") {
    _show = (uint32_t)ivalue * 1000UL;
  } else if (param == "map") {                        // set the valueMap for the input
    setValueMap(value);                               // see RetroRadioInput::setValueMap() for details
  } else if (param == "force") {                      // force a readout event
    _force = true;                                    // on mapped input, if not hit, nearest valid value will be used
  } else if (param == "calibrate") {                  // as "show", just more details on map hits.
    _calibrate = ivalue;
  } else if (param == "debounce") {                     // how long must a value be read before considered valid?
      _debounceTime = ivalue;                           // (in msec).
  } else if (param == "delta") {
    if (ivalue > 1)
      _delta = ivalue;
    else
      _delta = 1;
  } else if (param == "step") {
    if (ivalue >= 0)
      _step = ivalue;
  } else if (param == "faststep") {
    if (ivalue >= 0)
      _fastStep = ivalue;
  }
}


//**************************************************************************************************
// Class: RetroRadioInput.setValueMap(String value)                                                *
//**************************************************************************************************
//  - Set the mapping table of the RetroRadioInput object                                          *
//  - more than one mapping entry can be added, separate mapping entries must be separated by '|'  *
//  - if value (after chomp) starts with @ the remaining string is treated as key to an NVS entry  *
//    from preferences                                                                             *
//  - One single entry is expected to have the format:                                             *
//        x1[,x2] = y1[,y2]                                                                        *
//    Interpretation: if read is called, the phyical value obtained from physRead() (if valid)     *
//    is compared with all defined map-entries if it fits within x1 and x2 (x2 can be below x1)    *
//    x1 and x2 are inclusive, if x2 is not provided it will be set equal to x1.                   *
//    If there is a hit, x will be mapped to y1 to y2. (if y2 is not provided, y2 will set to y1)  *
//    both x and y range can be in reverse order (but for y must not be below 0)                   *
//    Examples:                                                                                    *
//       0 = 1; 1 = 0  (expanded to 0,0 = 1,1;1,1 = 0,0) will iverse a digital input               *
//       0,1 = 1,0 would do the same with just one map entry                                       *
//       0, 4095 = 100, 0  will convert input values between 0-4095 (i. e. from analog input) to   *
//                 an output range of 100 to 0                                                     *
//  - A map entry with empty left part is also possible. In that case the left part will be set to *
//    x1 = 0 and x2 = INT16_MAX. This cancels also the map setting, as this entry will cover the   *
//    full possible input range.                                                                   *
//  - In case of overlapping map ranges, the first defined map enty will fire. Example:            *
//    1000,3000 = 1;=2;4000,4050=3;                                                                *
//    will yield 1 for all readings between 1000 and 3000 (inclusive), and 2 in any other case     *
//    (the third entry will be ignored since the range between 4000 and 4050 is also covered by    *
//    the second  
//  - calling setMap will erase the old mapping                                                    *                                       
//**************************************************************************************************

void RetroRadioInput::setValueMap(String value) {
  chomp(value);
  clearValueMap();
  if (value.c_str()[0] == '@') {                 // value pointing to NVS-Entry?
    value = nvsgetstr(value.c_str() + 1);         // yes: get that entry
  }
  do {
    int16_t x1, x2, y1, y2;
    String mapEntry = getStringPart(value, '|');     // extract one map entry
    String left = getStringPart(mapEntry, '=');      // split left and right part
    Serial.printf("After split: (%s)=(%s)\r\n", left.c_str(), mapEntry.c_str());
    if (getPairInt16(mapEntry, y1, y2, true)) {
      bool ok = (left.length() == 0);
      if (ok) {
        x1 = 0;x2 = INT16_MAX;value = "";
      } else
        ok = getPairInt16(left, x1, x2, true);
      if (ok) {
        _valueMap.push_back(x1);                  // add to map if both x and y pair is valid
        _valueMap.push_back(x2);
        _valueMap.push_back(y1);
        _valueMap.push_back(y2);        
        Serial.printf("Map entry for input_%s: %d, %d = %d, %d\r\n", getId(), _valueMap[_valueMap.size() - 4],
                                 _valueMap[_valueMap.size() - 3],_valueMap[_valueMap.size() - 2],_valueMap[_valueMap.size() - 1]);
      }
    }
    chomp(value);
  } while (value.length() > 0);                   // repeat if there is more in the input
}


//**************************************************************************************************
// Class: RetroRadioInput.clearValueMap()                                                          *
//**************************************************************************************************
//  - Clears the value map, if that map is not empty. In that case, also _lastRead is reset, as    *
//    from now on the mapping used to calculate _lastRead is no longer valid                       *
//**************************************************************************************************
void RetroRadioInput::clearValueMap() {
  if (_valueMap.size()) {
    _valueMap.clear();
    _lastRead = _lastUndebounceRead = INT16_MIN;
  }
}

//**************************************************************************************************
// Class: RetroRadioInput.valid()                                                                  *
//**************************************************************************************************
//  - Virtual method, can be overidden by subclasses. True, if a valid input is associated with    *
//    the calling object.                                                                          *
//    from now on the mapping used to calculate _lastRead is no longer valid                       *
//**************************************************************************************************
bool RetroRadioInput::valid() {
  return _valid;
}


//**************************************************************************************************
// Class: RetroRadioInput.physRead()                                                               *
//**************************************************************************************************
//  - Virtual method, must be overidden by subclasses. Returns an int16 as current read value.     *
//  - Invalid, if below 0                                                                          *
//**************************************************************************************************
int16_t RetroRadioInput::physRead() {
  return -1;
}

//**************************************************************************************************
// Class: RetroRadioInput.Destructor                                                               *
//**************************************************************************************************
RetroRadioInput::~RetroRadioInput() { 
  if (_id)
    free(_id);
  if (_name)
    free(_name);
};


//To be documented....
int16_t RetroRadioInput::read(bool show) {
  int16_t x;
  bool found = false;
  int16_t nearest = 0;
  if (!valid())
    return INT16_MIN;
  x = physRead();
  if (show)
    doprint("PhysRead = %d, ValueMapSize =%d (_show=%ld, show=%d)", x, _valueMap.size(), _show, show);
  if ((x >= 0) && (_valueMap.size() >= 4)) {
    size_t idx = 0;
    uint16_t maxDelta = UINT16_MAX;
    while (idx < _valueMap.size() && !found) {
      int16_t c1, c2;
      int16_t x1, x2, y1, y2;
      x1 = _valueMap[idx++];
      x2 = _valueMap[idx++];
      y1 = _valueMap[idx++];
      y2 = _valueMap[idx++];  
      c1 = x1 = (x1 < 0?_maximum+x1:x1);
      c2 = x2 = (x2 < 0?_maximum+x2:x2);
      if (c1 > c2) {
        int16_t t = c1;
        c1 = c2;
        c2 = t;
      }
      if ((x >= c1) && (x <= c2)) {
        found = true;
        if (show)
          if (_calibrate)
            doprint("Compare success at valueMap[%d] %d < %d < %d ?= map(%d, %d, %d, %d, %d)", (idx - 4) / 4, c1, x, c2, x, x1, x2, y1, y2);
        
        if (y1 == y2)
          x = y1;
        else 
          x = map(x, x1, x2, y1, y2);
      } else {
        int16_t newNearest;
        uint16_t myDelta = x < c1?c1 - x: x - c2;
        if (x < c1) {
          newNearest = -y1 - 1;
          myDelta = c1 - x;  
        } else {
          newNearest = -y2 - 1;
          myDelta = x - c2;
        }
        if (myDelta < maxDelta) {
          maxDelta = myDelta;
          nearest = newNearest;
        }
      }
    }; 
  }
  if (!found & (0 != nearest)) {
    if (show) 
      if (_calibrate)
        doprint("No direct match in map, nearest entry found is: %d", -nearest - 1);
    x = nearest;
  }
  return x;
}


void RetroRadioInput::check() {
//todo: filtern?
//todo: forcedRead
bool forced = _force;
bool showflag = false;
  _force = false;
  if (_show || forced)
    showflag = forced || ((millis() - _lastShowTime > _show));
  if (showflag)
    _lastShowTime = millis();
int16_t x = read(showflag);
  if ((x == INT16_MIN))
    return;
  if (!forced) {
    if (x == _lastRead) {
      return;
      _lastReadTime = millis();
      _lastUndebounceRead = x;
    } else if (x != _lastUndebounceRead) {
      _lastReadTime = millis();
      _lastUndebounceRead = x;
      return;
    } else if (millis() - _lastReadTime < _debounceTime)
      return;
  }  
  // Here we now have an debounced read that was different to the last one...
  if (forced || ((x >= 0) /*&& (_lastRead >= 0)*/ && (abs(_lastRead - x) >= _delta))) {
     if (_calibrate > 0)
       doprint("Change of Input_%s (from %d) to %d", getId(), _lastRead, x); 
     if ((x < 0) && forced)
        x = -x - 1;
      if (x >= 0) {
        int delta = abs(x - _lastRead);
        if (!forced && (_step > 0) && (delta >= _step) && (_lastRead >= 0))
          if (x > _lastRead)
            if ((delta > _fastStep) && (_fastStep > _step))
              x = _lastRead + _fastStep;
            else
              x  = _lastRead + _step;
          else {
            if ((_fastStep > _step) && (delta > _fastStep))
              x = _lastRead - _fastStep;
            else
              x  = _lastRead - _step;
            if (x < 0)
              x = 0;
          }
        if (_calibrate > 0)
          doprint("We have an event here: %s=%d", getId(), x);
        _lastRead = x;
        if (0 == strcmp("a33", getId())) {
          if (x <= 100) {
            char s[20];
            sprintf(s,"volume=%d", x);
            analyzeCmd(s); 
          }
        } else if (0 == strcmp("t6", getId())) {
          if (x <= 100) {
            char s[20];
            sprintf(s,"preset=%d", x);
            analyzeCmd(s); 
          }
        } else if (0 == strcmp("a35", getId())) {
          if (x <= 100) {
            char s[20];
            sprintf(s,"mp3track=0", x);
            analyzeCmd(s); 
          }
        }
      }
  } 
}

RetroRadioInputADC::RetroRadioInputADC(const char* id):RetroRadioInput(id) {
  _valid = true;
  _pin = atoi(id + 1);
  Serial.println("Now setParams for ADC should follow!");
  setParameters("@/inputa");
}

int16_t RetroRadioInputADC::physRead() {
  return analogRead(_pin); 
}

RetroRadioInputTouch::RetroRadioInputTouch(const char* id):RetroRadioInput(id), _auto(false) {
  _pin = (touch_pad_t)atoi(id + 1);
  if (_valid = (_pin < TOUCH_PAD_MAX)) {
     touch_pad_config(_pin, 0);
     Serial.println("Now setParams for Touch should follow!");
      setParameters("@/inputt");
  }
}

int16_t RetroRadioInputTouch::physRead() {
  uint16_t tuneReadValue;
  touch_pad_read_filtered(_pin, &tuneReadValue);
  if (_auto && (tuneReadValue < _maximum))
    _maximum--;
  return tuneReadValue;
}


//**************************************************************************************************
// Class: RetroRadioInputTouch.setParameter(String param, String value, int32_t ivalue)            *
//**************************************************************************************************
//  - overrides but uses method from base function                                                 *
//  - only addition is the parameter "auto"                                                        *
//**************************************************************************************************
void RetroRadioInputTouch::setParameter(String param, String value, int32_t ivalue) {
  if (param == "auto") {
    _auto = (bool)ivalue ;
  } else 
    RetroRadioInput::setParameter(param, value, ivalue);
}
