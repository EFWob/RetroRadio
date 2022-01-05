#include <stdint.h>
#include <Arduino.h>
#include "BasicStatemachine.h"



BasicStatemachine::BasicStatemachine() {
    resetStateTime();
    onEnter(STATE_INIT_NONE, STATE_INIT_NONE);  
}

bool BasicStatemachine :: setState(int16_t newStateNb, int32_t timeout) {
bool ret = false;  
  if (!_isHalted)
	if ((ret = transitionAllowed(_currentStateNumber, newStateNb)))    {
//      if (newStateNb != _currentStateNumber) {
        int16_t oldStateNb = _currentStateNumber;
//        onLeave(_currentStateNumber, newStateNb);
        _currentStateNumber = newStateNb;
		_timeout = timeout;
        onEnter(_currentStateNumber, oldStateNb);
        resetStateTime();
	} 
//	  else
//		resetStateTime();	
   return ret;
}

void BasicStatemachine::setTimeout(int32_t timeout, void *timeoutdata) {
	_timeout = timeout;
	_timeoutData = timeoutdata;
	resetStateTime();
}

void BasicStatemachine::setTimeoutMin(int32_t timeout) {
int32_t remain;	
	if (0 > _timeout)
		setTimeout(timeout, _timeoutData);
	else
	{
		remain = getStateTime();
		if (remain >= _timeout)
			setTimeout(timeout, _timeoutData);
		else
		{
			remain = _timeout - remain;
			if (remain < timeout)
				setTimeout(timeout, _timeoutData);
		}
	}
}


void BasicStatemachine::run() {
	if (!_isHalted)
		{ 
		runState(_currentStateNumber);
		if (_timeout >= 0)
		{
			static uint32_t lastReport = 0;
			if (millis() - lastReport > 500)
			{
				Serial.printf("TimeoutSet to: %ld, elapsed: %ld\r\n", _timeout, getStateTime());
				lastReport = millis();
			}
			if (getStateTime() > (uint32_t)_timeout)
			{
				_timeout = -1;
				Serial.println("TIMEOUT TRIGGER!!");
				//onTimeout(_currentStateNumber);
				onEvent(0, _timeoutData);
			}
		}
		}

}

StatemachineLooperClass::StatemachineLooperClass() :
	BasicStatemachine()
	{
	}

void StatemachineLooperClass::runState(int16_t stateNb)	{
static int looperSize = -1;if (looperSize != (int)_machines.size()) {Serial.printf("running looper with machines=%d\r\n", looperSize = _machines.size());};
	for(unsigned int i = 0;i < _machines.size();i++) {
		_machines[i]->run();		
		yield();
	}
}


bool StatemachineLooperClass::add(BasicStatemachine * newMachineToLoop) {
bool ret;	
	if ((ret = (NULL != newMachineToLoop))) 
		_machines.push_back(newMachineToLoop);
	return ret;
}

bool StatemachineLooperClass::remove(BasicStatemachine * machineToRemove) {
bool ret = false;
unsigned int idx = 0;
	while (!ret && (idx < _machines.size())) 
		if (!(ret = (_machines[idx] == machineToRemove)))
			idx++;
	if (ret)
		_machines.erase(_machines.begin() + idx);
	return ret;
}



bool StatemachineLooperClass::add(BASIC_LOOP_SIGNATURE) {
	if (loopcallback)
		return add(new BasicStatemachineLooper(loopcallback));
	return false;
}


void StatemachineLooperClass::shutdown() {
	for(unsigned int i = 0;i < _machines.size();i++)
		_machines[i]->shutdown();

}

void StatemachineLooperClass::restart() {
	this->shutdown();
#if defined(ESP32) || defined(ESP8266)
	ESP.restart();
#endif
	while (1) ;
}	

void StatemachineLooperClass::onEvent(int16_t event, void* eventData ) {
	for(unsigned int i = 0;i < _machines.size();i++)
		_machines[i]->onEvent(event, eventData);
}


BasicStatemachineLooper::BasicStatemachineLooper(BASIC_LOOP_SIGNATURE) {
	this->loopcallback = loopcallback;
}

void BasicStatemachineLooper::runState(int16_t stateNb) {
	if (loopcallback)
		loopcallback();
}


StatemachineLooperClass StatemachineLooper;