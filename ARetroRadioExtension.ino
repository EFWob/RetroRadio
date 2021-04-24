#if defined(RETRORADIO)
#include "ARetroRadioExtension.h"
#include <nvs.h>

std::vector<int16_t> channelList;     // The channels defined currently (by command "channels")
int16_t currentChannel = 0;           // The channel that has been set by command "channel" (0 if not set, 1..numChannels else)
int16_t numChannels = 0;              // Number of channels defined in current channellist

int readUint8(void *);                // Takes a void pointer and returns the content, assumes ptr to uint8_t
int readInt16(void *);                // Takes a void pointer and returns the content, assumes ptr to int16_t
int readSysVariable(const char *n);   // Read current value of system variable by given name (see table below for mapping)

#define KNOWN_SYSVARIABLES   (sizeof(sysVarReferences) / sizeof(sysVarReferences[0]))

struct {                           // A table of known internal variables that can be read from outside
  const char *id;
  void *ref;
  int (*readf)(void*);
} sysVarReferences[8] = {
  {"volume", &ini_block.reqvol, readUint8},
  {"preset", &ini_block.newpreset, readInt16},
  {"toneha", &ini_block.rtone[0], readUint8},
  {"tonehf", &ini_block.rtone[1], readUint8},
  {"tonela", &ini_block.rtone[2], readUint8},
  {"tonelf", &ini_block.rtone[3], readUint8},
  {"channel", &currentChannel, readInt16},
  {"channels", &numChannels, readInt16}
};


// Read current value of system variable by given name (see table sysvarreferences)
// Returns 0 if variable is not defined

int readSysVariable(const char *n) {
  for (int i = 0; i < KNOWN_SYSVARIABLES; i++)
    if (0 == strcmp(sysVarReferences[i].id, n))
      return sysVarReferences[i].readf(sysVarReferences[i].ref);
  return 0;
}

int readUint8(void *ref) {
  return (int) (*((uint8_t*)ref));
}

int readInt16(void *ref) {
  return (int) (*((uint16_t*)ref));
}
#define KNOWN_SYSVARIABLES =  (sizeof(sysVarReferences) / sizeof(sysVarReferences[0]))



// Obsolete?
void readDataList(std::vector<int16_t>& v, String value) {
  char *s = strdup(value.c_str());
  if (s) {
    char *first = s;
    while (first) {
      char * next = strchr(first, ',');
      if (NULL != next) {
        *next = 0;
        next++;
      }
      v.push_back(atoi(first));
      first = next;
    }

    free(s);
  }
}



//**************************************************************************************************
//                                          D O P R I N T                                          *
//**************************************************************************************************
// Copy of dbgprint. Only difference: will always print but only prefix "D: ", if DEBGUG==1        *
//**************************************************************************************************
char* doprint ( const char* format, ... )
{
  static char sbuf[2 * DEBUG_BUFFER_SIZE] ;                // For debug lines
  va_list varArgs ;                                    // For variable number of params

  va_start ( varArgs, format ) ;                       // Prepare parameters
  vsnprintf ( sbuf, sizeof(sbuf), format, varArgs ) ;  // Format the message
  va_end ( varArgs ) ;                                 // End of using parameters
  if ( DEBUG )                                         // DEBUG on?
  {
    Serial.print ( "D: " ) ;                           // Yes, print prefix
  }
  Serial.println ( sbuf ) ;                            // always print the info
  Serial.flush();
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


bool getPairInt16(String& value, int16_t& x1, int16_t& x2, bool duplicate, char delim) {
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

String extractgroup(String& inValue) {
  String(ret);
  chomp(inValue);
  if (inValue.c_str()[0] != '(') {
    ret = inValue;
    inValue = "";
  } else {
    const char *p = inValue.c_str() + 1;
    int nesting = 1;
    while (*p && nesting) {
      if (*p == '(')
        nesting++;
      else if (*p == ')')
        nesting--;
      if (nesting)
        p++;
    }
    ret = inValue.substring(0, p - inValue.c_str());
    if (*p)
      inValue = String(p + 1);
    else
      inValue = "";
  }
  return ret;
}

//**************************************************************************************************
//                          P A R S E G R O U P                                                    *
//**************************************************************************************************
// Extracts the left and the right part of a group                                                 *
//  - A group is considered to be enclosed by round paranthesis ()                                 *
//  - Groups can be nested.                                                                        *
//  - The opening paranthesis is optional but expected to be the first char of input value         *
//  - If no closing paranthesis is found, full input value is considered to be the group           *
//  - Remaining characters are ignored.                                                            *
//  - String inValue is set to the contents of remainder after the group                           *
//  - Group content (if any) is then split in left and right part by the strings contained in the  *
//    delimters arry. Last entry in delimiters arry must be NULL to denote end of delimiters       *
//  - If no delimiter is found, right part will be empty String("")                                *
//  - returns the index of used delimiter within array of delimters or < 0 if not found            *
//  - If delimiters==NULL, left will be set to full group (and -1 is returned)                     *
//  - If -1 is returned, right will be empty String("")                                            *
//  - All altered Strings (inValue, left, right) will be chomped upon return                       *
//  - If extendNvs (defaults to false) is true, left and right will be assumed to be reference to  *
//    nvs-key if preceeded by '@'                                                                  *
//**************************************************************************************************
int parseGroup(String & inValue, String& left, String& right, char** delimiters = NULL, bool extendNvs = false) {
  int idx = -1, i;
  bool found;
  chomp(inValue);
  int nesting = 0;
  bool commandGroup = (inValue.c_str()[0] == '{');
  char groupStart = '(';
  char groupEnd = ')';
  if ((inValue.c_str()[0] == '(') || commandGroup) {
    if (commandGroup) {
      groupStart = '{';
      groupEnd = '}';
    }
    inValue = inValue.substring(1);
    chomp(inValue);
    nesting = 1;
  }
  //    else return -1;
  const char *p = inValue.c_str();
  idx = -1;
  while (*p && (idx == -1)) {
    if (*p == groupStart)
      nesting++;
    else if (*p == groupEnd) {
      if (nesting <= 1)
        idx = p - inValue.c_str();
      else
        nesting--;
    }
    p++;
  }
  //  idx = inValue.indexOf(')');
  if (idx == -1) {
    left = inValue;
    inValue = "";
  } else {
    left = inValue.substring(0, idx);
    chomp(left);
    inValue = inValue.substring(idx + 1);
    chomp(inValue);
  }
  found = false;
  i = 0;
  if (delimiters)
    while (!found && (delimiters[i] != NULL))
      if (!(found = (-1 != (idx = left.indexOf(delimiters[i])))))
        i++;
  if (found) {
    right = left.substring(idx + strlen(delimiters[i]));
    left = left.substring(0, idx);
    if (extendNvs) {
      chomp_nvs(left);
      chomp_nvs(right);
    } else {
      chomp(left);
      chomp(right);
    }
    return i;
  } else {
    right = "";
    if (extendNvs)
      chomp_nvs(left);
    return -1;
  }
}


#ifdef OLD
int parseGroup(String & inValue, String& left, String& right, char** delimiters = NULL, bool extendNvs = false) {
  int idx, i;
  bool found;
  chomp(inValue);
  int nesting = 0;
  if (inValue.c_str()[0] == '(') {
    inValue = inValue.substring(1);
    chomp(inValue);
    nesting = 1;
  }
  const char *p = inValue.c_str();
  idx = -1;
  while (*p && (idx == -1)) {
    if (*p == '(')
      nesting++;
    else if (*p == ')') {
      if (nesting <= 1)
        idx = p - inValue.c_str();
      else
        nesting--;
    }
    p++;
  }
  //  idx = inValue.indexOf(')');
  if (idx == -1) {
    left = inValue;
    inValue = "";
  } else {
    left = inValue.substring(0, idx);
    chomp(left);
    inValue = inValue.substring(idx + 1);
    chomp(inValue);
  }
  found = false;
  i = 0;
  if (delimiters)
    while (!found && (delimiters[i] != NULL))
      if (!(found = (-1 != (idx = left.indexOf(delimiters[i])))))
        i++;
  if (found) {
    right = left.substring(idx + strlen(delimiters[i]));
    left = left.substring(0, idx);
    if (extendNvs) {
      chomp_nvs(left);
      chomp_nvs(right);
    } else {
      chomp(left);
      chomp(right);
    }
    return i;
  } else {
    right = "";
    if (extendNvs)
      chomp_nvs(left);
    return -1;
  }
}
#endif

int doCalculation(String& value, bool show, const char* ramKey = NULL) {
  char *operators[] = {"==", "!=", "<=", "><" , ">=", "<", "+", "^", "*", "/", "%", "&&", "&", "||", "|", "-", ">", NULL};
  String opLeft, opRight;
  int idx = parseGroup(value, opLeft, opRight, operators, true);
  int op1, op2, ret = 0;
  if (show) doprint("Calculate: (\"%s\" [%d] \"%s\")", opLeft.c_str(), idx, opRight.c_str());
  if (opLeft.c_str()[0] == '(')
    op1 = doCalculation(opLeft, show);
  else
    op1 = opLeft.toInt();
  if (opRight.c_str()[0] == '(')
    op2 = doCalculation(opRight, show);
  else
    op2 = opRight.toInt();
  switch (idx) {
    case -1:
      ret = op1;
      break;
    case 0:
      ret = op1 == op2;
      break;
    case 1:
      ret = op1 != op2;
      break;
    case 5:
      ret = op1 < op2;
      break;

    case 2:
      ret = op1 <= op2;
      break;
    case 3:
      ret = random(op1, op2 + 1);
      break;
    case 4:
      ret = op1 >= op2;
      break;
    case 6:
      ret = op1 + op2;
      break;
    case 7:
      ret = op1 ^ op2;
      break;
    case 8:
      ret = op1 * op2;
      break;
    case 9:
      ret = op1 / op2;
      break;
    case 10:
      ret = op1 % op2;
      break;
    case 11:
      ret = op1 && op2;
      break;
    case 12:
      ret = op1 & op2;
      break;
    case 13:
      ret = op1 || op2;
      break;
    case 14:
      ret = op1 | op2;
      break;
    case 15:
      ret = op1 - op2;
      break;
    case 16:
      ret = op1 > op2;
      break;

  }
  if (show)
    doprint("Caclulation result=%d", ret);
  if (ramKey)
    if (strlen(ramKey)) {
      ramsetstr(ramKey, String(ret));
      if (show) doprint("Calculation result set to ram.%s=%d", ramKey, ret);
    }
  return ret;
}

void doIf(String value, bool show) {
  String ifCommand, elseCommand;
  String dummy;
  int isTrue;
  if (show) doprint("Start if=\"%s\"\r\n", value.c_str());
  isTrue = doCalculation(value, show);
  parseGroup(value, ifCommand, dummy);
  parseGroup(value, elseCommand, dummy);
  dummy = String(isTrue);
  if (show) {
    doprint("  IfCommand = \"%s\"", ifCommand.c_str());
    doprint("ElseCommand = \"%s\"", elseCommand.c_str());
    doprint("Condition is %s (%d)", isTrue ? "TRUE" : "false", isTrue); Serial.flush();
  }
  if (isTrue) {
    chomp_nvs(ifCommand, dummy.c_str());
    if (show) doprint("Running \"if(true)\" with (expanded) command \"%s\"", ifCommand.c_str());
    analyzeCmdsRR(ifCommand);
  } else {
    chomp_nvs(elseCommand, dummy.c_str());
    if (show) doprint("Running \"else(false)\" with (expanded) command \"%s\"", elseCommand.c_str());
    analyzeCmdsRR(elseCommand);
  }
}

void doCalc(String value, bool show) {
  String calcCommand;
  String dummy;
  int result;
  if (show) doprint("Start calc=\"%s\"\r\n", value.c_str());
  result = doCalculation(value, show);
  parseGroup(value, calcCommand, dummy);
  dummy = String(result);
  if (show) {
    doprint("CalcCommand = \"%s\"", calcCommand.c_str());
  }
  chomp_nvs(calcCommand, dummy.c_str());
  if (show) doprint("Running \"calc\" with (expanded) command \"%s\"", calcCommand.c_str());
  analyzeCmdsRR(calcCommand);
}

void doIf(String condition, String value, bool show, String ramKey) {
  if (show) doprint("Start if(%s)=\"%s\"\r\n", condition.c_str(), value.c_str());
  int result = doCalculation(condition, show, ramKey.c_str());

  String ifCommand, elseCommand;
  String dummy;
  parseGroup(value, ifCommand, dummy);
  parseGroup(value, elseCommand, dummy);
  dummy = String(result);
  if (show) {
    doprint("  IfCommand = \"%s\"", ifCommand.c_str());
    doprint("ElseCommand = \"%s\"", elseCommand.c_str());
    doprint("Condition is %s (%d)", (result != 0) ? "TRUE" : "false", result); Serial.flush();
  }
  if (result != 0) {
    chomp_nvs(ifCommand, dummy.c_str());
    if (show) doprint("Running \"if(true)\" with (expanded) command \"%s\"", ifCommand.c_str());
    analyzeCmdsRR ( ifCommand );
  } else {
    chomp_nvs(elseCommand, dummy.c_str());
    if (show) doprint("Running \"else(false)\" with (expanded) command \"%s\"", elseCommand.c_str());
    analyzeCmdsRR ( elseCommand );
  }
}

void doCalc(String expression, String value, bool show, String ramKey) {
  if (show) doprint("Start calc(%s)=\"%s\"\r\n", expression.c_str(), value.c_str());
  int result = doCalculation(expression, show, ramKey.c_str());

  String calcCommand;
  String dummy;

  parseGroup(value, calcCommand, dummy);
  dummy = String(result);
  if (show) {
    doprint("CalcCommand = \"%s\"", calcCommand.c_str());
  }
  chomp_nvs(calcCommand, dummy.c_str());
  if (show) doprint("Running \"calc\" with (expanded) command \"%s\"", calcCommand.c_str());
  analyzeCmdsRR ( calcCommand );
}

void doWhile(String conditionOriginal, String valueOriginal, bool show, String ramKey) {
  bool done = false;
  if (show) doprint("Start while(%s)=\"%s\"\r\n", conditionOriginal.c_str(), valueOriginal.c_str());
  do {
    String value = valueOriginal;
    String condition = conditionOriginal;
    int result = doCalculation(condition, show, ramKey.c_str());
    done = (0 == result);
    if (!done) {
      String whileCommand, dummy;
      parseGroup(value, whileCommand, dummy);
      if (show)
        doprint("while(%d)=\"%s\"", result, whileCommand.c_str());
      dummy = String(result);
      chomp_nvs(whileCommand, dummy.c_str());
      if (show) doprint("Running \"while\" with (expanded) command \"%s\"", whileCommand.c_str());
      analyzeCmdsRR ( whileCommand );
    }
  } while (!done);
}

void doCalcIfWhile(String command, const char *type, String value) {
  /*
    doprint("Start doCalcIfWhile");
    delay(1000);
    doprint("Parameters are: command=%s, type=%s, value=%s", command.c_str(), type, value.c_str());
    delay(1000);
  */
  String ramKey, cond, dummy;
  const char *s = command.c_str() + strlen(type);
  bool show = *s == 'v';
  if (show)
    s++;
  char *condStart = strchr(s, '(');
  if (!condStart) {
    if (show)
      doprint("No condition/expression found for %s=()", type);
    return;
  } else {
    dummy = String(condStart);
    parseGroup(dummy, cond, dummy);
  }
  if (*s == '.') {
    ramKey = String(s + 1);
    ramKey = ramKey.substring(0, ramKey.indexOf('('));
    s = s + ramKey.length();
    ramKey.trim();
  }
  /*
    if (value.c_str()[0] != '(') {
      if (show)
        doprint("No condition/expression found for %s=()", type);
      return;
    }
    //  doprint("In doCalcIfWhile %s=%s", type, value.c_str());
    parseGroup(value, cond, dummy);
  */

  if (show)
    doprint("ParseResult: %s(%s)=%s; Ramkey=%s", type, cond.c_str(), value.c_str(), ramKey.c_str());
  switch (*type) {
    case 'w':
      doWhile(cond, value, show, ramKey);
      break;
    case 'c':
      doCalc(cond, value, show, ramKey);
      break;
    case 'i':
      doIf(cond, value, show, ramKey);
      break;

  }
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


std::vector<RetroRadioInput *> RetroRadioInput::_inputList;

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
RetroRadioInput::RetroRadioInput(const char* id): _show(0), _lastShowTime(0), _reader(NULL), _lastInputType(NONE), _mode(0),
  _onChangeEvent(NULL), _on0Event(NULL), _onNot0Event(NULL),
  _id(NULL), _debounceTime(0), /*_calibrate(false),*/ _delta(1), _step(0), _fastStep(0) {
  _maximum = _lastRead = _lastUndebounceRead = INT16_MIN;                 // _maximum holds the highest value ever read, _lastRead the
  // last read value (after mapping. Is below 0 if no valid read so far)
  setId(id);
  _inputList.push_back(this);
  //setParameters("@/input");
}


//**************************************************************************************************
// Class: RetroRadioInput.get(const char *id)                                                      *
//**************************************************************************************************
//  - Returns the handle to the RetroRadioInput identified by 'id'                                 *
//  - If id is not known, create new input handler and add it to the list of known handlers        *
//**************************************************************************************************
RetroRadioInput* RetroRadioInput::get(const char *id) {
  String idStr = String(id);
  RetroRadioInput *p;
  chomp(idStr);
  idStr.toLowerCase();
  if (idStr.length() == 0)
    return NULL;
  for (int i = 0; i  < _inputList.size(); i++)
    if (strcmp(_inputList[i]->getId(), idStr.c_str()) == 0)
      return _inputList[i];
  return new RetroRadioInput(id);
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
// Class: RetroRadioInput.setId()                                                                  *
//**************************************************************************************************
//  - Sets the id of the RetroRadioInput                                                        *
//**************************************************************************************************
void RetroRadioInput::setId(const char *id) {
  if (_id)
    free(_id);
  _id = NULL;
  if (id)
    if (strlen(id))
      _id = strdup(id);
}

//**************************************************************************************************
// Class: RetroRadioInput.getEvent(String type)                                                    *
//**************************************************************************************************
//  - Returns a pointer to the name of the requested RetroRadioInput change event                  *
//  - type can be "change" (for "onchange"), "0", "not0"                                           *
//  - returns NULL if type is not valid.                                                           *
//**************************************************************************************************
char** RetroRadioInput::getEvent(String type) {
  if (type == "change")
    return &_onChangeEvent;
  else if (type == "0")
    return &_on0Event;
  else if (type == "not0")
    return &_onNot0Event;
  return NULL;
}

//**************************************************************************************************
// Class: RetroRadioInput.setEvent(String type, const char* name)                                  *
//**************************************************************************************************
//  - Sets one of the (change) events of the RetroRadioInput object.                               *
//  - type can be "change" (for "onchange"), "0", "not0"                                           *
//**************************************************************************************************
void RetroRadioInput::setEvent(String type, const char* name) {
  char **event = NULL;
  event = getEvent(type);
  if (event) {
    if (*event)
      free(*event);
    *event = NULL;
    if (name)
      if (strlen(name))
        *event = strdup(name);
  }
}

//**************************************************************************************************
// Class: RetroRadioInput.setParameters(String value)                                              *
//**************************************************************************************************
//  - Sets one or more parameters of the Input objects                                             *
//  - Multiple parameters can be seperated by ','                                                  *
//  - If a parameter starts with '@' it is considered to reference an NVS-key (w/o leading '@') to *
//    read parameter(s) from                                                                       *
//**************************************************************************************************
void RetroRadioInput::setParameters(String params) {
  static int recursion = 0;
  /*
    doprint("Set Parameters!");
    doprint("Parameter id=%s", getId());
    doprint("Parameter value=%s", params.c_str());
    doprint("SetParameters for input%s=%s", getId(), params.c_str());
  */
  if (recursion > 5)
    return;
  recursion++;
  chomp(params);
  while (params.length()) {                             // anything left
    String value = getStringPart(params, ',');          // separate next parameter
    int ivalue;
    String param = getStringPart(value, '=');         // see if value is set
    if (param.c_str()[0] == '@') {                       // reference to NVS or RAM?
      chomp_nvs(param); setParameters(param.c_str());         // new version...
      // old version... setParameters(nvsgetstr(param.c_str() + 1));             // if yes, read NVS from key
    } else {
      value.toLowerCase();
      if ((value.c_str()[0] == '@') || (param != "src"))
        chomp_nvs(value);                                 // new version
      else
        chomp(value);                                     // chompNVS would destroy ~ for src link...
      //old chomp(value);
      //old value.toLowerCase();
      //old if (value.c_str()[0] == '@')
      //old   value = nvsgetstr(value.c_str() + 1);
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
  } else if (param == "map+") {                       // extent the valueMap for the input
    setValueMap(value, true);                         // see RetroRadioInput::setValueMap() for details
  } else if ((param == "event") || (param == "onchange")) {                      // set the change event
    setEvent("change", value.c_str());                  // on mapped input, if not hit, nearest valid value will be used
    //  } else if (param == "calibrate") {                  // as "show", just more details on map hits.
    //    _calibrate = ivalue;
  } else if (param.startsWith("on")) {                      // set the change event
    setEvent(param.substring(2), value.c_str());                          // on mapped input, if not hit, nearest valid value will be used
    //  } else if (param == "calibrate") {                  // as "show", just more details on map hits.
    //    _calibrate = ivalue;
  } else if (param == "debounce") {                     // how long must a value be read before considered valid?
    _debounceTime = ivalue;                           // (in msec).
  } else if (param == "delta") {
    if (ivalue > 1)
      _delta = ivalue;
    else
      _delta = 1;
  } else if (param == "mode") {
    _mode = ivalue;
    if (_reader)
      _reader->mode(_mode);
  } else if ((param == "nvs") || (param == "ram")) {
    if (value.length())
      if (_reader) {
        int x = _lastRead;
        if (_lastInputType == NONE) {
          x = read(false);
        }
        if (param == "nvs")
          nvssetstr(value.c_str(), String(x));
        else
          ramsetstr(value.c_str(), String(x));
      }
  } else if (param == "src") {
    setReader(value);
  } else if (param == "start") {
    _lastInputType = NONE;
    //Serial.printf("Executing Start for in.%s\r\n", getId());
    read(true);
    //Serial.printf("Start for in.%s Done!\r\n", getId());
  } else if (param == "stop") {
    _lastInputType = NONE;
  } else if (param == "info") {
    showInfo(true);
  }
}

//**************************************************************************************************
// Class: RetroRadioInput::showInfo(bool hint)                                                     *
//**************************************************************************************************
//  - Displays (all) relevant settings for the Input on Serial (even if DEBUG = 0)                 *
//  - Can be used for debugging (to see if all settings are as expected), no functional impact     *
//  - Will not read the input (for this, set the "property" 'show' to a value > 0 to have cyclic   *
//    display of the readings (with distance between readings set by the value assigned to 'show'  *
//  - If 'hint' is true, an info is displayed showing all other known inputs                       *
//**************************************************************************************************
void RetroRadioInput::showInfo(bool hint) {
  doprint("Info for Input \"in.%s\":", getId());
  if (_reader == NULL)
    doprint(" * No source linked, Input not operational!");
  else {
    doprint(" * Src: %s", _reader->info().c_str());
    if (_lastInputType == NONE)
      doprint(" * Input is not started (no cyclic polling)");
    else
      doprint(" * Cyclic polling is active");
  }
  if (_valueMap.size() >= 4) {
    doprint(" * Value map contains %d entries:", _valueMap.size() / 4);
    for (int i = 0; i < _valueMap.size(); i += 4)
      doprint("    %3d: (%d..%d = %d..%d)", 1 + i / 4,
              _valueMap[i], _valueMap[i + 1], _valueMap[i + 2], _valueMap[i + 3]);
  } else
    doprint(" * Value map is NOT set!");
  if (!hasChangeEvent()) {
    doprint(" * no on-event(s) defined.");
  } else for (int i = 0; i < 3; i++) {
      const char* type = (i == 0) ? "change" : (i == 1 ? "0" : "not0");
      char **cmnds = getEvent(type);
      if (*cmnds) {
        doprint(" * on%s-event: \"%s\"", type, *cmnds);
        if (ramsearch(*cmnds)) {
          String commands = ramgetstr(*cmnds);
          chomp_nvs(commands);
          doprint("    defined in RAM as: \"%s\"", commands.c_str());
        } else if (nvssearch(*cmnds)) {
          String commands = nvsgetstr(*cmnds);
          //chomp_nvs(commands);
          doprint("    defined in NVS as: \"%s\"", commands.c_str());
        }  else
          doprint("    NOT DEFINED! (Neither NVS nor RAM)!");
      }
    }
  doprint(" * Delta: %d", _delta);
  doprint(" * Show: %d (seconds)", _show / 1000);
  doprint(" * Debounce: %d (ms)", _debounceTime);
  if (hint)
    if (_inputList.size() > 1) {
      doprint("There are %d more Inputs defined:", _inputList.size() - 1);
      int n = 0;
      for (int i = 0; i < _inputList.size(); i++)
        if (_inputList[i] != this)
          doprint(" %2d: \"in.%s\"", ++n, _inputList[i]->getId());
    }
}

//**************************************************************************************************
// Class: RetroRadioInput::setValueMap(String value, bool extend)                                  *
//**************************************************************************************************
//  - Set the mapping table of the RetroRadioInput object                                          *
//  - If extend is true, the existing map is extended (default = false will clear existing map)    *
//  - more than one mapping entry can be added, each entry must be surrounded by '( )'             *
//  - if value (after chomp) starts with @ the remaining string is treated as key to an NVS entry  *
//    from preferences or RAM entry if no NVS entry is associated with that key                    *
//  - One single entry is expected to have the format:                                             *
//        x1[..x2] = y1[..y2]                                                                      *
//    Interpretation: if read is called, the phyical value obtained from physRead()                *
//    is compared with all defined map-entries if it fits within x1 and x2 (x2 can be below x1)    *
//    x1 and x2 are inclusive, if x2 is not provided it will be set equal to x1.                   *
//    If there is a hit, x will be mapped to y1 to y2. (if y2 is not provided, y2 will set to y1)  *
//    both x and y range can be in reverse order (but for y must not be below 0)                   *
//    Examples:                                                                                    *
//       (0 = 1) (1 = 0) (expanded to (0..0 = 1..1)(1..1 = 0..0) will iverse a digital input       *
//       (0..1 = 1..0) would do the same with just one map entry                                   *
//       0, 4095 = 100, 0  will convert input values between 0-4095 (i. e. from analog input) to   *
//                 an output range of 100 to 0                                                     *
//  - A map entry with empty left part is also possible. In that case the left part will be set to *
//    x1 = INT16_MIN and x2 = INT16_MAX. This cancels also the map setting, as this entry will     *
//    cover the full possible input range.                                                         *
//  - In case of overlapping map ranges, the first defined map enty will fire. Example:            *
//    (1000..3000 = 1)(=2)(4000..4050=3)                                                           *
//    will yield 1 for all readings between 1000 and 3000 (inclusive), and 2 in any other case     *
//    (the third entry will be ignored since the range between 4000 and 4050 is also covered by    *
//    the second
//  - calling setMap will erase the old mapping                                                    *
//**************************************************************************************************
void RetroRadioInput::setValueMap(String value, bool extend) {
  chomp_nvs(value);
  value.toLowerCase();
  if (!extend)
    _valueMap.clear();
  while (value.length() > 0) {
    String mapEntryLeft, mapEntryRight;
    char *delimiters[] = {"=", NULL};
    int idx = parseGroup(value, mapEntryLeft, mapEntryRight, delimiters);
    if (idx == 0) {
      char *delimiters[] = {"..", NULL};
      String from1, from2, to1, to2;
      if (mapEntryLeft.length() == 0) {
        from1 = String(INT16_MIN);
        from2 = String(INT16_MAX);
      }
      else if (-1 == parseGroup(mapEntryLeft, from1, from2, delimiters, true))
        from2 = from1;
      if (-1 == parseGroup(mapEntryRight, to1, to2, delimiters, true))
        to2 = to1;
      _valueMap.push_back(from1.toInt());                  // add to map if both x and y pair is valid
      _valueMap.push_back(from2.toInt());
      _valueMap.push_back(to1.toInt());
      _valueMap.push_back(to2.toInt());

      //Serial.printf("Map entry for input_%s: %d, %d = %d, %d\r\n", getId(), _valueMap[_valueMap.size() - 4],
      //                         _valueMap[_valueMap.size() - 3],_valueMap[_valueMap.size() - 2],_valueMap[_valueMap.size() - 1]);

    }
  }
  //_lastInputType = NONE;
}

#ifdef OLD
void RetroRadioInput::setValueMap(String value) {
  chomp(value);
  _valueMap.clear();

  //clearValueMap();
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
        x1 = 0; x2 = INT16_MAX; value = "";
      } else
        ok = getPairInt16(left, x1, x2, true);
      if (ok) {
        _valueMap.push_back(x1);                  // add to map if both x and y pair is valid
        _valueMap.push_back(x2);
        _valueMap.push_back(y1);
        _valueMap.push_back(y2);
        Serial.printf("Map entry for input_%s: %d, %d = %d, %d\r\n", getId(), _valueMap[_valueMap.size() - 4],
                      _valueMap[_valueMap.size() - 3], _valueMap[_valueMap.size() - 2], _valueMap[_valueMap.size() - 1]);
      }
    }
    chomp(value);
  } while (value.length() > 0);                   // repeat if there is more in the input
  _lastInputType = NONE;
}
#endif


class RetroRadioInputReaderDigital: public RetroRadioInputReader {
  public:
    RetroRadioInputReaderDigital(int pin, int mod) {
      _pin = pin;
      mode(mod);
    };
    int16_t read() {
      return _inverted ? !digitalRead(_pin) : digitalRead(_pin);
    };
    void mode(int mod) {
      _mode = mod;
      _inverted = 0 != (mod & 0b1);
      if (0 != (mod & 0b10))
        pinMode(_pin, INPUT_PULLUP);
      else
        pinMode(_pin, INPUT);
    }
    String info() {
      return String("Digital Input, pin: ") + _pin +
             ", Inverted: " + _inverted + ", PullUp: " + (0 != (_mode & 0b10));
    }

  private:
    int _pin;
    bool _inverted;
};

class RetroRadioInputReaderRam: public RetroRadioInputReader {
  public:
    RetroRadioInputReaderRam(const char *key): _key(NULL) {
      if (key)
        if (strlen(key))
          _key = strdup(key);
    };
    ~RetroRadioInputReaderRam() {
      if (_key)
        free((void *)_key);
    }
    int16_t read() {
      int16_t res = 0;
      String resultString;
      if (_key)
        resultString = ramgetstr(_key);
      res = atoi(resultString.c_str());
      return res;
    };
    void mode(int mod) {
    }
    String info() {
      return String("RAM Input, key: ") + (_key ? _key : "<null>");
    }

  private:
    const char *_key;
};


class RetroRadioInputReaderAnalog: public RetroRadioInputReader {
  public:
    RetroRadioInputReaderAnalog(int pin, int mod) {
      _pin = pin;
      _last = -1;
      _filter = 0;
      mode(mod);
    };
    void mode(int mod) {
      _mode = mod;
      _inverted = 0 != (mod & 0b1);
      if (0 != (mod & 0b10))
        pinMode(_pin, INPUT_PULLUP);
      else
        pinMode(_pin, INPUT);
      _filter = (0b11100 & mod) >> 2;
    }
    int16_t read() {
      uint32_t t = millis();
      if (_last >= 0)
        if ((t - _lastReadTime) < 20)
          return (_inverted ? (4095 - _last) : _last);
      _lastReadTime = t;
      int x = analogRead(_pin);
      //   Serial.printf("AnalogRead(%d) = %d\r\n", _pin, x);
      if ((_last >= 0) && (_filter > 0)) {
        _last = (_last * _filter + x) / (_filter + 1);
        if (_last < x)
          _last++;
        else if (_last > x)
          _last--;
      }
      else
        _last = x;
      return (_inverted ? (4095 - _last) : _last);
    };

    String info() {
      return String("Analog Input, pin: ") + _pin +
             ", Inverted: " + _inverted + ", PullUp: " + (0 != (_mode & 0b10)) + ", Filter: " + (7 & (_mode >> 2));
    }
  private:
    int _pin, _last, _filter;
    bool _inverted;
    uint32_t _lastReadTime;
};

#ifdef OLD
class RetroRadioInputReaderCapa: public RetroRadioInputReader {
  public:
    RetroRadioInputReaderCapa(int pin) {
      _pin = pin;
      touch_pad_config((touch_pad_t)pin, 0);

      _last = 0xffff;
    };
    int16_t read() {
      uint16_t x;
      touch_pad_read_filtered((touch_pad_t)_pin, &x);
      //      if (_last == 0xffff)
      _last = x;
      //      else
      //        _last = ((_last * 7) + x) / 4;
      return (_last);
    };
    void mode(int mod) {

    }
  private:
    int _pin;
    uint16_t _last;
};
#endif

class RetroRadioInputReaderTouch: public RetroRadioInputReader {
  public:
    RetroRadioInputReaderTouch(int pin, int mod) {
      _pin = pin;
      _auto = false;
      touch_pad_config((touch_pad_t)pin, 0);
      int i = 0;
      uint16_t x = 0;

      while ((i++ < 100) && (x == 0)) {   // Workaround: touch_pad_read_filtered() needs some time to settle... (will return 0 else)
        touch_pad_read_filtered((touch_pad_t)_pin, &x);
        delay(1);
      }
      //Serial.printf("TouchRead Nr. %d, T%d=%d\r\n", i, pin, x);
      mode(mod);
    };
    void mode(int mod) {
      bool wasAuto = _auto;
      _digital = (0 != (mod & 0b1));
      if (_digital)
        _auto = true;
      else
        _auto = (0 != (mod & 0b10));
      if (_auto && !wasAuto)
        _maxTouch = _minTouch = 50;
    }

    int16_t read() {
      uint16_t x;
      touch_pad_read_filtered((touch_pad_t)_pin, &x);
      if (x == 0) {
        return _digital ? 1 : 1023;
      }
      //        x = _maxTouch;
      /*
            static uint32_t lastShow = 0;
            if (millis() - lastShow > 1000) {
              Serial.printf("touch=%d, max=%d\r\n", x, _maxTouch);
              lastShow = millis();
            }
      */
      if (_auto) {
        if (x >= _maxTouch) {
          if (_minTouch == _maxTouch)
            if (0 > (_minTouch = x - 50)) {
              return _digital ? 1 : 1023;
            }
          _maxTouch = x;
        }
        else if (_maxTouch > _minTouch) {
          if (_minTouch > x)
            _minTouch = x;
          //      else if (x - _minTouch > 20)
          //        _minTouch++;
          if (_auto && (_maxTouch - x < 5) && (_maxTouch > 10))
            _maxTouch--;
        }

        x = map(x, _minTouch, _maxTouch, 0, 1023);
        return _digital ? (x >= 800) : x;
      }
      else
        return x;
    };
    String info() {
      //   doprint("TouchMin: %d, TouchMax: %d", _minTouch, _maxTouch);  //ToDo kann weg
      return String("Touch Input: T") + _pin + ", Digital use: " + _digital +
             ", Auto: " + _auto + (_auto ? " (auto calibration to 1023 if not touched)" : " (pin value is used direct w/o calibration)") ;
    }
  private:
    int _pin;
    bool _auto, _digital;
    int16_t _maxTouch, _minTouch;
};

class RetroRadioInputReaderSystem: public RetroRadioInputReader {
  public:
    RetroRadioInputReaderSystem(const char* reference): _readf(NULL), _refname(NULL), _ref(NULL) {
      int i;
      for (int i = 0; (_readf == NULL) && (i < sizeof(_references) / sizeof(_references[0])); i++)
        if (0 == strcmp(reference, _references[i].id))
          if ((NULL != _references[i].ref) && (NULL != _references[i].readf)) {
            _ref = _references[i].ref;
            _readf = _references[i].readf;
            _refname = _references[i].id;
          }
      if (NULL == _refname)
        _refname = strdup(reference);
      //Serial.printf("sytem[%s] has ref=%ld\r\n", reference, _ref);
    };

    void mode(int mod) {
    }

    int16_t read() {
      if (_readf)
        return _readf(_ref);
      return 0;
    };

    String info() {
      String ret;
      if (!_ref)
        ret = "Set to unknown ";
      ret = ret + "Variable: " + _refname;
      if (!_ref)
        ret += " (will always read as 0)";
      return ret;
    }

    static int readUint8(void *ref) {
      return (int) (*((uint8_t*)ref));
    }
    static int readInt16(void *ref) {
      return (int) (*((uint16_t*)ref));
    }

  private:
    struct {
      const char *id;
      void *ref;
      int (*readf)(void*);
    } _references[8] = {
      {"volume", &ini_block.reqvol, RetroRadioInputReaderSystem::readUint8},
      {"preset", &ini_block.newpreset, RetroRadioInputReaderSystem::readInt16},
      {"toneha", &ini_block.rtone[0], RetroRadioInputReaderSystem::readUint8},
      {"tonehf", &ini_block.rtone[1], RetroRadioInputReaderSystem::readUint8},
      {"tonela", &ini_block.rtone[2], RetroRadioInputReaderSystem::readUint8},
      {"tonelf", &ini_block.rtone[3], RetroRadioInputReaderSystem::readUint8},
      {"channel", &currentChannel, RetroRadioInputReaderSystem::readInt16},
      {"channels", &numChannels, RetroRadioInputReaderSystem::readInt16}
    };

    void *_ref;
    const char* _refname;
    int (*_readf)(void *);
};



void RetroRadioInput::setReader(String value) {
  RetroRadioInputReader *reader = NULL;
  chomp(value);
  value.toLowerCase();
  int idx = value.substring(1).toInt();
  Serial.printf("SetReader, value = %s\r\n", value.c_str());
  switch (value.c_str()[0]) {
    case 'd': reader = new RetroRadioInputReaderDigital(idx, _mode);
      break;
    case 'a': if ((_reader == NULL) && (_mode == 0))
        _mode = 0b11100;
      reader = new RetroRadioInputReaderAnalog(idx, _mode);
      break;
    /*
        case 'c': reader = new RetroRadioInputReaderCapa(idx);
                  break;
    */
    case 't': reader = new RetroRadioInputReaderTouch(idx, _mode);
      break;
    case '.': reader = new RetroRadioInputReaderRam(value.c_str() + 1);
      break;
    case '~': reader = new RetroRadioInputReaderSystem(value.c_str() + 1);
      break;
  }
  if (reader) {
    if (_reader)
      delete _reader;
    _reader = reader;
    _lastInputType = NONE;
    Serial.printf("Reader %lX set for %lX!", _reader, this);
  }
}

//**************************************************************************************************
// Class: RetroRadioInput.clearValueMap()                                                          *
//**************************************************************************************************
//  - Clears the value map, if that map is not empty. In that case, also _lastRead is reset, as    *
//    from now on the mapping used to calculate _lastRead is no longer valid                       *
//**************************************************************************************************
/*
  void RetroRadioInput::clearValueMap() {
  if (_valueMap.size()) {
    _valueMap.clear();
    _lastRead = _lastUndebounceRead = INT16_MIN;
  }
  _lastInputType = NONE;
  }
*/

//**************************************************************************************************
// Class: RetroRadioInput.valid()                                                                  *
//**************************************************************************************************
//  - Virtual method, can be overidden by subclasses. True, if a valid input is associated with    *
//    the calling object.                                                                          *
//    from now on the mapping used to calculate _lastRead is no longer valid                       *
//**************************************************************************************************
/*
  bool RetroRadioInput::valid() {
  return _valid;
  }
*/

//**************************************************************************************************
// Class: RetroRadioInput.physRead()                                                               *
//**************************************************************************************************
//  - Virtual method, must be overidden by subclasses. Returns an int16 as current read value.     *
//  - Invalid, if below 0                                                                          *
//**************************************************************************************************
int16_t RetroRadioInput::physRead() {
  if (!_reader) {
    _lastInputType = NONE;
    return 0;
  } else {
    if (_lastInputType == NONE)
      _lastInputType = HIT;
    return _reader->read();
  }
}

//**************************************************************************************************
// Class: RetroRadioInput.Destructor                                                               *
//**************************************************************************************************
RetroRadioInput::~RetroRadioInput() {
  if (_id)
    free(_id);
  if (_onChangeEvent)
    free(_onChangeEvent);
  if (_on0Event)
    free(_on0Event);
  if (_onNot0Event)
    free(_onNot0Event);
  if (_reader)
    delete _reader;
};

int RetroRadioInput::read(bool doStart) {
  bool forced = (_lastInputType == NONE);
  bool show = (_show > 0) && ((millis() - _lastShowTime > _show) || forced);
  String showStr;
  int16_t x = physRead();
  int16_t nearest;
  if (show)
    _lastShowTime = millis();
  if (NONE == _lastInputType) {
    if (show)
      doprint("Input \"in.%s\" fail: no source associated", getId());
    return 0;
  } else if (show) {
    char xbuf[10];
    sprintf(xbuf, "%5d", x);
    showStr = String("Input \"in.") + getId() + "\" (is " + (doStart ? "running" : "stopped") + ") physRead=" + xbuf;
  }

  bool found = true;
  if (_valueMap.size() >= 4) {
    size_t idx = 0;
    uint16_t maxDelta = UINT16_MAX;
    found = false;
    while (idx < _valueMap.size() && !found) {
      int16_t c1, c2;
      int16_t x1, x2, y1, y2;
      x1 = _valueMap[idx++];
      x2 = _valueMap[idx++];
      y1 = _valueMap[idx++];
      y2 = _valueMap[idx++];
      c1 = x1 = (x1 < 0 ? _maximum + x1 : x1);
      c2 = x2 = (x2 < 0 ? _maximum + x2 : x2);
      if (c1 > c2) {
        int16_t t = c1;
        c1 = c2;
        c2 = t;
      }
      if ((x >= c1) && (x <= c2)) {
        found = true;
        if (y1 == y2)
          x = y1;
        else
          x = map(x, x1, x2, y1, y2);
      } else {
        int16_t newNearest;
        uint16_t myDelta = x < c1 ? c1 - x : x - c2;
        if (x < c1) {
          //newNearest = -y1 - 1;
          newNearest = y1;
          myDelta = c1 - x;
        } else {
          //newNearest = -y2 - 1;
          newNearest = y2;
          myDelta = x - c2;
        }
        if (myDelta < maxDelta) {
          maxDelta = myDelta;
          nearest = newNearest;
        }
      }
    };
  }
  if (found) {
    if (show && (_valueMap.size() >= 4)) {
      char xbuf[10];
      sprintf(xbuf, "%5d", x);
      showStr = showStr + " ( mapped to:" + xbuf + ")";
    }
    _lastInputType = HIT;
  }
  else {
    if (show && (_valueMap.size() >= 4)) {
      char xbuf[10];
      sprintf(xbuf, "%5d", nearest);
      showStr = showStr + " (nearest is:" + xbuf + ")";
    }
    if (forced) {
      x = nearest;
      _lastInputType = NEAREST;
      /*      if (show) {// (_show > 0)
              char xbuf[10];
              sprintf(xbuf, "%5d)", x);
              showStr = showStr + " (mapped to nearest=" + xbuf;
            }
      */
    } else
      x = _lastRead;
  }
  int lastRead = 0;
  if (show)
    doprint(showStr.c_str());
  if (!doStart) {
    if (forced)
      _lastInputType = NONE;
    forced = false;
  } else if (forced) {
    _lastRead = _lastUndebounceRead = x;
    _lastReadTime = millis();
  } else {
    if (x != _lastUndebounceRead) {
      _lastUndebounceRead = x;
      _lastReadTime = millis();
      x = _lastRead;
    } else if (x != _lastRead) {
      if ((millis() - _lastReadTime < _debounceTime) || (_delta > abs(_lastRead - x)))
        x = _lastRead;
      else {
        lastRead = _lastRead;
        _lastRead = x;
        forced = true;
      }
    }
  }
  if (forced) {
    if (_show > 0)
      doprint("Input \"in.%s\": change to %d from %d, checking for events!", getId(), x, lastRead);
    char *cmnds = _onChangeEvent;
    if (cmnds) {
      if (_show > 0) {
        doprint("Input \"in.%s\": change to %d (event \"%s\")", getId(), x, cmnds);
      }

      String commands = ramsearch(cmnds) ? ramgetstr(cmnds) : nvsgetstr(cmnds); //nvssearch(specificEvent)?nvsgetstr(specificEvent):nvsgetstr(cmnds);
      Serial.printf("OnChangeCommand before chomp: %s\r\n", commands.c_str());
      chomp_nvs(commands, String(x).c_str());
      Serial.printf("OnChangeCommand after chomp: %s\r\n", commands.c_str());
      analyzeCmdsRR ( commands );
    }
    if (x == 0) {                   // went to 0
      cmnds = _on0Event;
      if (cmnds) {
        if (_show > 0) {
          doprint("Input \"in.%s\": became Zero (event \"%s\")", getId(), cmnds);
        }
        String commands = ramsearch(cmnds) ? ramgetstr(cmnds) : nvsgetstr(cmnds); //nvssearch(specificEvent)?nvsgetstr(specificEvent):nvsgetstr(cmnds);
        chomp_nvs(commands, String(x).c_str());
        analyzeCmdsRR ( commands );
      }
    } else if (lastRead == 0) {     // comes from 0
      cmnds = _onNot0Event;
      if (cmnds) {
        if (_show > 0) {
          doprint("Input \"in.%s\": became NonZero (event \"%s\")", getId(), cmnds);
        }
        String commands = ramsearch(cmnds) ? ramgetstr(cmnds) : nvsgetstr(cmnds); //nvssearch(specificEvent)?nvsgetstr(specificEvent):nvsgetstr(cmnds);
        chomp_nvs(commands, String(x).c_str());
        analyzeCmdsRR ( commands );
      }
    }
  }
  return x;
}

void RetroRadioInput::executeCmdsWithSubstitute(String commands, String substitute) {
  int idx = commands.indexOf('?');
  while (idx >= 0) {
    commands = commands.substring(0, idx) + substitute + commands.substring(idx + 1);
    idx = commands.indexOf('?');
  }
  analyzeCmdsRR ( commands );
}

bool RetroRadioInput::hasChangeEvent() {
  return (NULL != _onChangeEvent) || (NULL != _on0Event) || (NULL != _onNot0Event);
}

void RetroRadioInput::checkAll() {
  for (int i = 0; i < _inputList.size(); i++) {
    RetroRadioInput *p = _inputList[i];
    bool isRunning = p->_lastInputType != NONE;
    if ((isRunning && p->hasChangeEvent()) || ((p->_show > 0) && (millis() - p->_lastShowTime > p->_show))) {
      p->read(isRunning);
      //if (!isRunning)
      //Serial.printf("Input \"in.%s\" is STOPped!\r\n", p->getId());
      //p->_lastInputType = NONE;
    }
  }
}

void RetroRadioInput::showAll(const char *p) {
  Serial.printf("Running showAll! p=%ld\r\n", p);
  for (int i = 0; i < _inputList.size(); i++) {
    bool match = (p == NULL);
    if (!match)
      match = (strstr(_inputList[i]->getId(), p) != NULL);
    if (match)
      _inputList[i]->showInfo(false);
  }
}

void doInput(String id, String value) {
  RetroRadioInput *i = RetroRadioInput::get(id.c_str());
  if (i)
    i->setParameters(value);
}

/*
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
  if (false)  {
    if (x != _lastUndebounceRead) {
      _lastUndebounceRead = x;
      _lastReadTime = millis();
      x = _lastRead;
    } else if (x != _lastRead) {
      if (millis() - _lastReadTime < _debounceTime)
        x = _lastRead;
    }
  }
  return x;
  }



  void RetroRadioInput::check() {
  //todo: filtern?
  //todo: forcedRead
  bool forced = false;//_force;
  bool showflag = false;
  //  _force = false;
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
  if (forced || ((x >= 0)
  //&& (_lastRead >= 0)
  && (abs(_lastRead - x) >= _delta))) {
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
*/

std::map<String, String> ramContent;

//**************************************************************************************************
//                                      R A M G E T S T R                                          *
//**************************************************************************************************
// Read a string from RAM.                                                                         *
//**************************************************************************************************
String ramgetstr ( const char* key )
{
  auto search = ramContent.find(String(key));
  if (search == ramContent.end())
    return String("");
  else
    return search->second;
}


//**************************************************************************************************
//                                      R A M S E T S T R                                          *
//**************************************************************************************************
// Put a key/value pair in RAM.  Length is limited to allow easy read-back.                        *
//**************************************************************************************************
void ramsetstr ( const char* key, String val )
{
  ramContent[String(key)] = val;
}

//**************************************************************************************************
//                                      R A M S E A R C H                                          *
//**************************************************************************************************
// Check if key exists in RAM.                                                                     *
//**************************************************************************************************
bool ramsearch ( const char* key )
{
  auto search = ramContent.find(String(key));
  // if (search == ramContent.end()) Serial.printf("RAM search failed for key=%s\r\n", key);
  return (search != ramContent.end());
}

//**************************************************************************************************
//                                      R A M D E L K E Y                                          *
//**************************************************************************************************
// Delete a keyname in RAM.                                                                        *
//**************************************************************************************************
void ramdelkey ( const char* key)
{
  auto search = ramContent.find(String(key));
  dbgprint("RamDelKey(%s)=%d", key, search != ramContent.end());
  if (search != ramContent.end())
    ramContent.erase(search);
}


//**************************************************************************************************
//                                      D O R A M L I S T                                          *
//**************************************************************************************************
// - command "ramlist": list the full RAM content to Serial (even if DEBUG = 0)                    *
// - if parameter p != NULL, only show entries which have *p as substring in keyname               *
//**************************************************************************************************
void doRamlist(const char* p) {
  doprint("RAM content (%d entries)", ramContent.size());
  auto search = ramContent.begin();
  int i = 0;
  while (search != ramContent.end()) {
    bool match = (p == NULL);
    if (!match)
      match = (NULL != strstr(search->first.c_str(), p));
    if (match)
      doprint(" %2d: '%s' = '%s'", ++i, search->first.c_str(), search->second.c_str());
    search++;
  }
}


void doRam(const char* param, String value) {
  const char *s = param;
  if (*s == '?')
  {
    if ( value != "0" )
      doRamlist(value.c_str());
    else
      doRamlist ( NULL );
  }
  else if ( *s == '-' ) 
  {
    ramdelkey ( value.c_str() );
  }
  else if ( *s == '.' )
  {
    if ( strlen ( s + 1 ) )
      ramsetstr(s + 1, value);
  }
}

//**************************************************************************************************
//                                      D O N V S L I S T                                          *
//**************************************************************************************************
// - command "nvslist": list the full NVS content to Serial (even if DEBUG = 0)                    *
// - if parameter p != NULL, only show entries which have *p as substring in keyname               *
//**************************************************************************************************
void doNvslist(const char* p) {
  fillkeylist();
  int i = 0;
  int idx = 0;
  doprint("NVS content");
  while (nvskeys[i][0] != '\0') {
    bool match = (p == NULL);
    if (!match)
      match =  strstr(nvskeys[i], p) != NULL;
    if (match)
      doprint(" %3d: '%s' = '%s'", ++idx, nvskeys[i], nvsgetstr(nvskeys[i]).c_str());
    i++;
  }
}

void doNvs(const char* param, String value) {
  const char *s = param;
  if (*s == '?')
  {
    if ( value != "0" )
      doNvslist(value.c_str());
    else
      doNvslist ( NULL );
  }
  else if ( *s == '-' ) 
  {
    nvsdelkey ( value.c_str() );
  }
  else if ( *s == '&' )
  {
    if ( strlen ( s + 1 ) )
      nvssetstr(s + 1, value);
  }
}


/*
//**************************************************************************************************
//                                      D O N V S L I S T                                          *
//**************************************************************************************************
// - command "nvslist": list the full NVS content to Serial (even if DEBUG = 0)                    *
// - if parameter p != NULL, only show entries which have *p as substring in keyname               *
//**************************************************************************************************
void doNvslist(const char* p) {
  fillkeylist();
  int i = 0;
  int idx = 0;
  doprint("NVS content");
  while (nvskeys[i][0] != '\0') {
    bool match = (p == NULL);
    if (!match)
      match =  strstr(nvskeys[i], p) != NULL;
    if (match)
      doprint(" %3d: '%s' = '%s'", ++idx, nvskeys[i], nvsgetstr(nvskeys[i]).c_str());
    i++;
  }
}
*/


void doChannels(String value) {
  channelList.clear();
  currentChannel = 0;
  value.trim();
  if (value.length() > 0)
    readDataList(channelList, value);
  numChannels = channelList.size();
  dbgprint("CHANNELS (total %d): %s",  numChannels, value.c_str());
}

void doChannel(String value, int ivalue) {
  if (channelList.size() > 0) {
    int16_t channel = currentChannel;
    //    Serial.printf("CurrentChannel = %d, value=%s, ivalue=%d\r\n", channel, value.c_str(), ivalue);
    if (channel)
      if (channelList[channel - 1] != ini_block.newpreset) {
        //        Serial.println("Channel set to ZERO: does not match current preset");
        channel = currentChannel = 0;
      }
    if (ivalue) {
      if (ivalue <= channelList.size()) {
        channel = ivalue;
      }
    } else {
      if (!channel) {
        currentChannel = channelList.size();
        while ((currentChannel > 0) && (ini_block.newpreset != channelList[currentChannel - 1]))
          currentChannel--;
        channel = currentChannel;
      }
      if (value == "up") {
        if (channel) {
          if (channel < channelList.size())
            channel++;
        } else
          channel = 1;
      } else if (value == "down") {
        if (channel) {
          if (channel > 1)
            channel--;
        } else
          channel = channelList.size();
      } else if (value = "any") {
        if (0 == channel)
          channel = 1 + random(channelList.size());
        else if (channelList.size() > 1)
          while (channel == currentChannel)
            channel = 1 + random(channelList.size());
      }
    }
    if (channel != currentChannel) {
      //      Serial.printf("New Channel found: %d (was %d) with preset=%d\r\n", channel, currentChannel, channelList[channel-1]);
      char s[20];
      sprintf(s, "%d", channelList[channel - 1]);
      analyzeCmd("preset", s);
      currentChannel = channel;
    }
  }
}

void doInlist(const char *p) {
  RetroRadioInput::showAll(p);
}


void retroRadioInit() {
  bool initDone = false;
  if (!initDone) {
    initDone = true;
  }
}


char *retroRadioLoops[12];
int numLoops = 0;

void readprogbuttonsRetroRadio()
{
  char        mykey[20] ;                                   // For numerated key
  int8_t      pinnr ;                                       // GPIO pinnumber to fill
  int         i ;                                           // Loop control
  String      val ;                                         // Contents of preference entry
  for ( i = 0 ; ( pinnr = progpin[i].gpio ) >= 0 ; i++ )    // Scan for all programmable pins
  {
    sprintf ( mykey, "gpio_%02d", pinnr ) ;                 // Form key in preferences
    if ( nvssearch ( mykey ) )
    {
      val = nvsgetstr ( mykey ) ;                           // Get the contents
      if ( val.length() )                                   // Does it exists?
      {
        if ( !progpin[i].reserved )                         // Do not use reserved pins
        {
          char s[200];
          sprintf(s, "in.gpio_%02d=src=d%d,mode=2,on0=gpio_%02d,start", pinnr, pinnr, pinnr);
          analyzeCmd(s);
          progpin[i].reserved = true;                       // reserve it so original readprogbuttons
          // does not handle it
          dbgprint ( "gpio_%02d will execute %s",           // Show result
                     pinnr, val.c_str() ) ;
        }
      }
    }
  }
  // Now for the touch pins 0..9, identified by their GPIO pin number
  for ( i = 0 ; ( pinnr = touchpin[i].gpio ) >= 0 ; i++ )   // Scan for all programmable pins
  {
    sprintf ( mykey, "touch_%02d", i ) ;                    // Form key in preferences
    if ( nvssearch ( mykey ) )
    {
      val = nvsgetstr ( mykey ) ;                           // Get the contents
      if ( val.length() )                                   // Does it exists?
      {
        if ( !touchpin[i].reserved )                        // Do not use reserved pins
        {
          char s[200];
          sprintf(s, "in._touch_%02d_=src=t%d,mode=1,on0=touch_%02d,start", i, i, i);
          analyzeCmd(s);
          dbgprint ( "touch_%02d will execute %s",          // Show result
                     i, val.c_str() ) ;
          touchpin[i].reserved = true;                      // reserve it so original readprogbuttons
          // does not handle it
        }
        else
        {
          dbgprint ( "touch_%02d pin (GPIO%02d) is reserved for I/O!",
                     i, pinnr ) ;
        }
      }
    }
  }
}

#if (ETHERNET==1)
static bool EthernetFound = false;

//**************************************************************************************************
//                                       C O N N E C T E T H                                       *
//**************************************************************************************************
// Connecting to ETHERNET.                                                                         *
// This function waits for a defined period (should be at least 5 seconds in my experience) for    *
// ethernet ip                                                                                     *
// Returns true if we got IP from ethernet within time.                                            *
// Problem with ethernet-stack: once started, it can not be stopped again (i. e. if not plugged in)*
// As a result of running still in background, WiFi will become to slow. Therefore (if not connect *
// possible within set timeout), a flag is set in NVS and a reset is forced. This flag will be     *
// evaluated and if set no connect attempt is made but should be left to WiFi.                     *
//**************************************************************************************************
bool connecteth() {
  uint32_t myStartTime = millis();
  uint32_t ethTimeout = ETHERNET_CONNECT_TIMEOUT * 1000;
  if (nvssearch("eth_timeout"))                                   // Ethernet timeout in preferences?
  {
    String val = String(nvsgetstr ("eth_timeout"));               // De-reference to another NVS-Entry?
#if defined(RETRORADIO)
    chomp_nvs(val);                                               // remove anything unnecessary
#else
    chomp(val);
#endif
    if (val == "0")                                               // Explicitely set to Zero?
    {
      dbgprint ( "Ethernet disabled in preferences "              // Tell it
                 "(""eth_timeout = 0"" found)" );
      return false;                                               // And do not not attempt
    }
    ethTimeout = val.toInt();                                     // convert to integer
    if (ethTimeout < ETHERNET_CONNECT_TIMEOUT)
      ethTimeout = ETHERNET_CONNECT_TIMEOUT;
    else if (ethTimeout > 2 * ETHERNET_CONNECT_TIMEOUT)
    {
      dbgprint ("eth_timeout set to %d (seconds) in preferences. This will cause a long delay if ethernet fails/is not connected!", ethTimeout);
    }
    ethTimeout *= 1000;
  }
  WiFi.disconnect(true);                                          // Make sure old connections are stopped
  myStartTime = millis();
  WiFi.onEvent(ethevent);                                         // Event handler to catch connect status
  if (ETH.begin(ini_block.eth_phy_addr, ini_block.eth_phy_power, ini_block.eth_phy_mdc,
                ini_block.eth_phy_mdio,
                (eth_phy_type_t)ini_block.eth_phy_type,
                (eth_clock_mode_t)ini_block.eth_clk_mode))
    while (!EthernetFound && (millis() - myStartTime < ethTimeout))
      delay(5);                                                     // wait for ethernet to succeed (or timeout)
  WiFi.removeEvent(ethevent);                                     // event handler not needed anymore
  if (!EthernetFound) {                                           // connection failed?
    dbgprint("Ethernet did not work! Will try WiFi next!");
    esp_eth_deinit();                                           // leaving Ethernet stack on would lead to
    // distortions on WiFi
  }
  return EthernetFound;
}


//**************************************************************************************************
//                                         E T H E V E N T                                         *
//**************************************************************************************************
// Callback for Ethernet related system events. Main purpose is to set EthernetFound and           *
// NetworkFound flags.                                                                             *
//**************************************************************************************************

void ethevent(WiFiEvent_t event)
{
  char *pfs;
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      dbgprint("ETH Started");
      //set eth hostname here
      ETH.setHostname(NAME);
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      dbgprint("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      ipaddress = ETH.localIP().toString() ;             // Form IP address
      pfs = dbgprint ( "Connected to Ethernet");
      tftlog ( pfs ) ;
      pfs = dbgprint ( "IP = %s", ipaddress.c_str() ) ;   // String to dispay on TFT
      tftlog ( pfs ) ;
      EthernetFound = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      pfs = dbgprint("ETH Disconnected");
      tftlog ( pfs );
      if (EthernetFound) {
        EthernetFound = false;
        NetworkFound = false;
      }
      break;
    case SYSTEM_EVENT_ETH_STOP:
      pfs = dbgprint("ETH Stopped");
      tftlog ( pfs );
      if (EthernetFound) {
        EthernetFound = false;
        NetworkFound = false;
      }
      break;
    default:
      break;
  }
}

#endif


volatile IR_data  ir = { 0, 0, 0, 0, IR_NONE } ;         // IR data received
sv int8_t ir_pin = -1;
//sv bool IRseen = false;

//**************************************************************************************************
//                             D E C O D E R _I R R C 5                                            *
//**************************************************************************************************
// Routine for decoding Philips IR Protocol RC5, called from ISR.                                  *
// Parameter intval holds the length of the last pulse (in microseconds), t0 the current time (in  *
// microseconds and pin_state the current state of the IR input pin.                               *
// Algorithm adopted from https://github.com/pinkeen/avr-rc5/blob/master/rc5.c                     *
// Sets the IR-data for later processing in scanIR()                                               *
// (Toggle bit in the protocol frame is cleared before reporting result to application             *
//**************************************************************************************************
void IRAM_ATTR decoder_RR_IRRC5(uint32_t intval, uint32_t t0, int pin_state)
{
  //#define RC5SHORT_MIN 444   /* 444 microseconds */
  //#define RC5SHORT_MAX 1333  /* 1333 microseconds */
  //#define RC5LONG_MIN 1334   /* 1334 microseconds */
  //#define RC5LONG_MAX 2222   /* 2222 microseconds */
  //    IRseen = true;
  const uint8_t trans[4] = {0x01, 0x91, 0x9b, 0xfb} ; // state transition table
  sv uint8_t rc5_counter = 14 ;                       // Bit counter (counting downwards)
  sv uint16_t rc5_command = 0 ;                       // The command received
  sv RC5State rc5_state = RC5STATE_BEGIN ;            // State of the RC5 decoder statemachine
  sv uint32_t rc5_lasttime = 0 ;                      // last time a valid command has been received
  /* VS1838B pulls the data line up, giving active low,
     so the output is inverted. If data pin is high then the edge
     was falling and vice versa.

      Event numbers:
      0 - short space
      2 - short pulse
      4 - long space
      6 - long pulse
  */

  uint8_t event = pin_state ? 2 : 0 ;
  if (intval > 1333 && intval <= 2222) // "Long" event detected?
  {
    event += 4 ;
  }
  else if (intval < 444 || intval > 2222)
  {
    /* If delay wasn't long and isn't short then
       it is erroneous so we need to reset but
       we don't return from interrupt so we don't
       loose the edge currently detected. */
    rc5_counter = 14 ;                   // Bit counter (counting downwards)
    rc5_command = 0 ;                    // The command received
    rc5_state = RC5STATE_BEGIN ;         // State of the RC5 decoder statemachine
  }
  if (rc5_state == RC5STATE_BEGIN)        // First bit?
  {
    rc5_counter-- ;
    rc5_command = 1 ;                   // Is always 1
    rc5_state = RC5STATE_MID1 ;         // State: Middle of HIGH bit detected
  }
  else
  {
    RC5State rc5_newstate = (RC5State)((trans[rc5_state] >> event) & 0x03);

    if (rc5_newstate == rc5_state || rc5_state > RC5STATE_START0)
    {
      /* No state change or wrong state means
         error so reset. */
      rc5_counter = 14 ;                   // Bit counter (counting downwards)
      rc5_command = 0 ;                    // The command received
      rc5_state = RC5STATE_BEGIN ;         // State of the RC5 decoder statemachine
    }
    else
    {
      rc5_state = rc5_newstate ;

      /* Emit 0 or 1 to rc5_command */
      if ( (rc5_state == RC5STATE_MID0) || (rc5_state == RC5STATE_MID1))
      {
        rc5_counter-- ;
        rc5_command = rc5_command << 1 ;
        if (rc5_state == RC5STATE_MID1)
          rc5_command++ ;
      }
      /* The only valid end states are MID0 and START1.
        Mid0 is ok, but if we finish in MID1 we need to wait
        for START1 so the last edge is consumed. */
      if (rc5_counter == 0 && (rc5_state == RC5STATE_START1 || rc5_state == RC5STATE_MID0))
      {
        rc5_state = RC5STATE_END ;
        rc5_command = (rc5_command & 0x37ff) ; // + ((rc5_command & 0x1000) >> 1)+ 0xf000;
        if (ir.protocol == IR_NONE)
        {
          if ((rc5_command == ir.command) && (t0 - rc5_lasttime < 220000))
            ir.repeat_counter++ ;
          else
          {
            ir.exitcommand = ir.command ;
            ir.command = rc5_command ;
            ir.repeat_counter = 0 ;
          }
          ir.protocol = IR_RC5 ;
        }
        rc5_lasttime = t0;
      }
    }
  }
}


//**************************************************************************************************
//                             D E C O D E R _I R N E C                                            *
//**************************************************************************************************
// Routine for decoding NEC IR Protocol, called from ISR.                                          *
// Parameter intval holds the length of the last pulse (in microseconds), t0 the current time (in  *
// microseconds).                                                                                  *
// Intervals are 640 or 1640 microseconds for data.  syncpulses are 3400 micros or longer.         *
// Input is complete after 65 level changes.                                                       *
// Only the last 32 level changes are significant and will be handed over to common data.          *
//**************************************************************************************************
void IRAM_ATTR decoder_RR_IRNEC ( uint32_t intval, uint32_t t0 )
{
  //  IRseen = true;
  sv uint32_t      ir_locvalue = 0 ;                 // IR code
  sv int           ir_loccount = 0 ;                 // Length of code
  sv uint32_t      ir_lasttime = 0;                  // Last time of valid code (either first shot or repeat)
  sv uint8_t       repeat_step = 0;                  // Current Status of repeat code received
  uint32_t         mask_in = 2 ;                     // Mask input for conversion
  uint16_t         mask_out = 1 ;                    // Mask output for conversion
  if ( ( intval > 300 ) && ( intval < 800 ) )        // Short pulse?
  {
    if ( repeat_step == 2 )                          // Full preamble for repeat seen?
    {
      if (t0 - ir_lasttime < 120000)                 // Repeat shot is expected 108 ms either after last
      { // initial key or previous repeat
        if (ir.protocol == IR_NONE)                  // Last IR value has been consumed? repeatcode is valid?
        {
          ir.repeat_counter++ ;                      // Increment repeat counter (ir.command should still be valid)
          ir.protocol = IR_NEC;                      // Set protocol info to indicate valid data has been received
        }                                            // (repeat counter is in upper word of ir_value)
        ir_lasttime = t0 ;                           // Reset timer for next repeat shot.
      }
      repeat_step = 0 ;                              // Reset repeat search state
    }
    else
    {
      ir_locvalue = ir_locvalue << 1 ;               // Shift in a "zero" bit
      ir_loccount++ ;                                // Count number of received bits
      ir_0 = ( ir_0 * 3 + intval ) / 4 ;             // Compute average durartion of a short pulse
    }
  }
  else if ( ( intval > 1400 ) && ( intval < 1900 ) ) // Long pulse?
  {
    ir_locvalue = ( ir_locvalue << 1 ) + 1 ;         // Shift in a "one" bit
    ir_loccount++ ;                                  // Count number of received bits
    ir_1 = ( ir_1 * 3 + intval ) / 4 ;               // Compute average durartion of a short pulse
  }
  else if ( ir_loccount == 65 )                      // Value is correct after 65 level changes
  {
    ir.exitcommand = ir.command ;
    ir.command = 0 ;
    while ( mask_in )                                // Convert 32 bits to 16 bits
    {
      if ( ir_locvalue & mask_in )                   // Bit set in pattern?
      {
        ir.command |= mask_out ;                     // Set set bit in result
      }
      mask_in <<= 2 ;                                // Shift input mask 2 positions
      mask_out <<= 1 ;                               // Shift output mask 1 position
    }
    ir_loccount = 0 ;                                // Ready for next input
    ir.repeat_counter = 0 ;                          // Report as first press
    ir.protocol = IR_NEC;                            // Set protocol info to indicate valid data has been received
    ir_lasttime = t0 ;                               // Start timer for detecting valid repeat shots
  }
  else
  {
    if ((intval > 8500) && (intval < 9500) && (repeat_step == 0))         // Check for repeat: a repeat shot starts
      repeat_step++;                                                      // with a 9ms pulse
    else if ((intval > 2000) && (intval < 2500) && (repeat_step == 1))    // followed by a 2.25ms pulse
      repeat_step++;
    else
      repeat_step = 0;                                                    // Reset repeat shot decoding
    ir_locvalue = 0 ;                                // Reset decoding
    ir_loccount = 0 ;
  }

}

//**************************************************************************************************
//                                          I S R _ I R                                            *
//**************************************************************************************************
// Interrupts received from VS1838B on every change of the signal.                                 *
// This routine will call both decoders for NEC and RC5 protocols.                                 *
//**************************************************************************************************
void IRAM_ATTR isr_RR_IR()
{
  sv uint32_t      t0 = 0 ;                          // To get the interval
  uint32_t         t1, intval ;                      // Current time and interval since last change
  int pin_state = digitalRead(ir_pin);     // Get current state of InputPin

  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare

  decoder_RR_IRRC5 ( intval, t0, pin_state ) ;          // Run decoder for Philips RC5 protocol

  decoder_RR_IRNEC ( intval, t0 ) ;                     // Run decoder for NEC protocol
}

//**************************************************************************************************
//                                          I S R _ I R N E C                                      *
//**************************************************************************************************
// Interrupts received from VS1838B on every change of the signal.                                 *
// This routine will call the decoder for NEC protocol only.                                       *
//**************************************************************************************************
void IRAM_ATTR isr_RR_IRNEC()
{
  sv uint32_t      t0 = 0 ;                          // To get the interval
  uint32_t         t1, intval ;                      // Current time and interval since last change
  int pin_state = digitalRead(ir_pin);               // Get current state of InputPin

  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare


  decoder_RR_IRNEC ( intval, t0 ) ;                     // Run decoder for NEC protocol
}


//**************************************************************************************************
//                                          I S R _ I R R C 5                                      *
//**************************************************************************************************
// Interrupts received from VS1838B on every change of the signal.                                 *
// This routine will call the decoder for RC5 protocol only.                                       *
//**************************************************************************************************
void IRAM_ATTR isr_RR_IRRC5()
{
  sv uint32_t      t0 = 0 ;                          // To get the interval
  uint32_t         t1, intval ;                      // Current time and interval since last change
  int pin_state = digitalRead(ir_pin);               // Get current state of InputPin

  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare

  decoder_RR_IRRC5 ( intval, t0, pin_state ) ;          // Run decoder for Philips RC5 protocol

}


void setupIR() {
  String val;
  ir_pin = ini_block.ir_pin;
  if ( ir_pin >= 0 )
  {
    dbgprint ( "Enable pin %d for IR (NEC and RC5)",
               ir_pin ) ;
    pinMode ( ir_pin, INPUT ) ;                // Pin for IR receiver VS1838B
    attachInterrupt ( ir_pin,                  // Interrupts will be handle by isr_IR
                      isr_RR_IR, CHANGE ) ;
  }
  if ( ir_pin < 0)
  {
    val = nvsgetstr("pin_ir_nec");
    if (val.length() > 0)                                 // Parameter in preference?
    {
      ir_pin = val.toInt() ;                            // Convert value to integer pinnumber
    }
    if (ir_pin >= 0)
    {
      dbgprint ( "Enable pin %d for IR (NEC)", ir_pin);
      pinMode ( ir_pin, INPUT ) ;                        // Pin for IR receiver VS1838B
      attachInterrupt ( ir_pin,                  // Interrupts will be handle by isr_IR
                        isr_RR_IRNEC, CHANGE ) ;

    }
  }
  if ( ir_pin < 0)
  {
    val = nvsgetstr("pin_ir_rc5");
    if (val.length() > 0)                                 // Parameter in preference?
    {
      ir_pin = val.toInt() ;                            // Convert value to integer pinnumber
    }
    if (ir_pin >= 0)
    {
      dbgprint ( "Enable pin %d for IR (RC5)", ir_pin);
      pinMode ( ir_pin, INPUT ) ;                        // Pin for IR receiver VS1838B
      attachInterrupt ( ir_pin,                  // Interrupts will be handle by isr_IR
                        isr_RR_IRRC5, CHANGE ) ;

    }
  }

  ini_block.ir_pin = -1;
  ini_block.ir_pin_nec = -1;
  ini_block.ir_pin_rc5 = -1;
#define DEBUG_BUFFER_SIZE 150
  if (ir_pin >= 0)
    reservepin(ir_pin);
}


//**************************************************************************************************
//                             E X E C U T E _ I R _ C O M M A N D                                 *
//**************************************************************************************************
// Searches NVS-Keys for specific IR command(s) and executes command(s) if found                   *
// The keycode is taken from ir-data struct.                                                       *
//    if type == "", ir_XXXX commands will be searched for (key pressed)                           *
//    if type == "r", in order ir_XXXXRY, ir_XXXXrY and ir_XXXXr commands will be searched for (in *
//    that order) and the commands stored for first found key will be executed (key hold)          *
//    if type == "x", irXXXXx commands will be searched for (key released)                         *
//    for any other type content, type is assumed to already contain the nvs-searchkey             *
//  Returns true if any commands have been found (and executed)                                    *
//  If verbose is set to "true", will also report if no entry has been found in NVS                *
//**************************************************************************************************

bool execute_ir_command ( String type, bool verbose = true )
{
  char *protocol_names[] = {"", " (NEC)", " (RC5)"} ;           // Known IR protocol names
  bool ret = false;
  char mykey[20];
  bool done = false;                                            // If more than one possible key results, set to
  // true if one of those keys already "fired"
  strcpy(mykey, type.c_str());                                  // Default: type contains the key already
  if ( type == "" )                                             // If not
    sprintf ( mykey, "ir_%04X", ir.command );                   // Generate key for "key pressed"
  else if ( type == "x" )                                       // Or
    sprintf ( mykey, "ir_%04Xx", ir.exitcommand );              // Generate key for "key released"
  else if ( type == "r" ) {                                     // Or
    done = true;                                                // Generate and already execute sequence of "repeat" events
    sprintf ( mykey, "ir_%04XR%d",                              // First check, if there is a cyclic command sequence defined
              ir.command, ir.repeat_counter );
    ret = execute_ir_command ( String(mykey) );                 // did we have a cyclic sequence?
    if (ret)                                                    // if so
    {
      ir.repeat_counter = 0;                                    // Reset repeat counter
    }
    else                                                        // no cyclic sequence found
    {
      sprintf ( mykey, "ir_%04Xr%d",                            // Do we have a "longpress" with precise current repeat counter?
                ir.command, ir.repeat_counter );
      ret = execute_ir_command ( String(mykey), false );        // try and execute
      if (!ret)                                                 // Longpress event key for this precise repeat counter found?
      { // If not
        sprintf ( mykey, "ir_%04Xr", ir.command );              // Search for "unspecific" longpress event ir_XXXXr
        ret = execute_ir_command ( String(mykey), false );
      }
    }
  }
  if (! done )                                                  // Did we already (tried) some key(s) (can be from longpress only)
  { // If not, mykey now contains the NVS key to check
    ret = nvssearch ( mykey ) ;
    if (! ret )                                                 // Key not found?
    {
      if ( verbose )                                            // Show info if key not found?
        dbgprint ( "IR event %s%s received but not found in preferences.",
                   mykey, protocol_names[ir.protocol] ) ;
    }
    else
    {
      String val = nvsgetstr ( mykey ) ;                    // Get the contents
      const char* reply;
      chomp_nvs ( val );
      dbgprint ( "IR code for %s%sreceived. Will execute %s",
                 mykey, protocol_names[ir.protocol], val.c_str() ) ;
      reply = analyzeCmdsRR ( val ) ;                         // Analyze command and handle it
      dbgprint ( reply ) ;                                  // Result for debugging
    }
  }
  return ret;
}



//**************************************************************************************************
//                                     S C A N I R R R                                             *
//**************************************************************************************************
// See if IR input is available.  Execute the programmed command(s).                               *
//**************************************************************************************************
void scanIRRR()
{
  //  char        mykey[20] ;                                   // For numerated key
  //  String      val ;                                         // Contents of preference entry
  //  const char* reply ;                                       // Result of analyzeCmd
  //  uint16_t    code ;                                        // The code of the Remote key
  //  uint16_t    repeat_count ;                                // Will be increased by every repeated
  // call to scanIR() until the key is released.
  // Zero on first detection

  if ( ir_pin < 0)
    return;

  if ( ir.exitcommand != 0 )                                // Key release detected?
  {
    execute_ir_command ( "x" ) ;
    ir.exitcommand = 0;
    ir.exittimeout = 0;
  }
  if ( ir.protocol != IR_NONE )                             // Any input?
  {
    //    char *protocol_names[] = {"", "NEC", "RC5"} ;           // Known IR protocol names
    //    code = ir.command ;                                     // Key code is stored in lower word of ir_value
    //    repeat_count = ir_value >> 16 ;                         // Repeat counter in upper word of if_value
    if ( 0 < ir.repeat_counter )                            // Is it a "longpress"?
    { // In case of a "longpress", we will first search
      // for ir_XXXXrY, where XXXX is the HEX code of the
      // key as usual and Y is the repeat counter in decimal,
      // just as is, no leading '0'. Example: ir_40BFr20 means
      // this is the 20th repeat of key with code $0BF. That
      // translates (roughly) to a press time of (at least) 20 * 100ms
      // If such a preference is not found, we will search for
      // ir_XXXXr, which fires for any repeat count.
      // That means, ir_XXXXrY takes precedence, if defined for a
      // specific repeat count, ir_XXXXr will NOT fire.
      // (BTW: ir_XXXXr0 will never fire. ir_XXXX cannot be masked)
      // If the 'R' is in uppercase, the event will fire and also
      // reset the repeat count. This is all handled in function
      execute_ir_command ( "r" );                           // execute_ir_command ( "r" );
    }
    else                                                    // On first press, do just search for ir_XXXX
    {
      execute_ir_command ( "" ) ;
    }
    ir.protocol = IR_NONE ;                                 // Indicate IR code has been handled to ISR
    ir.exittimeout = millis() ;                             // Set timeout for generating "key released event"
  }
  else                                                      // No new key press/longpress detected
  {
    if (( ir.command != 0 ) && (ir.exittimeout != 0 ) )     // was there a valid keypress before
      if ( millis() - ir.exittimeout > 500 )                // That is too long ago (so no further repeat can be expected)
      {
        ir.exitcommand = ir.command ;                       // Set exitcode
        ir.command = 0 ;                                    // Clear code info
        ir.repeat_counter = 0 ;                             // Clear repeat counter (just to be sure)
        ir.exittimeout = 0 ;                                // Clear exitcode timeout (jsut to be sure)
        execute_ir_command ( "x" );                         // Search NVS for "key release" ir_XXXXx
        ir.exitcommand = 0 ;                                // Clear exitcode after it has been used.
      }
  }
}



void setupRR(uint8_t setupLevel) {
  if (setupLevel == SETUP_START)
  {
    touch_pad_init();
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    touch_pad_filter_start(16);
    adc_power_on();
    readprogbuttonsRetroRadio();
    setupIR();
  }
  else if (setupLevel == SETUP_NET)
  {
#if (ETHERNET==1)
    adc_power_on();                                        // Workaround to allow GPIO36/39 as IR-Input
    NetworkFound = connecteth();                           // Try ethernet
    if (NetworkFound)                                     // Ethernet success??
      WiFi.mode(WIFI_OFF);
#endif
  }
  else if (setupLevel == SETUP_DONE)
  {
    char s[20] = "::setup";
    int l = strlen(s);
    doprint("Running commands from NVS '%s'", s);
    analyzeCmdsRR ( nvsgetstr(s) );
    s[l + 1] = 0;
    for (int i = 0; i < 10; i++)
    {
      s[l] = '0' + i;
      dbgprint("Running commands from NVS '%s'", s);
      analyzeCmdsRR ( nvsgetstr(s) );
    }
    strcpy(s, "::loop");
    l = strlen(s);
    String loopcmd = nvsgetstr(s);
    numLoops = 0;
    int i = 0;
    s[l + 1] = 0;
    do
    {
      loopcmd.trim();
      if (loopcmd.length() > 0)
      {
        retroRadioLoops[numLoops] = strdup(loopcmd.c_str());
        dbgprint("Found loop command: %s", retroRadioLoops[numLoops]);
        numLoops++;
      }
      if (i < 10)
      {
        s[l] = '0' + i;
        loopcmd = nvsgetstr(s);
      }
      i++;
    } while (i < 11);
    retroRadioLoops[numLoops] = NULL;
  }
}



void loopRR() {
  static bool first = false;

  if (first) {
    first = false;
    retroRadioInit();

    return;
  }

  RetroRadioInput::checkAll();
  if (numLoops) {
    int deb = DEBUG;
    for (int i = 0; i < numLoops; i++) 
    {
      DEBUG = 0;
      analyzeCmdsRR(String(retroRadioLoops[i]));
    }
    DEBUG = deb;
  }
  scanIRRR();
  return;
  /*
    static bool statusWasNeverPublished = true;
    static int ledToShow = 0;
      if (first)
        setupMedionExtension();
      if (first || (1 == playingstat) || (localfile))
        {
    //        toggleMedionLED(LED_MODE_LOW);

          first = false;
        }
    //   volumeRead();
    //   if ((1 == playingstat) && (0 != lastVolume))
    //   readChannel();
    #ifdef OLD
     toneRead();
     switchKnobRead();
     volumeKnobRead();
     tuneKnobRead();
     if (numLeds) {
      showLed(ledToShow, 0);
      if (++ledToShow >= numLeds)
        ledToShow = 0;
     }
    #endif
     //dummy->check();
     //dummyVol->check();
     //dummySwitch->check();
     static uint32_t last39;
     if (millis() - last39 > 1000)
     {
    //      Serial.printf("GPIO39: %d\r\n", analogRead(39));
        //Serial.printf("GPIO36: %d\r\n", analogRead(36));
        //Serial.printf("GPIO35: %d\r\n", analogRead(35));
        //Serial.printf("GPIO34: %d\r\n", analogRead(34));
        last39 = millis();
     }
    RetroRadioInput::checkAll();
  */
}

bool analyzeCmdRR(char* reply, String& param, String& value) {
  const char *s = param.c_str();
  char firstChar = *s;
  bool ret = false;
  chomp_nvs(value);
  if (!isalpha(firstChar))
  {
    ret = true;
    if ( firstChar == '.' )
    {
      if (strchr ("?-", s[1])) 
        doRam ( s + 1, value );
      else
        doRam ( s , value );
    }
    else if ( firstChar == '&' )
    {
      if (strchr ("?-", s[1])) 
        doNvs ( s + 1, value );
      else
        doNvs ( s , value );
    }
  }
  else if ( ret = ( param == "execute" ) ) 
  {
    analyzeCmdsRR ( ramsearch(value.c_str())?ramgetstr(value.c_str()):nvsgetstr(value.c_str()) );  
  }
  else if ( ret = (param.startsWith("ram" ))) 
  {
    //Serial.println("RAM FROM NEW SOURCE!!!");
    doRam ( param.c_str() + 3, value ) ;
  }
  else if ( ret = (param.startsWith("nvs" ))) 
  {
    //Serial.println("RAM FROM NEW SOURCE!!!");
    doNvs ( param.c_str() + 3, value ) ;
  }
  else if ( ret = ( param == "channels" ) ) 
  {
    doChannels (value);
  } 
  else if ( ret = ( param == "channel" ) )
  {
    doChannel (value, atoi ( value.c_str() ) );
  }
  if ( ret )
    if ( strlen ( reply ) == 0 ) 
      strcpy ( reply, "OK from RetroRadio" ) ;
  return ret;
}

void analyzeCmdDoneRR (char* reply, String& argument, String& value ) {

}


//**************************************************************************************************
//                                   A N A L Y Z E C M D S                                         *
//**************************************************************************************************
// Handling of the various commands from remote webclient, Serial or MQTT.                         *
// Here a sequence of commands is allowed, commands are expected to be seperated by ';'            *
//**************************************************************************************************
const char* analyzeCmdsRR ( String commands )
{
  static char reply[512] ;                       // Reply to client, will be returned
  uint16_t commands_executed = 0 ;               // How many commands have been executed

  char * cmds = strdup ( commands.c_str() ) ;
  if ( cmds )
  {
    char *s = cmds ;
    while ( s )
    {
      char *p = s;
      char * next = NULL;
      int nesting = 0;
      while (*p && !next) {
        if ((*p == '(') || (*p == '{'))
          nesting++;
        else if (nesting == 0) {
          if (*p == ';' )
            next = p;
        } else if ((*p == ')') || (*p == '}'))
          nesting--;
        p++;
      }
      if ( next )
      {
        *next = 0 ;
        next++ ;
      }
      analyzeCmd ( s ) ;
      commands_executed++ ;
      s = next ;
    }
    snprintf ( reply, sizeof ( reply ), "Executed %d command(s) from sequence %s", commands_executed, cmds ) ;
    free ( cmds ) ;
  }
  return reply ;
}




#endif // Retroradio
