/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Copyright (c) Robomatter 2017, All rights reserved.                     */
/*                                                                            */
/*    Module:     main.cpp                                                    */
/*    Author:     James Pearman                                               */
/*    Created:    29 Dec 2017                                                 */
/*                                                                            */
/*    Revisions:  V0.1                                                        */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#include "vex.h"
#include "stopwatch.h"

using namespace vex;

vex::brain Brain;

#define SW_CENTER_X   (120)
#define SW_CENTER_Y   (120)
#define SW_CENTER_Y_M  (80)
#define SW_DIAL_SIZE  (106)
#define SW_DIAL_SIZE_M (30)


#define B_RST_CENTER_X  (240 +  60)
#define B_RST_CENTER_Y  (60)
#define B_STP_CENTER_X  (240 + 180)
#define B_STP_CENTER_Y  (60)
#define B_RADIUS        (40)

bool      bStopwatchRunning = false;
bool      bStopwatchReset   = false;
uint32_t  nTimerOffset    = 0;
uint32_t  nTimerStartTime = 0;
uint32_t  nTimerStopTime  = 0;

// collect data for on screen button
typedef struct _button {
    int    xpos;
    int    ypos;
    int    width;
    int    height;
    const char  *label;
    bool   state;
} button;

// Button definitions
// First 10 are for reverse setting of motor
// last two select between tank/arcade drive and single/dual control
button buttons[] = {
  {  B_RST_CENTER_X-B_RADIUS,  B_RST_CENTER_Y-B_RADIUS, (B_RADIUS*2), (B_RADIUS*2), "reset",        false },
  {  B_STP_CENTER_X-B_RADIUS,  B_STP_CENTER_Y-B_RADIUS, (B_RADIUS*2), (B_RADIUS*2), "start",        false }
};

void displayStopwatchControls( bool mode, bool bStartPressed, bool bResetPressed );

/*-----------------------------------------------------------------------------*/
/** @brief      Check if touch is inside button                                */
/*-----------------------------------------------------------------------------*/
int
findButton(  int16_t xpos, int16_t ypos ) {
    int nButtons = sizeof(buttons) / sizeof(button);

    for( int index=0;index < nButtons;index++) {
      button *pButton = &buttons[ index ];
      if( xpos < pButton->xpos || xpos > (pButton->xpos + pButton->width) )
        continue;

      if( ypos < pButton->ypos || ypos > (pButton->ypos + pButton->height) )
        continue;
      
      return(index);
    }
    return (-1);
}
/*-----------------------------------------------------------------------------*/
/** @brief      Init button states                                             */
/*-----------------------------------------------------------------------------*/
void
initButtons() {
    int nButtons = sizeof(buttons) / sizeof(button);

    for( int index=0;index < nButtons;index++) {
      buttons[index].state = false;
    }
}

/*-----------------------------------------------------------------------------*/
/** @brief      Screen has been touched                                        */
/*-----------------------------------------------------------------------------*/
void
userTouchCallbackPressed() {
    int index;
    int xpos = Brain.Screen.xPosition();
    int ypos = Brain.Screen.yPosition();
    
    if( (index = findButton( xpos, ypos )) >= 0 ) {
      displayStopwatchControls( bStopwatchRunning, (index == 1 ) ? true : false, (index == 0 ) ? true : false );
    }
    
}

/*-----------------------------------------------------------------------------*/
/** @brief      Screen has been (un)touched                                    */
/*-----------------------------------------------------------------------------*/
void
userTouchCallbackReleased() {
    int index;
    int xpos = Brain.Screen.xPosition();
    int ypos = Brain.Screen.yPosition();
    
    if( (index = findButton( xpos, ypos )) >= 0 ) {
      buttons[index].state = !buttons[index].state;

      bStopwatchRunning = buttons[1].state;

      if( index == 0 )
        bStopwatchReset = true;

      displayStopwatchControls( bStopwatchRunning, false, false );
    }
}

/*-----------------------------------------------------------------------------*/
/** @brief      Task to handle touch screen                                    */
/*-----------------------------------------------------------------------------*/
int
selectionTask() { 
		Brain.Screen.pressed( userTouchCallbackPressed );
		Brain.Screen.released( userTouchCallbackReleased );

    initButtons();
      
    while(1) {
      // All handled by callback
      vex::task::sleep(100);
    }
}

/*-----------------------------------------------------------------------------*/
/** @brief      Display the stopwatch background image                         */
/*-----------------------------------------------------------------------------*/
void
displayStopwatchFace() {
    Brain.Screen.drawImageFromBuffer( _image, (240-214)/2, (240-214)/2, 0 );
}

/*-----------------------------------------------------------------------------*/
/** @brief      Display the stopwatch second hand                              */
/*-----------------------------------------------------------------------------*/
void
drawStopwatchSecondHand( double seconds ) {
    double sangle = 0;
    if( seconds != 0 )
      sangle = (360.0 / 60.0 * seconds) * (3.14159 / 180);
  
    sangle -= (3.141592 / 2);
  
    double xpos = cos( sangle ) * SW_DIAL_SIZE;
    double ypos = sin( sangle ) * SW_DIAL_SIZE;
  
    Brain.Screen.setPenColor( vex::color(ClrDarkOrange) );
    Brain.Screen.setFillColor( vex::color::black );
  
    Brain.Screen.drawLine( SW_CENTER_X, SW_CENTER_Y, SW_CENTER_X + xpos, SW_CENTER_Y + ypos );
    if( fabs(xpos) > fabs(ypos) )
      Brain.Screen.drawLine( SW_CENTER_X, SW_CENTER_Y-1, SW_CENTER_X + xpos, SW_CENTER_Y -1 + ypos );
    else
      Brain.Screen.drawLine( SW_CENTER_X-1, SW_CENTER_Y, SW_CENTER_X -1 + xpos, SW_CENTER_Y + ypos );
  
    Brain.Screen.drawCircle( SW_CENTER_X, SW_CENTER_Y, 3, vex::color::transparent );
    Brain.Screen.drawCircle( SW_CENTER_X, SW_CENTER_Y, 2 );
}

/*-----------------------------------------------------------------------------*/
/** @brief      Display the stopwatch minute hand                              */
/*-----------------------------------------------------------------------------*/
void
drawStopwatchMinuteHand( double minutes ) {
    double angle = 0;
    if( minutes != 0 )
      angle = (360.0 / 30.0 * minutes) * (3.14159 / 180);
  
    angle -= (3.141592 / 2);
  
    double xpos = cos( angle ) * SW_DIAL_SIZE_M;
    double ypos = sin( angle ) * SW_DIAL_SIZE_M;
  
    Brain.Screen.setPenColor( vex::color(ClrDarkOrange) );
    Brain.Screen.setFillColor( vex::color::black );
  
    Brain.Screen.drawLine( SW_CENTER_X, SW_CENTER_Y_M, SW_CENTER_X + xpos, SW_CENTER_Y_M + ypos );
  
    Brain.Screen.drawCircle( SW_CENTER_X, SW_CENTER_Y_M, 2 );
}

/*-----------------------------------------------------------------------------*/
/** @brief      Display the stopwatch hands                                    */
/*-----------------------------------------------------------------------------*/
void
displayStopwatchHands( int hours, double minutes, double seconds ) {
    drawStopwatchSecondHand( seconds );
    drawStopwatchMinuteHand( minutes );
}

/*-----------------------------------------------------------------------------*/
/** @brief      Display the stopwatch time as text                             */
/*-----------------------------------------------------------------------------*/
void
displayStopwatchTime( double minutes, double seconds ) {
    Brain.Screen.setPenColor( vex::color::white );
    Brain.Screen.setFillColor( vex::color::black );
    // small font
    Brain.Screen.setFont( vex::fontType::prop20 );

    int s1 = (int) seconds;
    int s2 = (int)((seconds - s1) * 100);
    Brain.Screen.printAt( SW_CENTER_X - 36, SW_CENTER_Y + 48, true, "%02d:%02d.%02d", (int)minutes, s1, s2);

    Brain.Screen.setPenColor( vex::color(0x303030) );
    Brain.Screen.drawRectangle( 260, 130, 200, 65, vex::color(0x000020) );
    Brain.Screen.drawRectangle( 260, 130, 200, 65, vex::color::transparent );
    
    if( bStopwatchRunning )
      Brain.Screen.setPenColor( vex::color(0x20D020) );
    else
      Brain.Screen.setPenColor( vex::color(0x606060) );

    // TODO
//    vexDisplayTextSize( 7, 8 );
    Brain.Screen.setFont( vex::fontType::prop40 );
    Brain.Screen.printAt( 288, 174, false, "%02d:%02d.%02d", (int)minutes, s1, s2);
}

/*-----------------------------------------------------------------------------*/
/** @brief      Display the stopwatch start/stop and reset buttons             */
/*-----------------------------------------------------------------------------*/
void
displayStopwatchControls( bool mode, bool bStartPressed, bool bResetPressed ) {
    uint32_t  stp_fg_color;
    uint32_t  rst_fg_color;
    
    // small font
    Brain.Screen.setFont( vex::fontType::prop20 );

    if( !mode ) {
      if( bStartPressed )
        stp_fg_color = 0x102010;
      else
        stp_fg_color = 0x204020; 
      Brain.Screen.setPenColor( vex::color(stp_fg_color) );
      
      Brain.Screen.setFillColor( vex::color(stp_fg_color) );
      Brain.Screen.drawCircle( B_STP_CENTER_X, B_STP_CENTER_Y, B_RADIUS-6 );
      Brain.Screen.drawCircle( B_STP_CENTER_X, B_STP_CENTER_Y, B_RADIUS-2, vex::color::transparent );
      Brain.Screen.drawCircle( B_STP_CENTER_X, B_STP_CENTER_Y, B_RADIUS-1, vex::color::transparent );
      Brain.Screen.drawCircle( B_STP_CENTER_X, B_STP_CENTER_Y, B_RADIUS  , vex::color::transparent );

      Brain.Screen.setPenColor( vex::color(0x20D020) );
      Brain.Screen.printAt( B_STP_CENTER_X-20, B_STP_CENTER_Y+6, true, "Start");
    }
    else {
      if( bStartPressed )
        stp_fg_color = 0x401010;
      else
        stp_fg_color = 0x700000;      
      Brain.Screen.setPenColor( vex::color(stp_fg_color) );
      Brain.Screen.setFillColor( vex::color(stp_fg_color) );

      Brain.Screen.drawCircle( B_STP_CENTER_X, B_STP_CENTER_Y, B_RADIUS-6 );
      Brain.Screen.drawCircle( B_STP_CENTER_X, B_STP_CENTER_Y, B_RADIUS-2, vex::color::transparent );
      Brain.Screen.drawCircle( B_STP_CENTER_X, B_STP_CENTER_Y, B_RADIUS-1, vex::color::transparent );
      Brain.Screen.drawCircle( B_STP_CENTER_X, B_STP_CENTER_Y, B_RADIUS  , vex::color::transparent );

      Brain.Screen.setPenColor( vex::color(0xFF0000) );
      Brain.Screen.printAt( B_STP_CENTER_X-19, B_STP_CENTER_Y+6, true, "Stop");
    }
  
    if( bResetPressed )
      rst_fg_color = 0x101010;
    else
      rst_fg_color = 0x303030;      
    Brain.Screen.setPenColor( vex::color(rst_fg_color) );
    Brain.Screen.setFillColor( vex::color(rst_fg_color) );
    
    Brain.Screen.drawCircle( B_RST_CENTER_X, B_RST_CENTER_Y, B_RADIUS-6 );
    Brain.Screen.drawCircle( B_RST_CENTER_X, B_RST_CENTER_Y, B_RADIUS-2, vex::color::transparent );
    Brain.Screen.drawCircle( B_RST_CENTER_X, B_RST_CENTER_Y, B_RADIUS-1, vex::color::transparent );
    Brain.Screen.drawCircle( B_RST_CENTER_X, B_RST_CENTER_Y, B_RADIUS  , vex::color::transparent );


    Brain.Screen.setPenColor( vex::color(0xD0D0D0) );

    Brain.Screen.printAt( B_RST_CENTER_X-23, B_RST_CENTER_Y+6, true, "Reset");
}

/*-----------------------------------------------------------------------------*/
/** @brief      Wait for LCD vsync                                             */
/*-----------------------------------------------------------------------------*/

int
main() {
    double s = 0;
    double m = 0;
    bool  bLastTimerState = false;
    uint32_t  nTimer;
    
    Brain.Screen.setPenColor( black );
    Brain.Screen.drawRectangle(0, 0, SYSTEM_DISPLAY_WIDTH, SYSTEM_DISPLAY_HEIGHT );
    
    vex::task t( selectionTask );
    
    displayStopwatchControls( bStopwatchRunning, false, false );

    while(1) {
      // update rate somewhere around 30Hz
      uint32_t  t = vex::timer::system();

      // Starting
      if( !bLastTimerState && bStopwatchRunning ) {
        nTimerStartTime = t - (nTimerStopTime - nTimerStartTime);
      }
      // stopping
      if( bLastTimerState && !bStopwatchRunning ) {
        nTimerStopTime  = t;
      }
      // reset
      if( bStopwatchReset ) {
        nTimerStartTime = t;
        nTimerStopTime  = t;
        bStopwatchReset = false;
      }
      
      bLastTimerState = bStopwatchRunning;

      if( !bStopwatchRunning )
        nTimer = (nTimerStopTime - nTimerStartTime);
      else
        nTimer = (t - nTimerStartTime);          
          
      // seconds
      s = (nTimer % 60000) / 1000.0;
      // minutes
      m = (nTimer / 60000.0);
      
      displayStopwatchFace();
      displayStopwatchTime( m, s );
      displayStopwatchHands( 0, m, s );

      Brain.Screen.render();
    }
}
