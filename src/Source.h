#ifndef SOURCE_H
#define SOURCE_H

#include "Sharper.h"
// render
void GLrender(void);

// update
void runGL(int argc, char **argv);
void runCL(void);
void runBenchmark(void);

// input
void press(unsigned char key, int x, int y);
void release(unsigned char key, int x, int y);

// console output
void printHelp(void);			// shows help menu in interactive mode
void printReset(void);			// shows values after alfa, beta, gama, radius or sigma reset
inline void printUsage(void) {			// shows argument usage and help
	system("cls");
	cout << endl << "=======================================================================================";
	cout << endl << "Available options: (default values between <>)";
	cout << endl << "help\t\t shows this help screen;";
	cout << endl << "image:_path\t loads specific image to be processed from path <image:lena.ppm>";
	cout << endl << "using:_device\t specifies the hardware to run openCL on: CPU or GPU <using:GPU>";
	cout << endl << "radius:5\t specifies the radius of the kernel <radius:3>";
	cout << endl << "cycles:10\t Sets how many times to run the kernel when calculating average run time <cycles:1>";
	cout << endl << "measure:10\t Enters CPU&GPU benchmark mode sequentially measuring runtime for kernels with radii from 1 to 10 <measure:5>";
	cout << endl << "kernel:_option\t Options are average or gaussian : select the running kernel <kernel:average>";
	cout << endl << "export:_option\t Options are blur, delta, sharp : what to save on the output image <export:sharp>";
	cout << endl << "saveAs:out.ppm\t specify the name of the output image <saveAs:output.ppm>";
	cout << endl << "alpha:0.3\t specify ALPHA value in kernel <alpha:1.5>";
	cout << endl << "beta:-1.15\t specify BETA value in kernel <beta:-2.0>";
	cout << endl << "gamma:0.64\t specify GAMMA value in kernel <gamma:0>";
	cout << endl << "radius:1\t specify the radius of the gaussian grid <radius:3>";
	cout << endl << "sigma:0.95\t specify the value of the sigma used to generate the gaussian grid <sigma:1>";
	cout << endl << "interactive\t enables the interactive mode that uses openGL";
	cout << endl << "verbose\t\t enables logging during execution, this will also display the gaussian grid values";
	cout << endl << "=======================================================================================";
	cout << endl << "Example usage:";
	cout << endl << "sharpen.exe interactive image:../../lena.ppm";
	cout << endl << "sharpen.exe image:../../lena.ppm using:CPU radius:3 cycles:10";
	cout << endl << "sharpen.exe image:lena.ppm measure:5 cycles:20";
	cout << endl << "sharpen.exe help";
	cout << endl << "sharpen.exe image.lena.ppm verbose saveAs:ImageOutput.ppm";
	cout << endl << "=======================================================================================";
	cout << endl << "Press any key to continue";
	cout << endl << "=======================================================================================";
	cin.get();
	system("cls");
}

inline void execute(Sharper &toCall) {
	toCall.run();
}

#endif // !SOURCE_H
