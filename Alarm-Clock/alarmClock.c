#include "ses_scheduler.h"
#include "ses_fsm.h"
#include "ses_button.h"
#include "ses_lcd.h"
#include "ses_led.h"
#include "ses_common.h"
#include "ses_timer.h"

/** VARIABLES **/
static Fsm alarmClock;			//name this FSM is alarmClock
static struct time_t clock_t;	//contain the time in human-friendly structure
bool modifyingAlarm = 0;		//0: the Alarm is not being modified.

taskDescriptor displayClock; //task to display Clock on LCD
taskDescriptor matchedAlarm; //task to watch Alarm on LCD
taskDescriptor flashLed;	 //task to flashing LED
taskDescriptor flashOff;	 //task to turn off LED
taskDescriptor joystick_t;	 //task to force joystick callback function executed within task_context
taskDescriptor rotary_t;	 //task to force rotarr callback function executed within task_context

/** DEFINES **/
//state is the function to be executed
#define TRANSITION(newState) (fsm->state = newState, RET_TRANSITION)

/** FUNCTION DECLARATION **/

/* these states are mentioned in the UML files */
fsmReturnStatus initialClock (Fsm *fsm, const Event *event);
fsmReturnStatus setHourClock(Fsm *fsm, const Event *event);
fsmReturnStatus setMinuteClock(Fsm *fsm, const Event *event);
fsmReturnStatus normalClock	(Fsm *fsm, const Event *event);
fsmReturnStatus setHourAlarm(Fsm *fsm, const Event *event);
fsmReturnStatus setMinuteAlarm(Fsm *fsm, const Event *event);
fsmReturnStatus alarmMatched(Fsm *fsm, const Event *event);

inline static void fsm_init(Fsm* fsm, State init);
inline static void fsm_dispatch(Fsm* fsm, const Event* event);
void joystick_task(void);
void rotary_task(void);
void joystick(void* para);
void rotary(void* para);
void displayClk(void* para);
void button_debouncing(void *para);
void alarmMatch(void* para);
void ledToggle(void* para);
void ledOff(void* para);

/** MAIN FUNCTION **/
int main(void)
{
	led_greenInit();
	//led_greenOff();
	led_redInit();
	led_redOff();
	led_yellowInit();
	led_yellowOff();
	lcd_init();
	lcd_clear();
	scheduler_init();
	button_init(1);

	/* button_debouncing task every 5ms */
	taskDescriptor buttonDebounce;
	buttonDebounce.task    = button_debouncing;
	buttonDebounce.expire  = 0;
	buttonDebounce.period  = 5;
	buttonDebounce.execute = 0;
	scheduler_add(&buttonDebounce);

	/* displaying clock after every 1 seconds */
	displayClock.task	 = displayClk;
	displayClock.expire  = 0;
	displayClock.period  = 1000;
	displayClock.execute = 0;

	/* button debouncing task every 5ms */
	matchedAlarm.task  = alarmMatch;
	matchedAlarm.period 	= 0;
	matchedAlarm.execute	= 0;

	/* toggle LED with frequency of 4Hz */
	flashLed.task 	 = ledToggle;
	flashLed.execute = 0;
	flashLed.expire  = 0;
	flashLed.period  = 250;

	/* turn off LED after 5 seconds since it has turned on */
	flashOff.task 	= ledOff;
	flashOff.execute= 0;
	flashOff.expire	= 5000;
	flashOff.period = 0;

	/*let the FSM run in Task_context
	 * however this option cause undesired behaviour while running code,
	 * this is still not successfully fixed within the given time,
	 */

//	joystick_t.execute 	= 1; //execute immediately
//	joystick_t.expire 	= 0;
//	joystick_t.period	= 0;
//	joystick_t.task		= joystick;
//
//	rotary_t.execute = 1;	//execute immediately
//	rotary_t.expire  = 0;
//	rotary_t.period  = 0;
//	rotary_t.task	 = rotary;
//
//	button_setJoystickButtonCallback(joystick_task);
//	button_setRotaryButtonCallback(rotary_task);

	button_setJoystickButtonCallback(joystick);
	button_setRotaryButtonCallback(rotary);

	fsm_init((Fsm*) &alarmClock, &initialClock);
	sei();

	for(;;){
		scheduler_run();
	}
}

/** FUNCTION DEFINITION **/
/**
 * function to be passed through scheduler to debounce button
 */
void button_debouncing(void *para)
{
	button_checkState();
}

/**
 * function to be passed through scheduler to force execution fo joystick callback function within task_context
 */
void joystick_task(void)
{
	scheduler_add(&joystick_t);
}

/**
 * function to be passed through scheduler to force execution fo rotary callback function within task_context
 */
void rotary_task(void)
{
	scheduler_add(&rotary_t);
}

/**
 * callback function assigned to joystick button.
 * to dispatch the "state" and "event".
 */
void joystick(void* para)
{
	Event e = {.signal = JOYSTICK_PRESSED};
	fsm_dispatch(&alarmClock, &e);
}

/**
 * callback function assigned to rotary button.
 * to dispatch the "state" and "event".
 */
void rotary(void* para)
{
	Event e = {.signal = ROTARY_PRESSED};
	fsm_dispatch(&alarmClock, &e);
}

/* dispatches events to state machine, called in application*/
inline static void fsm_dispatch(Fsm* fsm, const Event* event) {
	/*static Event entryEvent;
	entryEvent.signal = ENTRY;*/
	static Event entryEvent = {.signal = ENTRY};
	static Event exitEvent  = {.signal = EXIT};

	State s = fsm->state;
	fsmReturnStatus r = fsm->state(fsm, event);
	if (r==RET_TRANSITION) {
		s(fsm, &exitEvent); 			// call exit action of last state
		fsm->state(fsm, &entryEvent); 	// call entry action of new state
	}
}

/* sets and calls initial state of state machine */
inline static void fsm_init(Fsm* fsm, State init) {
//initialization
	Event entryEvent = {.signal = ENTRY};
	fsm->state = init;
	fsm->state(fsm, &entryEvent);
}

/**
 * initial state of alarmClock
 * showing screen prompt user for setting time.
 */
fsmReturnStatus initialClock(Fsm *fsm, const Event *event)
{
	switch (event->signal){
		case ENTRY:{
			lcd_setPixel(28,1,1);
			lcd_setPixel(28,2,1);
			lcd_setPixel(28,3,1);
			lcd_setPixel(28,4,1);

			fsm->timeSet.hour 	= 0;
			fsm->timeSet.minute = 0;
			fsm->timeSet.second = 0;
			fsm->isAlarmEnabled = 0; //off at the begining

			lcd_setCursor(0,0);
			fprintf(lcdout,"HH:MM");

			lcd_setCursor(0,1);
			fprintf(lcdout,"Press Any Button to\nstart Adjust Hour");

			return RET_HANDLED;
		}

		case ROTARY_PRESSED:{
			return TRANSITION(setHourClock);
		}

		case JOYSTICK_PRESSED:{
			return TRANSITION(setHourClock);
		}

		case EXIT:{
			return RET_HANDLED;
		}

		default:
			return RET_IGNORED;
	}
}

/**
 * state to set Hour clock
 */
fsmReturnStatus setHourClock(Fsm *fsm, const Event *event)
{
	switch(event->signal){
		case ENTRY:{
			lcd_clear();
			lcd_setPixel(28,6,1);
			lcd_setPixel(28,7,1);
			lcd_setPixel(28,8,1);
			lcd_setPixel(28,9,1);

			lcd_setCursor(0,0);
			fprintf(lcdout,"00:MM");

			lcd_setCursor(0,1);
			fprintf(lcdout,"Press Rotary Button\nTo Adjust Hour");

			return RET_HANDLED;
		}

		case ROTARY_PRESSED:{
			fsm->timeSet.hour = (fsm->timeSet.hour+1)%24;
			lcd_setCursor(0,0);
			fprintf(lcdout,"%02d:MM",fsm->timeSet.hour);

			return RET_HANDLED;
		}

		case JOYSTICK_PRESSED:{
			return TRANSITION(setMinuteClock);
		}

		case EXIT:{
			return RET_HANDLED;
		}

		default:
			return RET_IGNORED;
	}
}

/**
 * state to set Minute clock
 */
fsmReturnStatus setMinuteClock(Fsm *fsm, const Event *event)
{
	switch(event->signal){
		case ENTRY:{
			lcd_clear();

			lcd_setPixel(28,11,1);
			lcd_setPixel(28,12,1);
			lcd_setPixel(28,13,1);
			lcd_setPixel(28,14,1);

			lcd_setCursor(0,0);
			fprintf(lcdout,"%02d:00",fsm->timeSet.hour);

			lcd_setCursor(0,1);
			fprintf(lcdout,"Press Rotary Button\nTo Adjust Minute");

			return RET_HANDLED;
		}

		case ROTARY_PRESSED:{
			fsm->timeSet.minute = (fsm->timeSet.minute+1)%60;;
			lcd_setCursor(0,0);
			fprintf(lcdout,"%02d:%02d",fsm->timeSet.hour,fsm->timeSet.minute);


			return RET_HANDLED;
		}

		case JOYSTICK_PRESSED:{
			return TRANSITION(normalClock);
		}

		case EXIT:{
			systemTime_t temp = 0;
			temp = (systemTime_t) fsm->timeSet.minute*60*1000 + (systemTime_t)fsm->timeSet.hour*60*60*1000; //in millisecond
			scheduler_setTime(temp);
			scheduler_add(&displayClock);

			return RET_HANDLED;
		}

		default:
			return RET_IGNORED;
	}
}

/**
 * clock is function normally after setting.
 */
fsmReturnStatus normalClock(Fsm *fsm, const Event *event)
{
	switch(event->signal){
		case ENTRY:{
			lcd_setPixel(28,16,1);
			lcd_setPixel(28,17,1);
			lcd_setPixel(28,18,1);
			lcd_setPixel(28,19,1);
			/* Sychronizing time set with system time */

			return RET_HANDLED;
		}

		case ROTARY_PRESSED:{
			led_yellowToggle();
			fsm->isAlarmEnabled ^=1;	//toggle the alarm

			return RET_HANDLED;
		}

		case JOYSTICK_PRESSED:{
			return TRANSITION(setHourAlarm);
		}

		case EXIT:{
			lcd_setPixel(28,16,0);
			lcd_setPixel(28,17,0);
			lcd_setPixel(28,18,0);
			lcd_setPixel(28,19,0);

			return RET_HANDLED;
		}

		default:
			return RET_IGNORED;
	}
}

/**
 * state to set Alarm Hour
 */
fsmReturnStatus setHourAlarm(Fsm *fsm, const Event *event)
{
	switch(event->signal){
		case ENTRY:{
			scheduler_remove(&displayClock);
			scheduler_remove(&matchedAlarm);

			/*reset*/
			fsm->timeSet.hour 	= 0;
			fsm->timeSet.minute = 0;
			fsm->timeSet.second = 0;

			modifyingAlarm = 1;
			lcd_clear();
			lcd_setPixel(28,21,1);
			lcd_setPixel(28,22,1);
			lcd_setPixel(28,23,1);
			lcd_setPixel(28,24,1);

			lcd_setCursor(0,0);
			fprintf(lcdout,"HH:MM");

			lcd_setCursor(0,1);
			fprintf(lcdout,"Press Rotary Button\nto Adjust Hour");

			return RET_HANDLED;
		}

		case ROTARY_PRESSED:{
			fsm->timeSet.hour = (fsm->timeSet.hour+1)%24;
			lcd_setCursor(0,0);
			fprintf(lcdout,"%02d:MM",fsm->timeSet.hour);

			return RET_HANDLED;
		}

		case JOYSTICK_PRESSED:{
			return TRANSITION(setMinuteAlarm);
		}

		case EXIT:{
			lcd_setPixel(28,21,0);
			lcd_setPixel(28,22,0);
			lcd_setPixel(28,23,0);
			lcd_setPixel(28,24,0);

			return RET_HANDLED;
		}

		default:
			return RET_IGNORED;
	}
}

/**
 * state to set Alarm Minute
 */
fsmReturnStatus setMinuteAlarm(Fsm *fsm, const Event *event)
{
	switch(event->signal){
		case ENTRY:{
			lcd_clear();

			lcd_setPixel(28,26,1);
			lcd_setPixel(28,27,1);
			lcd_setPixel(28,28,1);
			lcd_setPixel(28,29,1);

			lcd_setCursor(0,0);
			fprintf(lcdout,"%02d:00",fsm->timeSet.hour);

			lcd_setCursor(0,1);
			fprintf(lcdout,"Press Rotary Button\nTo Adjust Minute");

			return RET_HANDLED;
		}

		case ROTARY_PRESSED:{
			fsm->timeSet.minute = (fsm->timeSet.minute+1)%60;;
			lcd_setCursor(0,0);
			fprintf(lcdout,"%02d:%02d",fsm->timeSet.hour,fsm->timeSet.minute);

			return RET_HANDLED;
		}

		case JOYSTICK_PRESSED:{
			return TRANSITION(normalClock);
		}

		case EXIT:{

			systemTime_t alarmTime = (systemTime_t) fsm->timeSet.minute*60*1000 + (systemTime_t)fsm->timeSet.hour*60*60*1000;
			systemTime_t temp = scheduler_getTime();
			systemTime_t expire = (systemTime_t)alarmTime - (systemTime_t)temp + (systemTime_t)((temp/1000)%60);

			matchedAlarm.expire		= (uint16_t) expire;

			scheduler_add(&matchedAlarm);
			//flashLed.expire = expire;
			//scheduler_add(&flashLed);

			modifyingAlarm = 0;
			scheduler_add(&displayClock);

			lcd_setPixel(28,26,0);
			lcd_setPixel(28,27,0);
			lcd_setPixel(28,28,0);
			lcd_setPixel(28,29,0);

			return RET_HANDLED;
		}

		default:
			return RET_IGNORED;
	}
}

/**
 * when the alarm matched to current time, the whole system is in this state
 */
fsmReturnStatus alarmMatched(Fsm *fsm, const Event *event)
{
	switch(event->signal){
		case ENTRY:{
			/* the scheduler_add(&flashOff) should  remove the flashLed task after 5 seconds and turn of led_red. However
			 * for some unknow reasons, the task flashLed cannot be removed causing undesired effect.
			 * even not using the flashOff task to automatically turn the red_led after 5 seconds, I still got following problem:
			 *     while the red_led is flashing (when alarm matched), pressing anybutton will turn it off, however, when pressing the
			 *     rotary button again, the red is flashing again, and is stopped after pressing any button. Meaning, it stuck in the alarmMatched state.
			 *     The only way to stops it is: when the led_Red is toggled, pressed any button to turn if off, then press joystick button to start adjust
			 *     alarm time again, then every thing is reset. I still dont know what happening here.
			 * */
			scheduler_add(&flashLed);
			//scheduler_add(&flashOff);

			return RET_HANDLED;
		}

		case ROTARY_PRESSED:{
			return TRANSITION(normalClock);
		}

		case JOYSTICK_PRESSED:{
			return TRANSITION(normalClock);
		}

		case EXIT:{
			scheduler_remove(&flashLed);
			//scheduler_remove(&flashOff);

			led_redOff();
			led_yellowOff();
			alarmClock.isAlarmEnabled = 0;

			return RET_HANDLED;
			break;
		}

		default:
			return RET_IGNORED;
	}
}

void displayClk(void* para)
{
	led_greenToggle();
	systemTime_t temp = scheduler_getTime();
	systemTime_t local = 0;
	/*converting system-time to human-time*/
	local = (systemTime_t)temp;
	clock_t.milli =  temp%1000;

	local = (systemTime_t)local/1000;
	clock_t.second = local%60;

	local = (systemTime_t)local/60;
	clock_t.minute = local%60;

	local = (systemTime_t)local/60;
	clock_t.hour = 	 local%24;

	lcd_clear();
	lcd_setCursor(0,0);
	fprintf(lcdout,"%02d:%02d:%02d",clock_t.hour, clock_t.minute, clock_t.second);

	lcd_setCursor(0,1);
	fprintf(lcdout,"System Updated\nClock Starts");

	lcd_setPixel(28,16,1);
	lcd_setPixel(28,17,1);
	lcd_setPixel(28,18,1);
	lcd_setPixel(28,19,1);
}

void alarmMatch(void* para)
{
	lcd_setPixel(28,55,1);
	lcd_setPixel(28,56,1);
	lcd_setPixel(28,57,1);
	lcd_setPixel(28,58,1);

	if(modifyingAlarm == 1||alarmClock.isAlarmEnabled == 0)
	{
		return;
	}
	else
		{
			//scheduler_add(&flashLed);
			//scheduler_add(&flashOff);
			scheduler_remove(&matchedAlarm);

			//change to alarmMatched state with event Entry
			alarmClock.state = alarmMatched;
			Event e = {.signal = ENTRY};
			fsm_dispatch(&alarmClock, &e);
		}
}

void ledToggle(void* para)
{
	lcd_setPixel(28,60,1);
	lcd_setPixel(28,61,1);
	lcd_setPixel(28,62,1);
	lcd_setPixel(28,63,1);

	led_redToggle();
}

void ledOff(void* para)
{
	//scheduler_remove(&flashLed);
	led_redOff();
	led_yellowOff();
	alarmClock.isAlarmEnabled = 0;

	//change to normalClock state with event Entry
	alarmClock.state = normalClock;
	Event e = {.signal = ENTRY};
	fsm_dispatch(&alarmClock, &e);
}
