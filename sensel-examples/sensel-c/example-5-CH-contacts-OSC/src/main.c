/******************************************************************************************
* MIT License
*
* Copyright (c) 2013-2017 Sensel, Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
******************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
    #include <windows.h>
#else
    #include <pthread.h>
#endif
#include "sensel.h"
#include "sensel_device.h"
#include "lo/lo.h"

//static const char* CONTACT_STATE_STRING[] = { "CONTACT_INVALID","CONTACT_START", "CONTACT_MOVE", "CONTACT_END" };
static bool enter_pressed = false;

#ifdef WIN32
DWORD WINAPI waitForEnter()
#else
void * waitForEnter()
#endif
{
    getchar();
    enter_pressed = true;
    return 0;
}

int main(int argc, char **argv)
{
	//Handle that references a Sensel device
	SENSEL_HANDLE handle = NULL;
	//List of all available Sensel devices
	SenselDeviceList list;
	//SenselFrame data that will hold the contacts
	SenselFrameData *frame = NULL;
    
    lo_address t = lo_address_new(NULL, "3435");

    
    SenselStatus status;
    
	//Get a list of avaialble Sensel devices
	senselGetDeviceList(&list);
	if (list.num_devices == 0)
	{
		fprintf(stdout, "No device found\n");
		fprintf(stdout, "Press Enter to exit example\n");
		getchar();
		return 0;
	}

	//Open a Sensel device by the id in the SenselDeviceList, handle initialized 
	senselOpenDeviceByID(&handle, list.devices[0].idx);
    
	//Set the frame content to scan contact data
	senselSetFrameContent(handle, FRAME_CONTENT_CONTACTS_MASK);
	//Allocate a frame of data, must be done before reading frame data
	senselAllocateFrameData(handle, &frame);
    
    senselSetMaxFrameRate(handle,250);
    
	//Start scanning the Sensel device
    senselStartScanning(handle);
    
    fprintf(stdout, "Press Enter to exit example\n");
    #ifdef WIN32
        HANDLE thread = CreateThread(NULL, 0, waitForEnter, NULL, 0, NULL);
    #else
        pthread_t thread;
        pthread_create(&thread, NULL, waitForEnter, NULL);
    #endif
    
    while (!enter_pressed)
    {
		unsigned int num_frames = 0;
		//Read all available data from the Sensel device
		senselReadSensor(handle);
		//Get number of frames available in the data read from the sensor
		senselGetNumAvailableFrames(handle, &num_frames);
		for (int f = 0; f < num_frames; f++)
		{
			//Read one frame of data
            status = senselGetFrame(handle, frame)==-1;
			if(status == -1)return 0;
			//Print out contact data
			if (frame->n_contacts > 0) {
				//fprintf(stdout, "Num Contacts: %d\n", frame->n_contacts);
				for (int c = 0; c < frame->n_contacts; c++)
				{
					unsigned int state = frame->contacts[c].state;
					//fprintf(stdout, "Contact ID: %d State: %s X: %f Y: %f Force : %f Area : %f\n", frame->contacts[c].id, CONTACT_STATE_STRING[state],frame->contacts[c].x_pos,frame->contacts[c].y_pos,frame->contacts[c].total_force,frame->contacts[c].area);
                    
                    // /sensel/contacts id state x y tforce area orientation major_axis minor_axis delta_x delta_y delta_force delta_area peak_x peak_y peak_force
                    lo_send(t, "/sensel/contacts", "iiffffffffffffff", frame->contacts[c].id, state%3 ,frame->contacts[c].x_pos,frame->contacts[c].y_pos,frame->contacts[c].total_force,frame->contacts[c].area, frame->contacts[c].orientation,frame->contacts[c].major_axis,frame->contacts[c].minor_axis,frame->contacts[c].delta_x,frame->contacts[c].delta_y,frame->contacts[c].delta_force,frame->contacts[c].delta_area,frame->contacts[c].peak_x,frame->contacts[c].peak_y,frame->contacts[c].peak_force);

					//Turn on LED for CONTACT_START
					if (state == CONTACT_START) {
						senselSetLEDBrightness(handle, frame->contacts[c].id, 100);
					}
					//Turn off LED for CONTACT_END
					else if (state == CONTACT_END) {
						senselSetLEDBrightness(handle, frame->contacts[c].id, 0);
					}
				}
				//fprintf(stdout, "\n");
			}
		}
	}
	return 0;
}
