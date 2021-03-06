

#include <avr/interrupt.h>
#include "../global.h"
#include "heater.h"

/** 
 * Service vetor that controls the heater module
 */
ISR(TIMER2_OVF_vect) {
    static uint8_t count = 0;
    static uint8_t turn_switch = 1;
    ++count;
    if(count >= DECIDE_HEATING_INTERVAL){
        count = 0;
        turn +=  HEATER_PERIOD_FAKTOR;

         if(turn > 100){
             turn = 0;
             turn_switch = 1;
         }
                           
        bool shouldHeat = false;
        if(state != STATE_COOL){
	        int16_t diffTemp = (globalTemp /100)- targetTemp;
	        if( diffTemp > 0 ){
                shouldHeat = false;
			    data_out.control &= ~(1 << 6);
    	    } else if( diffTemp < 0 ){
    	    	shouldHeat = true;
    	    	data_out.control |= 1<< 6;
    	    } else {
    	    	// freue ich mich.
    	    }

    	} else {
    		shouldHeat = false;
    		data_out.control &= ~(1 << 6);
    	}
        
        if((turn % power) > 0){
            if(turn_switch){
                TURN_HEATER_OFF();
            } else {
                if(shouldHeat){ TURN_HEATER_ON(); } else { TURN_HEATER_OFF(); }
            }
        } else {
			--turn_switch;
		 }
    }
}

void heater_init(void){
	// set relay port
	DDRB |= (1 << PB1);
	PORTB &= ~(1 << PB1);
	
	// set state to preheat initially 
	state = STATE_PREHEAT;
	// Start temperature is 0°C = off
    targetTemp = 0;
    // The initial power is 100%
    power = 100;
    // after the timer is powerd on, the first turn of the first period starts
    turn = 0;
    
	// Timer 2 
	TCCR2B |= (1<<CS22) | (1<<CS20);        // Prescaler 1024
    TIMSK2 |= (1<<TOIE2);  
}

void heater_lock(report_data_t *data, bool lock){
	if(lock){
		data->control |= (1 << CONTROL_ISLOCKED_BIT);
	} else {
		data->control &= ~(1 << CONTROL_ISLOCKED_BIT);
	}
}

void heater_preheat(report_data_t *data, bool preheat){
	if(preheat){
		data->control |= (1 << CONTROL_PREHEAT_BIT);
	} else {
		data->control &= ~(1 << CONTROL_PREHEAT_BIT);
	}
}

void heater_reached(report_data_t *data, bool reached){
	if(reached){
		data->control |= (1 << CONTROL_REACHEDTEMP_BIT);
	} else {
		data->control &= ~(1 << CONTROL_REACHEDTEMP_BIT);
	}
}

void heater_cool(report_data_t *data, bool cool){
	if(cool){
		data->control |= (1 << CONTROL_COOLING_BIT);
	} else {
		data->control &= ~(1 << CONTROL_COOLING_BIT);
	}
}

void heater_setPower(uint8_t p){
    power = p;
}

void heater_adjust_power(uint8_t state){

    uint16_t Tc = (globalTemp /100);    // CurrentTemp
    uint16_t Tt = targetTemp;           // TargetTemp
    
    if(state == STATE_HOLD){
        heater_setPower( HEATER_IDLE_POWER );
        return;
    } else if( state == STATE_SOLDER){
        if( Tc < Tt ){
            uint16_t startAdjustTemp = (Tt - HEATER_ADJUST_TEMP_TRIGGER);
            if( Tc < startAdjustTemp ){
                heater_setPower(100);
                return;
            } // or fall throw to adjust
        } else {
            heater_setPower(0);
            return;
        }
    }
    
    // Adjust
    // get satuartion
    uint8_t saturation = ((((100 / HEATER_ADJUST_TEMP_TRIGGER) * (HEATER_ADJUST_TEMP_TRIGGER - (Tt - Tc))) -100) *-1);
    // get duty cycle range
    uint8_t dcRange = 100 - HEATER_IDLE_POWER;
    
    if( saturation > 0 && saturation < 100 ){
        uint8_t power = ((dcRange * saturation) / 100) + HEATER_IDLE_POWER;
        heater_setPower( power );
    }

}
