/*
 This is the code for the DIY intervalometer "Intervallino" based on the ATmega32 Arduino Chip.
 
 Please refer to the following blog entry for the full story:
 https://rastroludico.wordpress.com/intervallino
 
 L.
 */
 
#include <LiquidCrystal.h>

// some useful defines

//#define DEBUG				// define it to debug through the arduino console

#define RESISTOR_PAD 9810.0		// balance/pad resistor value, set this to
					// the measured resistance of your pad resistor
#define CHECKYES	5
#define CHECKNO		6
#define BUTTON_WAIT_OPT		200	//time in ms to wait for button in option mode
#define BUTTON_WAIT_SUMUP	700	//time in ms to wait for button in sumup mode (also regulates the time
					//before the next information is displayed)
#define	DELTA_LIGHT	5		//in case shot on flash, sets the delta value to trigger 
#define	DELTA_SOUND	10		//in case shot on clap, sets the delta value to trigger 

#define	FOCUS_TIME	1000		//in ms
#define SHOOT_TIME	1000		//in ms

#define	KEYPAD_NONE	0
#define	KEYPAD_UP	1
#define	KEYPAD_DOWN	2
#define	KEYPAD_LEFT	4
#define	KEYPAD_RIGHT	8
#define	KEYPAD_ALL	16

#define PIN_IN_BUTTON_UP_LEFT		A3
#define PIN_IN_BUTTON_DOWN_RIGHT	A2
#define PIN_OUT_SPEAKER		3
#define PIN_OUT_SHOOT		9
#define PIN_OUT_FOCUS		10
#define PIN_LCD_D7		5
#define PIN_LCD_D6		6
#define PIN_LCD_D5		7	
#define PIN_LCD_D4		8	
#define PIN_LCD_E		11	
#define PIN_LCD_RS		12	
#define PIN_LCD_VDD		13
#define PIN_IN_PHOTORESISTOR	A5	
#define PIN_IN_THERMISTOR	A4	


// Set up lcd global variable
LiquidCrystal lcd(PIN_LCD_RS,PIN_LCD_E,PIN_LCD_D4,PIN_LCD_D5,PIN_LCD_D6,PIN_LCD_D7);

// Set up global variables
int	g_run_mode=0;		// 0:choose options, 1:sumup-confirmation, 2:running, 3: ended
//options
int 	G_menu_id;		// current menu
int	g_tmp;			// temporary menu
int 	g_opt_mode;		// 0:intervalometer,1:shot on flash, 2: shot on clap
int 	g_opt_totpic;		// stop after this number of pictures have been taken
unsigned long g_opt_time;	// time interval between pics (in ms)
byte 	g_opt_time_mask[6];	// NB: must correspond to the value of g_opt_time in hh,mm,ss format!!!
int 	g_opt_startmode;	// 1 start after timer elapsed, 2 start after a condition on the light is met
unsigned long g_opt_starttime;	// time before starting (in ms)
byte 	g_opt_starttime_mask[6];	// NB: must correspond to the value of g_opt_starttime in hh,mm,ss format!!!
int 	g_opt_startlight;	// if startmode=2, sets the threshold value of the light to start
byte	g_opt_startlight_above;	// if startmode=2, sets start if lights falls above(1) or below(0) the threshold
int 	g_opt_focus;		// pre-focus: 0 no, 1 yes
int	g_opt_offlight;		// light: 0 always on, 1 always off, 2 off after 5s, 3 off after 30s
int	g_opt_offlight_deltat;	// delta time before switch off
unsigned long	g_opt_offlight_t0;	// time elapsed from last key input
int 	g_opt_beeps;    	// beep before shot: 0 none, 1 1sec before, 2 do an accelerating sequence
int 	g_opt_menubeeps;    	// beep on keypress

unsigned long g_time0;		// start time
unsigned long g_time_pic;	// time last pics has been taken
bool	g_okfoto=0;		// the conditions are met to take picture:
				// 1 is either if timer/light is not reached or the number of pic taken is already reached

int	g_tot_pic=0;		// tot taken pictures
int	g_last_light=99;	// last value of light
int	g_last_sound=99;	// last value of sound
int	g_lcd=0;		// status of display
int	g_button_wait=BUTTON_WAIT_OPT;	// wait time in ms for button press
float	g_temp;			// temperature

// menu conversion table
byte G_menu_id2txtup[]=  {
	1,3,4,5,6,7,8,0,0,0,9,10,0,0,0,0,0,0,0,0,
	11,12,13,0,0,0,0,0,0,0,14,15,16,0,0,0,0,0,0,0,
	17,18,19,0,0,0,0,0,0,0,20,21,22,0,0,0,0,0,0,0,
	23,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	24,0,0,0,0,0,0,0,0,0,25,0,0,0,0,0,0,0,0,0,
	26,27,28,29,0,0,0,0,0,0,
	30,31,32,0,0,0,0,0,0,0,
	33,0,0,0,0,0,0,0,0,0,
	34,0,0,0,0,0,0,0,0,0,
	35,0,0,0,0,0,0,0,0,0,
	36,0,0,0,0,0,0,0,0,0,
	37,0,0,0,0,0,0,0,0,0
};

// menu strings
char* G_menu_txt[]={
	"                ",
	"----------------",
	"XXXXXXXXXXXXXXXX",
	"quick mode    >",
	"set mode      >",
	"options       >",
	"start         >",
	"goodies       >",
	"about         >",
	"shot every 5s  ",
	"shot every 30s ",
	"intervalom.   >",
	"shot on flash  ",
	"shot on clap   ",
	"pre-focus      ",
	"backlight     >",
	"shot beeps    >",
	"now!",
	"timer         >",
	"light thresh  >",
	"thermometer   >",
	"light measure >",
	"battery stat  >",
	"Intervallino1.0",
	"set tot pics   ",
	"set time interv",
	"always on      ",
	"off after 5s   ",		
	"off after 30s  ",		
	"always off     ",
	"beeps off      ",
	"beep on shot   ",
	"bip-bip        ",
	"set time 2 start",
	"set light value",
	"temperature:",
	"light value:",
	"battery status:"
};

// custom characters (arrows up/down...) for the LCD
byte c_up[8] = {
	B00100,
	B01110,
	B10101,
	B00100,
	B00100,
	B00100,
	B00100,
	B00100,
};

byte c_down[8] = {
	B00100,
	B00100,
	B00100,
	B00100,
	B00100,
	B10101,
	B01110,
	B00100,
};

byte c_dot[8] = {
	B00000,
	B01110,
	B11111,
	B11111,
	B11111,
	B11111,
	B01110,
	B00000,
};

byte c_right[8] = {
	B00000,
	B10000,
	B11000,
	B11100,
	B11110,
	B11100,
	B11000,
	B10000,
};

byte c_closebox[8] = {
	B00000,
	B11111,
	B11111,
	B11111,
	B11111,
	B11111,
	B11111,
	B00000,
};

byte c_openbox[8] = {
	B00000,
	B11111,
	B10001,
	B10001,
	B10001,
	B10001,
	B11111,
	B00000,
};
byte c_ok[8] = {
	B00000,
	B00000,
	B00100,
	B00010,
	B11111,
	B00010,
	B00100,
	B00000,
};

// entry function, executed each time arduino powers up
void setup()
{
	//setup of the I/O pins
	pinMode(PIN_OUT_SPEAKER, OUTPUT);     
	pinMode(PIN_OUT_SHOOT, OUTPUT);     
	pinMode(PIN_OUT_FOCUS, OUTPUT);     
	pinMode(PIN_LCD_VDD, OUTPUT);     

	pinMode(PIN_IN_BUTTON_UP_LEFT, INPUT);     
	pinMode(PIN_IN_BUTTON_DOWN_RIGHT, INPUT);     
	pinMode(PIN_IN_PHOTORESISTOR, INPUT);     
	pinMode(PIN_IN_THERMISTOR, INPUT);     

	#ifdef DEBUG
	Serial.begin(9600);
	#endif

	reset();
}

void reset(void)
{

	SwitchBacklight(0);
	delay(300);
	beep(1);
	SwitchBacklight(1);

	lcd.print("Welcome ;-)");

	// set default values
	g_run_mode=0;
	G_menu_id=1;
	g_tmp=0;
	g_opt_mode=0;
	g_opt_totpic=1000;
	g_opt_time=15000;
	g_opt_time_mask[0]=0;
	g_opt_time_mask[1]=0;
	g_opt_time_mask[2]=0;
	g_opt_time_mask[3]=0;
	g_opt_time_mask[4]=1;
	g_opt_time_mask[5]=5;
	g_opt_startmode=1;
	g_opt_starttime=60000;
	g_opt_starttime_mask[0]=0;
	g_opt_starttime_mask[1]=0;
	g_opt_starttime_mask[2]=0;
	g_opt_starttime_mask[3]=1;
	g_opt_starttime_mask[4]=0;
	g_opt_starttime_mask[5]=0;
	g_opt_startlight=50;
	g_opt_startlight_above=1;
	g_opt_focus=0;
	g_opt_offlight=0;
	g_opt_offlight_deltat=0;
	g_opt_offlight_t0=0;
	g_opt_beeps=0;
	g_opt_menubeeps=1;
	g_okfoto=0;
	g_tot_pic=0;
	g_last_light=99;
	g_last_sound=99;

	delay(1000);
	show_opt_display();
}

// main arduino loop
void loop()
{
	#ifdef DEBUG
	delay(150);
	Serial.println(G_menu_id);
	#endif

	check_input();
	check_action();
}

void check_input()
{
	int button_pressed;

	button_pressed=WaitButton(g_button_wait);
	if (button_pressed!=KEYPAD_NONE) {
		//a button has been pressed, dispatch to the correct handler depending on the run mode

		if (button_pressed==KEYPAD_ALL) {
			reset();
			return;
		}
		if (g_opt_menubeeps==1) beep(1);

		g_opt_offlight_t0=millis();	//reset timer from last keypad input
		if (g_opt_offlight!=3 && g_lcd==0) {
			SwitchBacklight(1);	//switch on LCD if it was off 
			return;
		}

		switch(g_run_mode) {
			case 0:
				check_opt_input(button_pressed);
				break;
			case 1:
				check_sumup_input(button_pressed);
				break;
			case 2:
				check_run_input(button_pressed);
				break;
			default:
				break;
		}
	}
}

void check_action()
{

	switch(g_run_mode){
		case 0:
			//if it shows one of the Temperature/Light menu, then must refresh the values..
			if (G_menu_id==160 || G_menu_id==170 || G_menu_id==180|| G_menu_id==60) {
				show_opt_display();
			}
			break;
		case 1:
			update_sumup_display();
			break;
		case 2:
			if (!g_okfoto) check_startnow();
			else check_foto();

			if (g_opt_offlight>0 && g_lcd==1) {
				//switch off light if (is on and) time elapsed is enough
				if ((millis()-g_opt_offlight_t0)>g_opt_offlight_deltat) SwitchBacklight(0);
			}

			if (g_lcd==1) update_run_display();

			break;
		case 3:
			if (g_lcd==1) {
				//switch off light if (is on and) 2mn are passed after the end
				if ((millis()-g_opt_offlight_t0)>120000) SwitchBacklight(0);
			}
			if (g_lcd==1) update_ended_display();
			break;
	}
	return;

}

void show_opt_display()
{
	int n,tmp;
	float tmpf;
	byte mid=G_menu_id2txtup[G_menu_id];
	byte i;

	lcd.clear();
	//Serial.println((int)G_menu_id);
	//Serial.println((int)g_tmp);

	//put the '>' symbol in front of the first line
	//except for some special cases
	if (G_menu_id!=60 && G_menu_id!=110 && G_menu_id!=120  && G_menu_id<160) {
		lcd.write(0);
		lcd.setCursor(1,0);
	}
	lcd.print(G_menu_txt[mid]);
	lcd.setCursor(1,1);

	//for the second line there are some sepcial cases..
	switch(G_menu_id) {
		//case of the last selection in first row
		case 6:
		case 11:
		case 32:
		case 42:
		case 52:
			lcd.print(G_menu_txt[0]);
			break;
		case 20:
			//shot on flash/clap check box
			lcd.print(G_menu_txt[mid+1]);
			lcd.setCursor(15,1);
			if (g_opt_mode==1) lcd.write(CHECKYES);
			else lcd.write(CHECKNO);
			break;
		case 21:
			//shot on flash/clap check box
			lcd.print(G_menu_txt[mid+1]);
			lcd.setCursor(15,0);
			if (g_opt_mode==1) lcd.write(CHECKYES);
			else lcd.write(CHECKNO);
			lcd.setCursor(15,1);
			if (g_opt_mode==2) lcd.write(CHECKYES);
			else lcd.write(CHECKNO);
			break;
		case 22:
			//shot on flash/clap check box
			lcd.setCursor(15,0);
			if (g_opt_mode==2) lcd.write(CHECKYES);
			else lcd.write(CHECKNO);
			lcd.setCursor(0,1);
			lcd.print(G_menu_txt[0]);
			break;

		case 30:
			//prefocus check box
			lcd.print(G_menu_txt[mid+1]);
			lcd.setCursor(15,0);
			if (g_opt_focus==1) lcd.write(CHECKYES);
			else lcd.write(CHECKNO);
			if (g_tmp==1) {
				lcd.setCursor(15,0);
				lcd.blink();
			} else lcd.noBlink();
			break;

		case 60:
			//measure temperature
			g_temp=MeasureTemp();
			lcd.print(g_temp,2);
			lcd.print("C ");
			if (g_temp<10) lcd.print("brr..!");
			else if (g_temp<20) lcd.print("cool!");
			else if (g_temp<25) lcd.print("nice :)");
			else if (g_temp<31) lcd.print("hot!");
			else lcd.print("BURNING!");
			lcd.noBlink();
			break;

		case 110:
			//set tot pics
			lcd.setCursor(0,1);
			n=int(g_opt_totpic/1000);
			lcd.print(n);
			tmp=g_opt_totpic-n*1000;
			n=int(tmp/100);
			lcd.print(n);
			tmp-=n*100;
			n=int(tmp/10);
			lcd.print(n);
			n=tmp-n*10;
			lcd.print(n);
			lcd.setCursor(15,1);
			lcd.write(4);
			if (g_tmp<4) {
				lcd.setCursor(g_tmp,1);
			} else lcd.setCursor(15,1);
			lcd.blink();
			break;
		case 120:
			//set time interval
			lcd.print((int)g_opt_time_mask[0]);
			lcd.print((int)g_opt_time_mask[1]);
			lcd.print("h ");
			lcd.print((int)g_opt_time_mask[2]);
			lcd.print((int)g_opt_time_mask[3]);
			lcd.print("m ");
			lcd.print((int)g_opt_time_mask[4]);
			lcd.print((int)g_opt_time_mask[5]);
			lcd.print("s ");
			lcd.setCursor(15,1);
			lcd.write(4);
			if (g_tmp<6) lcd.setCursor(1+2*int(g_tmp/2)+g_tmp,1);
			else lcd.setCursor(15,1);
			lcd.blink();
			break;
		case 130:
		case 131:
		case 132:
			//backlight check box
			lcd.print(G_menu_txt[mid+1]);
			lcd.setCursor(15,0);
			if (g_opt_offlight==G_menu_id-130) lcd.write(CHECKYES);
			else lcd.write(CHECKNO);
			lcd.setCursor(15,1);
			if (g_opt_offlight==G_menu_id-129) lcd.write(CHECKYES);
			else lcd.write(CHECKNO);
			break;
		case 133:
			//backlight check box
			lcd.setCursor(15,0);
			if (g_opt_offlight==G_menu_id-130) lcd.write(CHECKYES);
			else lcd.write(CHECKNO);
			lcd.setCursor(0,1);
			lcd.print(G_menu_txt[0]);
			break;

		case 140:
		case 141:
			//beeps check box
			lcd.print(G_menu_txt[mid+1]);
			lcd.setCursor(15,0);
			if (g_opt_beeps==G_menu_id-140) lcd.write(CHECKYES);
			else lcd.write(CHECKNO);
			lcd.setCursor(15,1);
			if (g_opt_beeps==G_menu_id-139) lcd.write(CHECKYES);
			else lcd.write(CHECKNO);
			break;
		case 142:
			//beeps check box
			lcd.setCursor(15,0);
			if (g_opt_beeps==2) lcd.write(CHECKYES);
			else lcd.write(CHECKNO);
			lcd.setCursor(0,1);
			lcd.print(G_menu_txt[0]);
			break;

		case 150:
			//set time to start
			lcd.print((int)g_opt_starttime_mask[0]);
			lcd.print((int)g_opt_starttime_mask[1]);
			lcd.print("h ");
			lcd.print((int)g_opt_starttime_mask[2]);
			lcd.print((int)g_opt_starttime_mask[3]);
			lcd.print("m ");
			lcd.print((int)g_opt_starttime_mask[4]);
			lcd.print((int)g_opt_starttime_mask[5]);
			lcd.print("s ");
			lcd.setCursor(15,1);
			lcd.write(4);
			if (g_tmp<6) lcd.setCursor(1+2*int(g_tmp/2)+g_tmp,1);
			else lcd.setCursor(15,1);
			lcd.blink();
			break;

		case 160:
			//set light to start
			lcd.setCursor(15,1);
			lcd.write(4);
			n=MeasureLight();
			lcd.setCursor(0,1);
			lcd.print("l=");
			lcd.print(n);
			lcd.setCursor(5,1);
			lcd.print("shot if");
			if (g_opt_startlight_above) lcd.print(">");
			else lcd.print("<");
			lcd.print(g_opt_startlight);
			lcd.setCursor(12+g_tmp,1);
			lcd.blink();
			break;

		case 170:
			//measure temperature
			g_temp=MeasureTemp();
			lcd.print(g_temp,2);
			lcd.print("C  ");
			tmpf=(g_temp * 9.0)/ 5.0 + 32.0;
			lcd.print(tmpf,1);
			lcd.print("F");
			lcd.noBlink();
			break;

		case 180:
			//measure light
			n=MeasureLight();
			lcd.print(n);
			lcd.noBlink();
			break;

		case 190:
			//measure battery
			n=MeasureBattery();
			lcd.print(n);
			lcd.noBlink();
			break;

		default:
			lcd.print(G_menu_txt[mid+1]);
			break;
	}
}

void update_sumup_display()
{
	lcd.clear();

	switch(g_opt_mode){
		case 0:
			switch(g_tmp){
				case 0:
					lcd.print("intervalometer");
					break;
				case 1:
					lcd.print("tot pics:");
					lcd.print(g_opt_totpic);
					lcd.print("      ");
					break;
				case 2:
					lcd.print("pic every:");
					lcd.print(g_opt_time/1000);
					lcd.print("s");
					break;
				default:
					print_common_sumup(g_tmp-3);
					break;
			}
			g_tmp++;
			if(g_tmp>5) g_tmp=0;
			break;

		case 1:
			switch(g_tmp){
				case 0:
					lcd.print("pic on flash");
					break;
				default:
					print_common_sumup(g_tmp-1);
					break;
			}
			g_tmp++;
			if(g_tmp>3) g_tmp=0;
			break;

		case 2:
			switch(g_tmp){
				case 0:
					lcd.print("pic on sound");
					break;
				default:
					print_common_sumup(g_tmp-1);
					break;
			}
			g_tmp++;
			if(g_tmp>3) g_tmp=0;
			break;
	}

	lcd.setCursor(0,1);
	lcd.print("confirm ?");
	lcd.setCursor(15,1);
	lcd.write(4);
	lcd.cursor();
	lcd.blink();
}

void print_common_sumup(byte ix)
{

	switch(ix){
		case 0:
			if (g_opt_focus) lcd.print("focus on shot");
			else lcd.print("no focus on shot");
			break;
		case 1:
			if (g_opt_beeps==0) lcd.print("no beeps");
			else lcd.print("beep on shot"); 
			break;
		case 2:
			lcd.print("start ");
			if (g_opt_startmode==1){
				if(g_opt_starttime==0) lcd.print("now!");
				else {
					lcd.print("in ");
					lcd.print(g_opt_starttime/1000);
					lcd.print("s");
				}
			} else {
				lcd.print(":light ");
				if (g_opt_startlight_above) lcd.print(">");
				else lcd.print("<");
				lcd.print(g_opt_startlight);
			}
			break;
	}
}

void update_run_display()
{
	unsigned long dt;
	int n,hh,mm,ss;
	lcd.clear();
	lcd.noCursor();
	lcd.noBlink();

	if (g_okfoto) {

		lcd.print(g_tot_pic);
		lcd.print("/");
		lcd.print(g_opt_totpic);
		lcd.print(" pics");
		lcd.setCursor(0,1);
		lcd.print("next ");
		switch(g_opt_mode) {
			case 0:
				lcd.print(":");
				dt=(g_opt_time-(millis()-g_time_pic))/1000;
				hh=int(dt/3600);
				mm=int(dt/60)%60;
				ss=dt%60;
				if (hh>0) {
					lcd.print(hh);
					lcd.print("h ");
				} 
				if ((hh>0) || (mm>0)) {
					lcd.print(mm);
					lcd.print("m ");
				}
				lcd.print(ss);
				lcd.print("s");
				break;
			case 1:
				lcd.print("on flash");
				break;
			case 2:
				lcd.print("on clap");
				break;
		}
	} else {
		//waiting for conditions to be met to start
		if(g_opt_startmode==1) {
			lcd.print("starting in:");
			lcd.setCursor(0,1);
			dt=(g_opt_starttime-(millis()-g_time0))/1000;
			hh=int(dt/3600);
			mm=int(dt/60)%60;
			ss=dt%60;
			if (hh>0) {
				lcd.print(hh);
				lcd.print("h ");
			} 
			if ((hh>0) || (mm>0)) {
				lcd.print(mm);
				lcd.print("m ");
			}
			lcd.print(ss);
			lcd.print("s");
		} else {
			lcd.print("start when:");
			n=MeasureLight();
			lcd.print(n);
			if (g_opt_startlight_above) lcd.print(">");
			else lcd.print("<");
			lcd.print(g_opt_startlight);
		}
	}
}

void update_ended_display()
{
	//Finish!
	lcd.print(g_tot_pic);
	lcd.print("/");
	lcd.print(g_opt_totpic);
	lcd.print(" pics");
	lcd.setCursor(0,1);
	lcd.print("finished!");
	delay(750);
	lcd.clear();
	delay(500);

	return;
}

void check_opt_input(int button_pressed)
{
	int n1,n2,n3,n4,n,tmp;

	switch(button_pressed) {

		case KEYPAD_UP:
			switch(G_menu_id) {
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:

				case 11:

				case 21:
				case 22:

				case 31:
				case 32:

				case 41:
				case 42:

				case 51:
				case 52:

				case 131:
				case 132:
				case 133:

				case 141:
				case 142:
					G_menu_id--;
					break;

				case 60:
					G_menu_id=6;

					break;
				case 30:
					if(g_tmp==0) G_menu_id++;
					else g_opt_focus=1-g_opt_focus;
					break;
				case 110:
					//increase the correct number of total pictures
					n1=int(g_opt_totpic/1000);
					n2=int((g_opt_totpic-n1*1000)/100);
					n3=int((g_opt_totpic-n1*1000-n2*100)/10);
					n4=g_opt_totpic-n1*1000-n2*100-n3*10;
					switch(g_tmp){
						case 0:
							if (n1<9) g_opt_totpic+=1000;
							else g_opt_totpic-=9000;
							break;
						case 1:
							if (n2<9) g_opt_totpic+=100;
							else g_opt_totpic-=900;
							break;
						case 2:
							if (n3<9) g_opt_totpic+=10;
							else g_opt_totpic-=90;
							break;
						case 3:
							if (n4<9) g_opt_totpic++;
							else g_opt_totpic-=9;
							break;
						default:
							break;
					}
					break;

				case 120:
					switch(g_tmp) {
						case 0:
						case 1:
						case 3:
						case 5:
							if (g_opt_time_mask[g_tmp]<9) g_opt_time_mask[g_tmp]++;
							else g_opt_time_mask[g_tmp]=0;
							break;
						case 2:
						case 4:
							if (g_opt_time_mask[g_tmp]<5) g_opt_time_mask[g_tmp]++;
							else g_opt_time_mask[g_tmp]=0;
							break;
					}
					g_opt_time =36000000*g_opt_time_mask[0]+3600000*g_opt_time_mask[1]+600000*g_opt_time_mask[2]+60000*g_opt_time_mask[3]+10000*g_opt_time_mask[4]+1000*g_opt_time_mask[5];
					break;

				case 150:
					switch(g_tmp) {
						case 0:
						case 1:
						case 3:
						case 5:
							if (g_opt_starttime_mask[g_tmp]<9) g_opt_starttime_mask[g_tmp]++;
							else g_opt_starttime_mask[g_tmp]=0;
							break;
						case 2:
						case 4:
							if (g_opt_starttime_mask[g_tmp]<5) g_opt_starttime_mask[g_tmp]++;
							else g_opt_starttime_mask[g_tmp]=0;
							break;
					}
					g_opt_starttime =36000000*g_opt_starttime_mask[0]+3600000*g_opt_starttime_mask[1]+600000*g_opt_starttime_mask[2]+60000*g_opt_starttime_mask[3]+10000*g_opt_starttime_mask[4]+1000*g_opt_starttime_mask[5];
					break;

				case 160:
					switch(g_tmp) {
						case 0:
							g_opt_startlight_above=1-g_opt_startlight_above;
							break;
						case 2:
							if(g_opt_startlight<99) g_opt_startlight++;
							else  g_opt_startlight=0;
							break;
					}
					break;

				default:
					// blink (and beeps) for wrong press
					WrongPress();
					break;
			}

			break;


		case KEYPAD_DOWN:
			switch(G_menu_id) {
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:

				case 10:

				case 20:
				case 21:

				case 31:

				case 40:
				case 41:
				case 50:
				case 51:

				case 130:
				case 131:
				case 132:

				case 140:
				case 141:
					G_menu_id++;
					break;
				case 60:
					G_menu_id=6;
					break;
				case 30:
					if(g_tmp==0) G_menu_id++;
					else g_opt_focus=1-g_opt_focus;
					break;

				case 110:
					//decrease the correct number of total pictures
					n1=int(g_opt_totpic/1000);
					n2=int((g_opt_totpic-n1*1000)/100);
					n3=int((g_opt_totpic-n1*1000-n2*100)/10);
					n4=g_opt_totpic-n1*1000-n2*100-n3*10;
					switch(g_tmp) {
						case 0:
							if (n1>0) g_opt_totpic-=1000;
							else g_opt_totpic+=9000;
							break;
						case 1:
							if (n2>0) g_opt_totpic-=100;
							else g_opt_totpic+=900;
							break;
						case 2:
							if (n3>0) g_opt_totpic-=10;
							else g_opt_totpic+=90;
							break;
						case 3:
							if (n4>0) g_opt_totpic--;
							else g_opt_totpic+=9;
							break;
						default:
							break;
					}
					break;

				case 120:
					switch(g_tmp) {
						case 0:
						case 1:
						case 3:
						case 5:
							if (g_opt_time_mask[g_tmp]>0) g_opt_time_mask[g_tmp]--;
							else g_opt_time_mask[g_tmp]=9;
							break;
						case 2:
						case 4:
							if (g_opt_time_mask[g_tmp]>0) g_opt_time_mask[g_tmp]--;
							else g_opt_time_mask[g_tmp]=5;
							break;
					}
					g_opt_time =36000000*g_opt_time_mask[0]+3600000*g_opt_time_mask[1]+600000*g_opt_time_mask[2]+60000*g_opt_time_mask[3]+10000*g_opt_time_mask[4]+1000*g_opt_time_mask[5];
					break;

				case 150:
					switch(g_tmp) {
						case 0:
						case 1:
						case 3:
						case 5:
							if (g_opt_starttime_mask[g_tmp]>0) g_opt_starttime_mask[g_tmp]--;
							else g_opt_starttime_mask[g_tmp]=9;
							break;
						case 2:
						case 4:
							if (g_opt_starttime_mask[g_tmp]>0) g_opt_starttime_mask[g_tmp]--;
							else g_opt_starttime_mask[g_tmp]=5;
							break;
					}
					g_opt_starttime =36000000*g_opt_starttime_mask[0]+3600000*g_opt_starttime_mask[1]+600000*g_opt_starttime_mask[2]+60000*g_opt_starttime_mask[3]+10000*g_opt_starttime_mask[4]+1000*g_opt_starttime_mask[5];
					break;

				case 160:
					switch(g_tmp) {
						case 0:
							g_opt_startlight_above=1-g_opt_startlight_above;
							break;
						case 2:
							if(g_opt_startlight>0) g_opt_startlight--;
							else  g_opt_startlight=99;
							break;
					}
					break;

				default:
					// blink (and beeps) for wrong press
					WrongPress();
					break;
			}

			break;


		case KEYPAD_RIGHT:
			if (G_menu_id<10) {
				g_tmp=0;
				G_menu_id=G_menu_id*10;  //if in main menu jump in one of the first submenu
				break;
			} else {
				switch(G_menu_id) {
					case 10:
						//quick start 5s
						g_opt_mode=0;
						g_opt_totpic=9999;
						g_opt_time=5000;
						g_opt_time_mask[0]=0;
						g_opt_time_mask[1]=0;
						g_opt_time_mask[2]=0;
						g_opt_time_mask[3]=0;
						g_opt_time_mask[4]=0;
						g_opt_time_mask[5]=5;
						g_opt_startmode=1;
						g_opt_starttime=0;
						g_opt_starttime_mask[0]=0;
						g_opt_starttime_mask[1]=0;
						g_opt_starttime_mask[2]=0;
						g_opt_starttime_mask[3]=0;
						g_opt_starttime_mask[4]=0;
						g_opt_starttime_mask[5]=0;
						g_button_wait=BUTTON_WAIT_SUMUP;
						g_okfoto=1;	//start immediately
						g_run_mode=1;
						break;
					case 11:
						//quick start 30s
						g_opt_mode=0;
						g_opt_totpic=9999;
						g_opt_time=30000;
						g_opt_time_mask[0]=0;
						g_opt_time_mask[1]=0;
						g_opt_time_mask[2]=0;
						g_opt_time_mask[3]=0;
						g_opt_time_mask[4]=3;
						g_opt_time_mask[5]=0;
						g_opt_startmode=1;
						g_opt_starttime=0;
						g_opt_starttime_mask[0]=0;
						g_opt_starttime_mask[1]=0;
						g_opt_starttime_mask[2]=0;
						g_opt_starttime_mask[3]=0;
						g_opt_starttime_mask[4]=0;
						g_opt_starttime_mask[5]=0;
						g_button_wait=BUTTON_WAIT_SUMUP;
						g_okfoto=1;	//start immediately
						g_run_mode=1;
						break;
					case 20:
						g_tmp=5;
						G_menu_id=110;
						break;
					case 21:
						g_opt_mode=1;
						//	G_menu_id=2;
						break;
					case 22:
						g_opt_mode=2;
						//	G_menu_id=2;
						break;
					case 30:
						g_tmp=1;
						break;
					case 31: 
						G_menu_id=130;
						break;
					case 32:
						G_menu_id=140;
						break;
					case 40:
						//start!
						g_opt_startmode=1;
						g_opt_starttime=0;
						g_opt_starttime_mask[0]=0;
						g_opt_starttime_mask[1]=0;
						g_opt_starttime_mask[2]=0;
						g_opt_starttime_mask[3]=0;
						g_opt_starttime_mask[4]=0;
						g_opt_starttime_mask[5]=0;
						g_button_wait=BUTTON_WAIT_SUMUP;
						g_okfoto=1;	//start immediately
						g_run_mode=1;
						break;
					case 41:
						g_tmp=0;
						G_menu_id=150;
						break;
					case 42:
						g_tmp=0;
						G_menu_id=160;
						break;
					case 50:
						G_menu_id=170;
						break;
					case 51:
						G_menu_id=180;
						break;
					case 52:
						G_menu_id=190;
						break;
					case 60:
						G_menu_id=6;
						break;
					case 110:
						if (g_tmp==5) {
							//clicked right on the OK => go to next menu
							g_tmp=6;
							G_menu_id=120;
						} else g_tmp++;      
						break;

					case 120:
						if (g_tmp==6) {
							//clicked right on the OK => go to next menu
							g_tmp=0;
							g_opt_mode=0;
							G_menu_id=2;
						} else g_tmp++;      
						break;

					case 130:
					case 131:
					case 132:
					case 133:
						//always on
						g_opt_offlight=G_menu_id-130;
						switch(g_opt_offlight){
							case 0:
								break;
							case 1:
								g_opt_offlight_deltat = 5000;
								break;
							case 2:
								g_opt_offlight_deltat = 30000;
								break;
							case 3:
								g_opt_offlight_deltat = 0;
								break;
						}
						break;
					case 140:
					case 141:
					case 142:
						//beeps
						g_opt_beeps=G_menu_id-140;
						break;
					case 150:
						if (g_tmp==6) {
							//start!
							g_tmp=0;
							g_button_wait=BUTTON_WAIT_SUMUP;
							g_okfoto=0;
							g_run_mode=1;
						} else g_tmp++;
						break;
					case 160:
						if (g_tmp==0) g_tmp=2;
						else if (g_tmp==2) g_tmp++;
						else {
							//start!!
							g_tmp=0;
							g_opt_startmode=2;
							g_button_wait=BUTTON_WAIT_SUMUP;
							g_okfoto=0;
							g_run_mode=1;
						}
						break;
					default:
						// blink (and beeps) for wrong press
						WrongPress();
						break;
				} 
			}

			break;

		case KEYPAD_LEFT:
			switch(G_menu_id){
				case 10:
				case 11:
					G_menu_id=1;
					break;
				case 20:
				case 21:
				case 22:
					G_menu_id=2;
					break;
				case 30:
					if (g_tmp==1) {
						g_tmp=0;
						lcd.noBlink();
					} else G_menu_id=3;
					break;
				case 31:
				case 32:
					G_menu_id=3;
					break;
				case 40:
				case 41:
				case 42:
					G_menu_id=4;
					break;
				case 50:
				case 51:
				case 52:
					G_menu_id=5;
					break;
				case 60:
					G_menu_id=6;
					break;

				case 110:
					if (g_tmp==0) {
						//clicked left on leftmost number => return previous menu
						G_menu_id=20;
					} else g_tmp--;
					break;

				case 120:
					if (g_tmp==0) {
						//clicked left on leftmost number => return previous menu
						G_menu_id=110;
					} else g_tmp--;
					break;

				case 130:
				case 131:
				case 132:
				case 133:
					G_menu_id=31;
					break;

				case 140:
				case 141:
				case 142:
					G_menu_id=32;
					break;

				case 150:
					if (g_tmp==0) {
						//clicked left on leftmost number => return previous menu
						G_menu_id=41;
					} else g_tmp--;
					break;

				case 160:
					if (g_tmp==0) G_menu_id=42;
					else if (g_tmp==2) g_tmp=0;
					else g_tmp--;
					break;

				case 170:
					G_menu_id=50;
					break;
				case 180:
					G_menu_id=51;
					break;
				case 190:
					G_menu_id=52;
					break;

				default:
					// blink (and beeps) for wrong press
					WrongPress();
					break;
			} 

			break;

		default:
			break;
	}

	//update display
	show_opt_display();

	return;

}

void check_sumup_input(int button_pressed)
{
	switch(button_pressed) {
		case KEYPAD_RIGHT:
			//confirmed, run!
			g_button_wait=BUTTON_WAIT_OPT;
			g_time0=millis();	//save time in case there is a timer before start
			g_time_pic=g_time0;
			g_run_mode=2;
			break;
		case KEYPAD_LEFT:
			// cancel confirmation, back to options
			g_okfoto=0;
			g_button_wait=BUTTON_WAIT_OPT;
			g_run_mode=0;
			break;
	}
}


void check_run_input(int button_pressed)
{
	switch(button_pressed) {
		case KEYPAD_UP:
			break;
		case KEYPAD_DOWN:
			break;
		case KEYPAD_RIGHT:
			break;
		case KEYPAD_LEFT:
			break;
	}
}

int WaitButton(int waittime)
{
	unsigned long int t0=millis();
	int b0;
	while(((b0=CheckButton())==KEYPAD_NONE)&&(millis()-t0<waittime))
	{
	}
	return b0;
}

byte CheckButton(void)
{
	int a1,a2,bU,bD,bL,bR;

	bU=bD=bL=bR=0;

	a1=analogRead(PIN_IN_BUTTON_UP_LEFT);
	a2=analogRead(PIN_IN_BUTTON_DOWN_RIGHT);

	delay(150);

	if (a1>1000 && a2>1000) return KEYPAD_NONE;
	
	#ifdef DEBUG
	if (a1<10 && a2<10) {Serial.print("ALL\n");return KEYPAD_ALL;}
	if (a1<10)  {Serial.print("LEFT\n");return KEYPAD_LEFT;}
	if (a1<200) {Serial.print("UP\n");return KEYPAD_UP;}
	if (a2<10)  {Serial.print("DOWN\n");return KEYPAD_DOWN;}
	if (a2<200) {Serial.print("RIGHT\n");return KEYPAD_RIGHT;}
	#else

	if (a1<10 && a2<10) return KEYPAD_ALL;
	if (a1<10) return KEYPAD_LEFT;
	if (a1<200) return KEYPAD_UP;
	if (a2<10) return KEYPAD_DOWN;
	if (a2<200) return KEYPAD_RIGHT;

	#endif


	return KEYPAD_NONE;
}

void WrongPress()
{
	lcd.noDisplay();
	delay(150);
	lcd.display();
	if (g_opt_menubeeps==1) beep(1);
	return;
}

void check_startnow()
{
	int n;
	if ((g_opt_mode!=0) || (g_opt_mode==0 && g_tot_pic<g_opt_totpic)) {

		//check if the start conditions are met
		if (g_opt_startmode==1) {
			if (millis()-g_time0>g_opt_starttime) g_okfoto=1;
		} else {
			n=MeasureLight();
			if (g_opt_startlight_above && n>=g_opt_startlight) g_okfoto=1;
			else if (!g_opt_startlight_above && n<=g_opt_startlight) g_okfoto=1;
		}

		//reset the timer of the picture
		if (g_okfoto==1) g_time_pic=millis();
	}
}

void check_foto()
{
	unsigned long now;
	int i,n,taken;


	switch(g_opt_mode){
		case 0:
			//intervalometer
			now=millis();
			if (now-g_time_pic>g_opt_time){
				//if it is time to take a pic..
				g_time_pic=now;
				if (g_opt_focus) focus();
				take_picture();

				g_tot_pic++;
				//if reached the maximum number of pic, then switch off the okfoto variable
				//and put the system in ended mode
				if (g_tot_pic>=g_opt_totpic) {
					g_okfoto=0;
					g_opt_offlight_t0=millis();	//save end time
					g_run_mode=3;
				}
			}
			break;
		case 1:
			//shot on flash
			i=0;
			taken=0;
			do {
				n=MeasureLight();
				if (n-g_last_light>DELTA_LIGHT) {
					if (g_opt_focus) focus();
					take_picture();
					taken=1;
				}
				i++;
			} while(i<10 && taken==0);
			if (taken) {
				g_tot_pic++;
				//wait a bit before resetting the base value
				delay(500);
				g_last_light=MeasureLight();
			}
			else g_last_light=n;	//if no foto, then safe to reset the base value for the light
			break;
		case 2:
			//shot on sound
			i=0;
			taken=0;
			do {
				n=MeasureSound();
				if (n-g_last_sound>DELTA_SOUND) {
					if (g_opt_focus) focus();
					take_picture();
					taken=1;
				}
				i++;
			} while(i<10 && taken==0);
			if (taken) g_tot_pic++;		//resetting the base value may risk to pick exactly the value of the flash instead of the base
			else g_last_sound=n;		//reset the base value for the light
			break;
	}

}

void SwitchBacklight(byte status)
{
	//switch display light
	if (status==0) {
		lcd.clear();
		digitalWrite(PIN_LCD_VDD,LOW);
		g_lcd=0;
	} else {
		digitalWrite(PIN_LCD_VDD,HIGH);
		delay(10);
		lcd.createChar(0,c_right);
		lcd.createChar(1,c_dot);
		lcd.createChar(2,c_up);
		lcd.createChar(3,c_down);
		lcd.createChar(4,c_ok);
		lcd.createChar(CHECKNO,c_openbox);
		lcd.createChar(CHECKYES,c_closebox);

		lcd.begin(16, 2);
		lcd.clear();
		lcd.noCursor();
		lcd.noBlink();
		g_lcd=1;
	}
}

void take_picture()
{
	if (g_opt_beeps>0) beep(g_opt_beeps);
	if (g_lcd==1) {
		lcd.clear();
		lcd.print("Taking picture..");
	}
	digitalWrite(PIN_OUT_SHOOT,HIGH);
	delay(SHOOT_TIME);
	digitalWrite(PIN_OUT_SHOOT,LOW); 
}

void focus()
{
	if (g_lcd==1) {
		lcd.clear();
		lcd.print("focusing..");
	}
	digitalWrite(PIN_OUT_FOCUS,HIGH);
	delay(FOCUS_TIME);
	digitalWrite(PIN_OUT_FOCUS,LOW); 

}

int MeasureBattery(void)
{
	//measure battery, return number in [0;99]
	//to be implemented
	return 99;
}

int MeasureSound(void)
{
	//measure sound, return number in [0;99]
	//to be implemented
	return 79;
}

int MeasureLight(void)
{
	//measure light, return number in [0;99]
	int rawADC,light;

	rawADC = analogRead(PIN_IN_PHOTORESISTOR);
	light=int((float)rawADC*100/1024);

	return light;
}

float MeasureTemp(void)
{
	long resistance;  
	float temp;
	int raw;

	raw = analogRead(PIN_IN_THERMISTOR);
	resistance=((1024 * RESISTOR_PAD / raw) - RESISTOR_PAD);
	temp = log(resistance);
	temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
	temp = temp - 273.15;  // convert Kelvin to Celsius                      

	return temp;
}

void beep(byte type)
{
	int i,j;
	switch(type){
		case 1:
			for (i=0; i<70; i++) {		// generate a tone
				digitalWrite(PIN_OUT_SPEAKER, HIGH);
				delayMicroseconds(250);
				digitalWrite(PIN_OUT_SPEAKER, LOW);
				delayMicroseconds(250);
			} 
			break;
		case 2:
			for (j=0;j<2;j++) {
				for (i=0; i<100; i++) {	// generate another tone
					digitalWrite(PIN_OUT_SPEAKER, HIGH);
					delayMicroseconds(400);
					digitalWrite(PIN_OUT_SPEAKER, LOW);
					delayMicroseconds(400);
				} 
				delay(150);
			}
			break;
	}

}
