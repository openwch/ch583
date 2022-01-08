#include "count_steps.h"
#include "stdint.h"
#include "stdio.h"  //using this for printing debug outputs

//this algorithm is a simple adaptation of the following paper:
//"RecoFit - Using a Wearable Sensor to Find, Recognize, and Count Repetitive Exercises"

#define NUM_AUTOCORR_LAGS       50          //number of lags to calculate for autocorrelation. 50 lags @20Hz corresponds to a step rate of 0.4Hz...its probably not physically possible to walk much slower than this
#define DERIV_FILT_LEN          5           //length of derivative filter
#define LPF_FILT_LEN            9           //length of FIR low pass filter
#define AUTOCORR_DELTA_AMPLITUDE_THRESH 5e8 //this is the min delta between peak and trough of autocorrelation peak
#define AUTOCORR_MIN_HALF_LEN   3           //this is the min number of points the autocorrelation peak should be on either side of the peak

static int8_t deriv_coeffs[DERIV_FILT_LEN]        = {-6,31,0,-31,6};            //coefficients of derivative filter from https://www.dsprelated.com/showarticle/814.php
static int8_t lpf_coeffs[LPF_FILT_LEN]            = {-5,6,34,68,84,68,34,6,-5}; //coefficients of FIR low pass filter generated in matlab using FDATOOL
static uint8_t mag_sqrt[NUM_TUPLES]               = {0};                        //this holds the square root of magnitude data of X,Y,Z (so its length is NUM_SAMPLES/3)
static int32_t lpf[NUM_TUPLES]                    = {0};                        //hold the low pass filtered signal
static int64_t autocorr_buff[NUM_AUTOCORR_LAGS]   = {0};                        //holds the autocorrelation results
static int64_t deriv[NUM_AUTOCORR_LAGS]           = {0};                        //holds derivative

static uint32_t SquareRoot(uint32_t a_nInput);
static void derivative(int64_t *autocorr_buff, int64_t *deriv);
static void autocorr(int32_t *lpf, int64_t *autocorr_buff);
static void remove_mean(int32_t *lpf);
static void lowpassfilt(uint8_t *mag_sqrt, int32_t *lpf);
static uint8_t get_precise_peakind(int64_t *autocorr_buff, uint8_t peak_ind);
static void get_autocorr_peak_stats(int64_t *autocorr_buff, uint8_t *neg_slope_count, int64_t *delta_amplitude_right, uint8_t *pos_slope_count, int64_t *delta_amplitude_left, uint8_t peak_ind);

//fixed point square root estimation from http://stackoverflow.com/questions/1100090/looking-for-an-efficient-integer-square-root-algorithm-for-arm-thumb2
/**
 * \brief    Fast Square root algorithm
 *
 * Fractional parts of the answer are discarded. That is:
 *      - SquareRoot(3) --> 1
 *      - SquareRoot(4) --> 2
 *      - SquareRoot(5) --> 2
 *      - SquareRoot(8) --> 2
 *      - SquareRoot(9) --> 3
 *
 * \param[in] a_nInput - unsigned integer for which to find the square root
 *
 * \return Integer square root of the input value.
 */
static uint32_t SquareRoot(uint32_t a_nInput) {
    uint32_t op  = a_nInput;
    uint32_t res = 0;
    uint32_t one = 1uL << 30; // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type
    
    // "one" starts at the highest power of four <= than the argument.
    while (one > op) {
        one >>= 2;
    }
    
    while (one != 0) {
        if (op >= res + one) {
            op  = op - (res + one);
            res = res +  2 * one;
        }
        res >>= 1;
        one >>= 2;
    }
    return res;
}


//take a look at the original autocorrelation signal at index i and see if
//its a real peak or if its just a fake "noisy" peak corresponding to
//non-walking. basically just count the number of points of the
//autocorrelation peak to the right and left of the peak. this function gets
//the number of points to the right and left of the peak, as well as the delta amplitude
static void get_autocorr_peak_stats(int64_t *autocorr_buff, uint8_t *neg_slope_count, int64_t *delta_amplitude_right, uint8_t *pos_slope_count, int64_t *delta_amplitude_left, uint8_t peak_ind) {
    
    //first look to the right of the peak. walk forward until the slope begins decreasing
    uint8_t  neg_slope_ind = peak_ind;
    uint16_t loop_limit    = NUM_AUTOCORR_LAGS-1;
    while ((autocorr_buff[neg_slope_ind+1] - autocorr_buff[neg_slope_ind] < 0) && (neg_slope_ind < loop_limit)) {
        *neg_slope_count = *neg_slope_count + 1;
        neg_slope_ind    = neg_slope_ind + 1;
    }


    //get the delta amplitude between peak and right trough
    *delta_amplitude_right = autocorr_buff[peak_ind] - autocorr_buff[neg_slope_ind];

    //next look to the left of the peak. walk backward until the slope begins increasing
    uint8_t pos_slope_ind = peak_ind;
    loop_limit    = 0;
    while ((autocorr_buff[pos_slope_ind] - autocorr_buff[pos_slope_ind-1] > 0) && (pos_slope_ind > loop_limit)) {
        *pos_slope_count = *pos_slope_count + 1;
        pos_slope_ind    = pos_slope_ind - 1;
    }
    
    //get the delta amplitude between the peak and the left trough
    *delta_amplitude_left = autocorr_buff[peak_ind] - autocorr_buff[pos_slope_ind];
}


//use the original autocorrelation signal to hone in on the
//exact peak index. this corresponds to the point where the points to the
//left and right are less than the current point
static uint8_t get_precise_peakind(int64_t *autocorr_buff, uint8_t peak_ind) {
    uint8_t loop_limit = 0;
    if ((autocorr_buff[peak_ind] > autocorr_buff[peak_ind-1]) && (autocorr_buff[peak_ind] > autocorr_buff[peak_ind+1])) {
        //peak_ind is perfectly set at the peak. nothing to do
    }
    else if ((autocorr_buff[peak_ind] > autocorr_buff[peak_ind+1]) && (autocorr_buff[peak_ind] < autocorr_buff[peak_ind-1])) {
        //peak is to the left. keep moving in that direction
        loop_limit = 0;
        while ((autocorr_buff[peak_ind] > autocorr_buff[peak_ind+1]) && (autocorr_buff[peak_ind] < autocorr_buff[peak_ind-1]) && (loop_limit < 10)) {
            peak_ind = peak_ind - 1;
            loop_limit++;
        }
    }
    else {
        //peak is to the right. keep moving in that direction
        loop_limit = 0;
        while ((autocorr_buff[peak_ind] > autocorr_buff[peak_ind-1]) && (autocorr_buff[peak_ind] < autocorr_buff[peak_ind+1]) && (loop_limit < 10)) {
            peak_ind = peak_ind + 1;
            loop_limit++;
        }
    }
    return peak_ind;
}


//calculate deriviative via FIR filter
static void derivative(int64_t *autocorr_buff, int64_t *deriv) {
    
    uint8_t n          = 0;
    uint8_t i          = 0;
    int64_t temp_deriv = 0;
    for (n = 0; n < NUM_AUTOCORR_LAGS; n++) {
        temp_deriv = 0;
        for (i = 0; i < DERIV_FILT_LEN; i++) {
            if (n-i >= 0) {     //handle the case when n < filter length
                temp_deriv += deriv_coeffs[i]*autocorr_buff[n-i];
            }
        }
        deriv[n] = temp_deriv;
    }
}


//autocorrelation function
//this takes a lot of computation. there are efficient implementations, but this is more intuitive
static void autocorr(int32_t *lpf, int64_t *autocorr_buff) {
    
    uint8_t lag;
    uint16_t i;
    int64_t temp_ac;
    for (lag = 0; lag < NUM_AUTOCORR_LAGS; lag++) {
        temp_ac = 0;
        for (i = 0; i < NUM_TUPLES-lag; i++) {
            temp_ac += (int64_t)lpf[i]*(int64_t)lpf[i+lag];
        }
        autocorr_buff[lag] = temp_ac;
    }
}


//calculate and remove the mean
static void remove_mean(int32_t *lpf) {
    
    int32_t sum = 0;
    uint16_t i;
    for (i = 0; i < NUM_TUPLES; i++) {
        sum += lpf[i];
    }
    sum = sum/(NUM_TUPLES);
    
    for (i = 0; i < NUM_TUPLES; i++) {
        lpf[i] = lpf[i] - sum;
    }
}


//FIR low pass filter
static void lowpassfilt(uint8_t *mag_sqrt, int32_t *lpf) {
    
    uint16_t n;
    uint8_t i;
    int32_t temp_lpf;
    for (n = 0; n < NUM_TUPLES; n++) {
        temp_lpf = 0;
        for (i = 0; i < LPF_FILT_LEN; i++) {
            if (n-i >= 0) {     //handle the case when n < filter length
                temp_lpf += (int32_t)lpf_coeffs[i]*(int32_t)mag_sqrt[n-i];
            }
        }
        lpf[n] = temp_lpf;
    }
}


//algorithm interface
uint8_t count_steps(int8_t *data) {
    
    //assume data is in the format data = [x1,y1,z1,x2,y2,z2...etc]
    //calculate the magnitude of each of triplet ie temp_mag = [x1^2+y1^2+z1^2]
    //then temp_mag = sqrt(temp_mag)
    uint16_t i;
    uint16_t temp_mag;
    for (i = 0; i < NUM_TUPLES; i++) {
        temp_mag = (uint16_t)((uint16_t)data[i*3+0]*(uint16_t)data[i*3+0] + (uint16_t)data[i*3+1]*(uint16_t)data[i*3+1] + (uint16_t)data[i*3+2]*(uint16_t)data[i*3+2]);
        mag_sqrt[i] = (uint8_t)SquareRoot(temp_mag);

    }
    
    //apply low pass filter to mag_sqrt, result is stored in lpf
    lowpassfilt(mag_sqrt, lpf);
    
    //remove mean from lpf, store result in lpf
    remove_mean(lpf);
    
    //do the autocorrelation on the lpf buffer, store the result in autocorr_buff
    autocorr(lpf, autocorr_buff);
    
    //get the derivative of the autocorr_buff, store in deriv
    derivative(autocorr_buff, deriv);
    
    //look for first zero crossing where derivative goes from positive to negative. that
    //corresponds to the first positive peak in the autocorrelation. look at two samples
    //instead of just one to maybe reduce the chances of getting tricked by noise
    uint8_t peak_ind = 0;
    //start index is set to 8 lags, which corresponds to a walking rate of 2.5Hz @20Hz sampling rate. its probably
    //running if its faster than this
    for (i = 8; i < NUM_AUTOCORR_LAGS; i++) {
        if ((deriv[i] > 0) && (deriv[i-1] > 0) && (deriv[i-2] < 0) && (deriv[i-3] < 0)) {
            peak_ind = i-1;
            break;
        }
    }
    
    //hone in on the exact peak index
    peak_ind = get_precise_peakind(autocorr_buff, peak_ind);
    //printf("peak ind: %i\n", peak_ind);
    
    //get autocorrelation peak stats
    uint8_t neg_slope_count = 0;
    int64_t delta_amplitude_right = 0;
    uint8_t pos_slope_count = 0;
    int64_t delta_amplitude_left = 0;
    get_autocorr_peak_stats(autocorr_buff, &neg_slope_count, &delta_amplitude_right, &pos_slope_count, &delta_amplitude_left, peak_ind);


    //now check the conditions to see if it was a real peak or not, and if so, calculate the number of steps
    uint8_t num_steps = 0;


    if ((pos_slope_count > AUTOCORR_MIN_HALF_LEN) && (neg_slope_count > AUTOCORR_MIN_HALF_LEN) && (delta_amplitude_right > AUTOCORR_DELTA_AMPLITUDE_THRESH) && (delta_amplitude_left > AUTOCORR_DELTA_AMPLITUDE_THRESH)) {
        //the period is peak_ind/sampling_rate seconds. that corresponds to a frequency of 1/period
        //with the frequency known, and the number of seconds is 4 seconds, you can then find out the number of steps
        num_steps = (SAMPLING_RATE*WINDOW_LENGTH)/peak_ind;
    } else {
        //not a valid autocorrelation peak
        num_steps = 0;
    }
    
    //printf("num steps: %i\n", num_steps);
    return num_steps;
}



