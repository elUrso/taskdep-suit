/*
 * Copyright (C) 2013 Michael Andersch <michael.andersch@mailbox.tu-berlin.de>
 *
 * This file is part of Starbench.
 *
 * Starbench is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Starbench is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Starbench.  If not, see <http://www.gnu.org/licenses/>.
 */


/**********************************************************************************
				INCLUDES & DEFINES
*************************************************************************************/
#include <sys/time.h>
#include <stdlib.h>
#include "rotation_engine.h"
#include "ray_engine.h"

#include <omp.h>

#define BAD_EXIT -1;
#define TIME(x) gettimeofday(&x,NULL)

typedef struct timeval timer;
using namespace std;

/**********************************************************************************
				FUNCTION PROTOTYPES
*************************************************************************************/
static long timevaldiff(timer* start, timer* finish);
string* convertToString(char **in, size_t size);
bool parseArgs(string* args, unsigned int &angle, unsigned int& xres, unsigned int& yres, unsigned int& rpp, string &inname, string &outname, int& flag);

/* GLOBAL VARIABLES */
string usage =  "Usage: ./ray-rot <infile> <outfile> <angle> <xres> <yres> <RPP> <flag>\n\n"
                "infile:      input file\n"
                "outfile:     output file\n"
                "angle:       angle to be rotated\n"
                "xres:        horizontal resolution\n"
                "yres:        vertical resolution\n"
                "RPP:         rays shot per pixel\n"
                "flag:         0 for sequential or 1 for parallel\n";

string p_name = "--- StarBENCH - ray-rot Workload ---\n";

/*
*	Function: main
*	--------------
*	The program main function.
*/
int main(int argc, char* argv[]) {
    cout << p_name;

    if(argc != 8) {
		cerr << usage;
		return BAD_EXIT;
    }

    string srcfile, destfile;
    unsigned int angle;
    unsigned int xres = 1024, yres = 768;
    unsigned int rpp = 1;
    int flag;
    timer start, finish;
    RotateEngine re;
    RayEngine ra;

    string *args = convertToString(argv, argc);
    if(!parseArgs(args, angle, xres, yres, rpp, srcfile, destfile, flag)) {
        cerr << usage;
        return BAD_EXIT;
    }
	delete [] args;

    if(!ra.init(srcfile, xres, yres, rpp)) {
        cerr << "Raytracing Kernel Init failed!" << endl;
        return BAD_EXIT;
    }
    if(!re.init(ra.getOutputImage(), angle, destfile)) {
        cerr << "Rotation Kernel Init failed!" << endl;
        return BAD_EXIT;
    }

//     ra.printRaytracingState();
// 	re.printRotationState();
    if (flag) {
    	TIME(start);
        ra.run();
        #pragma omp barrier
        re.run();
        #pragma omp barrier
    	TIME(finish);

        ra.finish();
        re.finish();
    } else {
        TIME(start);
        ra.run_seq();
        re.run_seq();
        TIME(finish);

        ra.finish();
        re.finish();

    }

    cout << "Time: " << (double)timevaldiff(&start, &finish)/1000 << endl;

    return 0;
}

/*
*   Function: convertToString
*   -------------------------
*   Converts the c-string program arguments into c++-strings and returns
*   a pointer to an array of such strings.
*/
string* convertToString(char** in, size_t size) {
    string* args = new string[size];
    for(size_t i = 0; i < size; i++) {
       args[i] = in[i];
    }
    return args;
}

/*
*   Function: parseArgs
*   -------------------
*   Extracts the rotation angle as well as the in- and output file names
*   from the string array args, storing them in the specified variables.
*/
bool parseArgs(string* args, unsigned int &angle, unsigned int& xres, unsigned int& yres, unsigned int& rpp, string &inname, string &outname, int& flag) {
    const char *tmp = args[3].c_str();
    angle = atoi(tmp) % 360;
    xres = atoi(args[4].c_str());
    yres = atoi(args[5].c_str());
    rpp = atoi(args[6].c_str());
    flag = atoi(args[7].c_str());

    if (angle < 0 || xres <= 0 || yres <= 0 || rpp < 1) {
        cerr << "Bad arguments, exiting" << endl;
        exit(-1);
    }
    if (flag < 0 || flag > 1){
        cerr << "Flag must be 0 (sequential) or 1 (parallel)" << endl;
        exit(-1);
    }


    inname = args[1];
    outname = args[2];
    return true;
}

/*
*   Function: timevaldiff
*   ---------------------
*   Provides a millisecond-resolution timer, computing the elapsed time
*   in between the two given timeval structures.
*/
static long timevaldiff(timer* start, timer* finish){
	long msec;
	msec = (finish->tv_sec - start->tv_sec)*1000;
	msec += (finish->tv_usec - start->tv_usec)/1000;
	return msec;
}
