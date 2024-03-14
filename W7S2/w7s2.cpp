/*
 *  SLAMTEC LIDAR
 *  Simple Data Grabber Demo App
 *
 *  Copyright (c) 2009 - 2014 RoboPeak Team
 *  http://www.robopeak.com
 *  Copyright (c) 2014 - 2020 Shanghai Slamtec Co., Ltd.
 *  http://www.slamtec.com
 *
 */
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "Include/sl_lidar.h" 
#include "Include/sl_lidar_driver.h"

#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>

//For CG2111A Starts
const int DEF_MARGIN = 20;
const int DISP_RING_ABS_DIST  = 100;
const float DISP_FULL_DIST    = 16000;
const float DISP_DEFAULT_DIST = 8000;
const float DISP_MIN_DIST     = 1000;
const float PI   = (float)3.14159265;
//For CG2111A Ends

static inline void delay(sl_word_size_t ms){
    while (ms>=1000){
        usleep(1000*1000);
        ms-=1000;
    };
    if (ms!=0)
        usleep(ms*1000);
}
#endif

using namespace sl;

void print_usage(int argc, const char * argv[])
{
    printf("Simple LIDAR data grabber for SLAMTEC LIDAR.\n"
           "Version:  %s \n"
           "Usage:\n"
           " For serial channel %s --channel --serial <com port> [baudrate]\n"
           "The baudrate is 115200 for A1.\n",
            argv[0], argv[0]);
}


sl_result capture_and_display(ILidarDriver * drv, const char* fname)
{
    sl_result ans;
    
	sl_lidar_response_measurement_node_hq_t nodes[8192];
    size_t   count = _countof(nodes);

    float angle;
    float dist;
    int   quality;

    FILE* outputFile;
    
    printf("waiting for data...\n");

    ans = drv->grabScanDataHq(nodes, count, 0);
    if (SL_IS_OK(ans) || ans == SL_RESULT_OPERATION_TIMEOUT) {
        drv->ascendScanData(nodes, count);
        
        outputFile = fopen(fname, "w");

        for (int pos = 0; pos < (int)count ; ++pos) {
            quality = nodes[pos].quality;
            angle = (nodes[pos].angle_z_q14 * 90.0f) / 16394.0f;
            dist = nodes[pos].dist_mm_q2 / 4.0f;

            //Print Debug info on screen
            printf("Debug [theta: %03.2f Dist: %08.2f]\n", angle, dist);


            //Calculate the scale of pixel vs distance, 
            //     i.e. 1 pixel = ?? meter
            float distScale = 300/DISP_DEFAULT_DIST;  //300 pixel / default_dist
            float distPixel = dist*distScale;

            //Convert angle to radian
            float rad = (float)(angle*PI/180.0);

            //assume a 320 x 320 pixels display, then center point 
            // (location of the RPLidar unit) is at (x=160, y=160) 
            float centerPtX = 160;
            float centerPtY = 160;

        
            //TODO: Figure out the transformation from angle+distnace
            // to (X,Y) coordinate
            float endptX = centerPtX; //change this            float endptY = centerPtY; //change this

            //Quality of the data is represented by brightness
            //Note: Not used for our studio
            int brightness = (quality<<1) + 128;
            if (brightness>255) brightness=255;

            //Print the data into a output data file
            fprintf(outputFile, "%d\t%d\t%d\n",(int)endptX,(int)endptY, brightness);

            
        }

        fclose(outputFile);
        printf("** Scan Complete! **\n\n");
    } else {
        printf("error code: %x\n", ans);
    }

    return ans;
}

int main(int argc, const char * argv[]) {
	const char * opt_channel = NULL;
    const char * opt_channel_param_first = NULL;
    sl_u32         opt_channel_param_second = 0;
    sl_result     op_result;
	int          opt_channel_type = CHANNEL_TYPE_SERIALPORT;

    IChannel* _channel;

    if (argc < 5) {
        print_usage(argc, argv);
        return -1;
    }

	const char * opt_is_channel = argv[1];
	if(strcmp(opt_is_channel, "--channel")==0)
	{
		opt_channel = argv[2];
		if(strcmp(opt_channel, "-s")==0||strcmp(opt_channel, "--serial")==0)
		{
			opt_channel_param_first = argv[3];
			if (argc>4) opt_channel_param_second = strtoul(argv[4], NULL, 10);
		}
		else if(strcmp(opt_channel, "-u")==0||strcmp(opt_channel, "--udp")==0)
		{
			opt_channel_param_first = argv[3];
			if (argc>4) opt_channel_param_second = strtoul(argv[4], NULL, 10);
			opt_channel_type = CHANNEL_TYPE_UDP;
		}
		else
		{
			print_usage(argc, argv);
			return -1;
		}
	}
    else
	{
		print_usage(argc, argv);
		return -1;
	}

    // create the driver instance
	ILidarDriver * drv = *createLidarDriver();

    if (!drv) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }

    sl_lidar_response_device_health_t healthinfo;
    sl_lidar_response_device_info_t devinfo;
    do {
        // try to connect
        if (opt_channel_type == CHANNEL_TYPE_SERIALPORT) {
            _channel = (*createSerialPortChannel(opt_channel_param_first, opt_channel_param_second));
        }
        else if (opt_channel_type == CHANNEL_TYPE_UDP) {
            _channel = *createUdpChannel(opt_channel_param_first, opt_channel_param_second);
        }
        
        if (SL_IS_FAIL((drv)->connect(_channel))) {
			switch (opt_channel_type) {	
				case CHANNEL_TYPE_SERIALPORT:
					fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n"
						, opt_channel_param_first);
					break;
				case CHANNEL_TYPE_UDP:
					fprintf(stderr, "Error, cannot connect to the ip addr  %s with the udp port %s.\n"
						, opt_channel_param_first, opt_channel_param_second);
					break;
			}
        }

        // retrieving the device info
        ////////////////////////////////////////
        op_result = drv->getDeviceInfo(devinfo);

        if (SL_IS_FAIL(op_result)) {
            if (op_result == SL_RESULT_OPERATION_TIMEOUT) {
                // you can check the detailed failure reason
                fprintf(stderr, "Error, operation time out.\n");
            } else {
                fprintf(stderr, "Error, unexpected error, code: %x\n", op_result);
                // other unexpected result
            }
            break;
        }

        // print out the device serial number, firmware and hardware version number..
        printf("SLAMTEC LIDAR S/N: ");
        for (int pos = 0; pos < 16 ;++pos) {
            printf("%02X", devinfo.serialnum[pos]);
        }

        printf("\n"
                "Version:  %s \n"
                "Firmware Ver: %d.%02d\n"
                "Hardware Rev: %d\n"
                , "SL_LIDAR_SDK_VERSION"
                , devinfo.firmware_version>>8
                , devinfo.firmware_version & 0xFF
                , (int)devinfo.hardware_version);


        // check the device health
        ////////////////////////////////////////
        op_result = drv->getHealth(healthinfo);
        if (SL_IS_OK(op_result)) { // the macro IS_OK is the preperred way to judge whether the operation is succeed.
            printf("Lidar health status : ");
            switch (healthinfo.status) 
			{
				case SL_LIDAR_STATUS_OK:
					printf("OK.");
					break;
				case SL_LIDAR_STATUS_WARNING:
					printf("Warning.");
					break;
				case SL_LIDAR_STATUS_ERROR:
					printf("Error.");
					break;
            }
            printf(" (errorcode: %d)\n", healthinfo.error_code);

        } else {
            fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n", op_result);
            break;
        }


        if (healthinfo.status == SL_LIDAR_STATUS_ERROR) {
            fprintf(stderr, "Error, slamtec lidar internal error detected. Please reboot the device to retry.\n");
            // enable the following code if you want slamtec lidar to be reboot by software
            // drv->reset();
            break;
        }

		switch (opt_channel_type) 
		{	
			case CHANNEL_TYPE_SERIALPORT:
				drv->setMotorSpeed();
			break;
		}

        // take only one 360 deg scan and display the result as a histogram
        ////////////////////////////////////////////////////////////////////////////////
        if (SL_IS_FAIL(drv->startScan( 0,1 ))) // you can force slamtec lidar to perform scan operation regardless whether the motor is rotating
        {
            fprintf(stderr, "Error, cannot start the scan operation.\n");
            break;
        }

		delay(3000);

        if (SL_IS_FAIL(capture_and_display(drv, "lidar_reading.dat"))) {
            fprintf(stderr, "Error, cannot grab scan data.\n");
            break;

        }

    } while(0);

    drv->stop();
    switch (opt_channel_type) 
	{	
		case CHANNEL_TYPE_SERIALPORT:
			delay(20);
			drv->setMotorSpeed(0);
		break;
	}
    if(drv) {
        delete drv;
        drv = NULL;
    }
    return 0;
}
