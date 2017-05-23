#pragma once
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#include <CL\cl.hpp>
#include <GL\glut.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <chrono>
#include <set>
#include <cerrno>
#include "Kernels.h"

using namespace std;
using namespace cl;

#define QUIT true

static const ImageFormat IMAGE_FORMAT(CL_RGBA, CL_UNSIGNED_INT8);

enum Save {
	BLUR_ONLY,
	SHARP_DELTA,
	SHARP
};

enum TimeUnit {
	NANOSECOND,
	MICROSECOND,
	MILLISECOND,
	SECOND
};

// function created after analysing various sources: 
//	- http://dev.theomader.com/gaussian-kernel-calculator/
//  - http://dev.theomader.com/scripts/gaussian_weights.js
// and generalising original code for a 5x5 kernel from
// http://www.codewithc.com/gaussian-filter-generation-in-c/
inline float* getGaussianFilter(bool verbose, bool normalized, int radius, double sigma = 1.0) {
	int gridLength = radius * 2 + 1;
	int gridSize = gridLength * gridLength;
	float *grid = new float[gridSize];
	float sum = 0.0f;
	double r = 0.0;
	double s = 2 * sigma * sigma;

	for (int x = -radius; x <= radius; x++) {
		for (int y = -radius; y <= radius; y++) {
			auto index = x + radius + (y + radius)*(gridLength);
			r = sqrt(x * x + y * y);
			grid[index] = exp(-r * r / s) / (sqrt(2 * CL_M_PI) * sigma);
			sum += grid[index];
		}
	}

	if (verbose) {
		cout << endl << "Grid used: ";
	}
	for (auto index = 0; index < gridSize; index++) {
		if (normalized) {
			grid[index] /= sum;
		}
		if (verbose) {
			if (index % gridLength == 0) {
				cout << endl;
			}
			printf("%06.7f\t", grid[index]);
		}
	}
	if (verbose) {
		cout << endl;
	}
	return grid;
}

// http://stackoverflow.com/questions/24326432/convenient-way-to-show-opencl-error-codes
inline const string getError(cl_int error)
{
	switch (error) {
		// run-time and JIT compiler errors
	case 0: return "CL_SUCCESS";
	case -1: return "CL_DEVICE_NOT_FOUND";
	case -2: return "CL_DEVICE_NOT_AVAILABLE";
	case -3: return "CL_COMPILER_NOT_AVAILABLE";
	case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case -5: return "CL_OUT_OF_RESOURCES";
	case -6: return "CL_OUT_OF_HOST_MEMORY";
	case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
	case -8: return "CL_MEM_COPY_OVERLAP";
	case -9: return "CL_IMAGE_FORMAT_MISMATCH";
	case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case -11: return "CL_BUILD_PROGRAM_FAILURE";
	case -12: return "CL_MAP_FAILURE";
	case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
	case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
	case -15: return "CL_COMPILE_PROGRAM_FAILURE";
	case -16: return "CL_LINKER_NOT_AVAILABLE";
	case -17: return "CL_LINK_PROGRAM_FAILURE";
	case -18: return "CL_DEVICE_PARTITION_FAILED";
	case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

		// compile-time errors
	case -30: return "CL_INVALID_VALUE";
	case -31: return "CL_INVALID_DEVICE_TYPE";
	case -32: return "CL_INVALID_PLATFORM";
	case -33: return "CL_INVALID_DEVICE";
	case -34: return "CL_INVALID_CONTEXT";
	case -35: return "CL_INVALID_QUEUE_PROPERTIES";
	case -36: return "CL_INVALID_COMMAND_QUEUE";
	case -37: return "CL_INVALID_HOST_PTR";
	case -38: return "CL_INVALID_MEM_OBJECT";
	case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case -40: return "CL_INVALID_IMAGE_SIZE";
	case -41: return "CL_INVALID_SAMPLER";
	case -42: return "CL_INVALID_BINARY";
	case -43: return "CL_INVALID_BUILD_OPTIONS";
	case -44: return "CL_INVALID_PROGRAM";
	case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
	case -46: return "CL_INVALID_KERNEL_NAME";
	case -47: return "CL_INVALID_KERNEL_DEFINITION";
	case -48: return "CL_INVALID_KERNEL";
	case -49: return "CL_INVALID_ARG_INDEX";
	case -50: return "CL_INVALID_ARG_VALUE";
	case -51: return "CL_INVALID_ARG_SIZE";
	case -52: return "CL_INVALID_KERNEL_ARGS";
	case -53: return "CL_INVALID_WORK_DIMENSION";
	case -54: return "CL_INVALID_WORK_GROUP_SIZE";
	case -55: return "CL_INVALID_WORK_ITEM_SIZE";
	case -56: return "CL_INVALID_GLOBAL_OFFSET";
	case -57: return "CL_INVALID_EVENT_WAIT_LIST";
	case -58: return "CL_INVALID_EVENT";
	case -59: return "CL_INVALID_OPERATION";
	case -60: return "CL_INVALID_GL_OBJECT";
	case -61: return "CL_INVALID_BUFFER_SIZE";
	case -62: return "CL_INVALID_MIP_LEVEL";
	case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
	case -64: return "CL_INVALID_PROPERTY";
	case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
	case -66: return "CL_INVALID_COMPILER_OPTIONS";
	case -67: return "CL_INVALID_LINKER_OPTIONS";
	case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

		// extension errors
	case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
	case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
	case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
	case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
	case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
	case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
	default: return "Unknown OpenCL error";
	}
};

inline static void onError(cl_uint error, string exitMessage, bool stopApplication = false) {
	if (error != CL_SUCCESS) {
		cerr << exitMessage << endl << getError(error) << endl;
		if (stopApplication) {
			cerr << "The application will exit on key press...";
			cin.get();
			exit(-1);
		}
	}
};

inline const string getKernelNameFrom(string source) {
	// get everything from __kernel until eof
	auto index = source.find("__kernel");
	string name = source.substr(index);

	// place the index after the void keyword
	name = name.substr(name.find_first_of("void") + 5);

	// remove any whitespaces before the kernel name
	while (name[0] == ' ') {
		name = name.substr(1);
	}

	// narrow down the name to go until the first occurance of an open bracket
	name = name.substr(0, name.find_first_of("("));

	// remove any white spaces after the kernel name
	while (name[name.length() - 1] == ' ') {
		name = name.substr(0, name.length() - 1);
	}

	return name;
};

inline const char* getKernelNameCharFrom(string source) {
	// c-style string if needed
	return getKernelNameFrom(source).c_str();
};

inline string textify(cl_device_type deviceType) {
	string errorSuffix = "";
	switch (deviceType) { 
	case CL_DEVICE_TYPE_GPU: errorSuffix = "GPU"; return errorSuffix;
	case CL_DEVICE_TYPE_CPU: errorSuffix = "CPU"; return errorSuffix;
	case CL_DEVICE_TYPE_ACCELERATOR: errorSuffix = "ACCELERATOR DEVICE"; return errorSuffix;
	case CL_DEVICE_TYPE_CUSTOM: errorSuffix = "CUSTOM DEVICE"; return errorSuffix;
	case CL_DEVICE_TYPE_ALL: errorSuffix = "DEVICE OF ANY TYPE"; return errorSuffix;
	case CL_DEVICE_TYPE_DEFAULT: errorSuffix = "DEVICE SET AS DEFAULT"; return errorSuffix;
	default: errorSuffix = "UNKNOWN"; return errorSuffix;
	}
};

inline string loadKernelToString(string kernelFileName) {
	ifstream kernelFile(kernelFileName, ios::in);
	if (!kernelFile.is_open()) {
		cerr << endl;
		cerr << "Cannot open file. Make sure the that file " + kernelFileName << " is in the same location as the executable file\n";
		cerr << "Loading default kernel" << endl;
		return GAUSSIAN_KERNEL;
	}
	return string(istreambuf_iterator<char>(kernelFile), (istreambuf_iterator<char>()));
}

// http://insanecoding.blogspot.co.uk/2011/11/how-to-read-in-file-in-c.html
inline string get_file_contents(const char *filename)
{
	ifstream in(filename, std::ios::in);
	if (in)
	{
		string contents;
		in.seekg(0, ios::end);
		contents.resize(in.tellg());
		in.seekg(0, ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return contents;
	}
	cerr << endl << "No input image defined, please specify the path to a ppm image.";
	cerr << endl << "Use argument image:path to define input image";
	cerr << endl << "Example: sharpen.exe image:../../lena.ppm";
	cerr << endl << "Exiting application...\n";
	throw errno;
}

inline vector<unsigned char> readPPMAddAlpha(string fileName, unsigned int& width, unsigned int& height , unsigned int& max, string& magic) {
	vector<unsigned char> result;
	string str;
	try {
		 str = get_file_contents(fileName.c_str());
	} catch (const std::ios_base::failure& e) {
		cerr << "Could not open " << fileName << endl;
		cerr << "Make sure the image is in the same location as the executable file, or that the image path is correct" << endl;
		cerr << e.what();
		exit(-1);
	}
	stringstream ss(str);
	ss >> magic >> width >> height >> max;
	assert(max <= UCHAR_MAX);
	result.reserve(width*height*4);
	unsigned u;
	unsigned counter = 0;
	while (ss >> u) {
		result.push_back(u); // Yes, pushing an uint into a vector of uchars
		if (++counter % 3 == 0) {
			result.push_back(max);
		}
	}
	return result;
}

inline void savePPMSkippingAlpha(string fileName, const vector<unsigned char> &data, unsigned int& width, unsigned int& height, unsigned int& max, string& magic ) {
	string str;
	str.reserve(width*height*3);
	std::stringstream ss(str);
	ss << magic << '\n' << width << ' ' << height << '\n' << max << '\n';
	unsigned skipper = 0;
	for (const unsigned col : data) { // Yes, reading a uchar as a uint
		if (++skipper % 4) {
			ss << col << ' ';
		}
	}

	std::ofstream out(fileName, std::ios::out);
	out << ss.rdbuf();
	out.close();
}

inline Program CreateProgram(string &kernelSource, cl_device_type deviceType, bool useAverageKernel, string whatToSave = string("sharp"))
{
	cl_int status = CL_SUCCESS;

	vector<Platform> platformList;
	onError(Platform::get(&platformList), "Could not get any platform", QUIT);
	auto platform = platformList.front();

	vector<Device> devices;
	onError(platform.getDevices(deviceType, &devices), "Could not find any OpenCL compatible " + textify(deviceType), QUIT);
	auto device = devices.front();

	auto src = useAverageKernel ? AVERAGE_KERNEL : GAUSSIAN_KERNEL;
	auto index = src.find_last_of(')');
	src = src.insert(index, whatToSave);
	kernelSource = src;

	Program::Sources sources(1, make_pair(src.c_str(), src.length() + 1));

	Context context(devices);
	Program program(context, sources);

	onError(program.build(), "Could not build program");

	// breakpoint here, check log for kernel errors

	auto log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
	if (log.find("error detected in the compilation") != string::npos) {
		cerr << log;
	}

	return program;
}

/*
template used to measure any method
based on original code from http://stackoverflow.com/questions/22387586/measuring-execution-time-of-a-function-in-c
*/
using namespace chrono;
struct Time { long long nano, millis, minimumNanos, maximumNanos, minimumMillis, maxmimumMillis; double seconds; };
template<typename M, typename... P>
Time averageMeasure(TimeUnit unit, bool verbose, unsigned int times, M method, P&&... parameters) {
	long long totalMillis = 0;
	long long minimumMillis = 999999999999;
	long long maximumMillis = 0;
	long long totalNanos = 0;
	long long minimumNanos = 99999999999999999;
	long long maximumNanos = 0;
	double totalSeconds = 0;
	auto samples = times;
	while (times--) {
		auto start = high_resolution_clock::now();
		method(forward<P>(parameters)...);
		auto end = high_resolution_clock::now();
		auto nanoRuntime = duration_cast<nanoseconds> (end - start).count();
		minimumNanos = nanoRuntime < minimumNanos ? nanoRuntime : minimumNanos;
		maximumNanos = nanoRuntime > maximumNanos ? nanoRuntime : maximumNanos;
		totalNanos += nanoRuntime;
		auto millisRuntime = duration_cast<milliseconds> (end - start).count();
		totalMillis += millisRuntime;
		minimumMillis = millisRuntime < minimumMillis ? millisRuntime : minimumMillis;
		maximumMillis = millisRuntime > maximumMillis ? millisRuntime : maximumMillis;
		totalSeconds += duration<double>(end - start).count();
	}
	auto millis = totalMillis / samples;
	auto nanos = totalNanos / samples;
	auto secs = totalSeconds / samples;
	if (verbose && samples != 1) {
		if (millis < 4) {
			cout << endl << "Average execution time was " << nanos << " nanoseconds";
			cout << endl << "Fastest execution time was " << minimumNanos << " nanoseconds";
			cout << endl << "Slowest execution time was " << maximumNanos << " nanoseconds" << endl;
			cout << endl;
		}
		else {
			cout << endl << "Average execution time was " << millis << " milliseconds";
			cout << endl << "Fastest execution time was " << minimumMillis << " milliseconds";
			cout << endl << "Slowest execution time was " << maximumMillis << " milliseconds" << endl;
			cout << endl;
		}
	}
	switch (unit)
	{
	case NANOSECOND:
		cout << nanos << " nanoseconds" << endl;
		break;
	case MICROSECOND:
		cout << nanos/1000.0 << " microseconds" << endl;
		break;
	case MILLISECOND:
		cout << millis << " milliseconds" << endl;
		break;
	case SECOND:
		cout << secs << " seconds" << endl;
		break;
	default:
		break;
	}
	
	return{ nanos, millis, minimumNanos, maximumNanos, minimumMillis, maximumMillis, secs };
}

// function to log time into a excel file format (comma separated values)
inline bool logStats(string statsFileName, const vector<Time> stats, unsigned int cycles, TimeUnit unit = NANOSECOND) {
	statsFileName.append(".csv");
	fstream outputStatsFile(statsFileName.c_str(), fstream::out);
	if (outputStatsFile.is_open()) {
		cout << endl << "Successfully opened the file " << statsFileName << " for writing ";
		outputStatsFile << statsFileName.substr(0, statsFileName.size() - 4) << "\n";
		outputStatsFile << "Cycles = " << cycles << ",Execution timings\n";
		outputStatsFile << "Radius, Minimum, Average, Maximum\n";
		switch (unit) {
			case NANOSECOND: default: {
				outputStatsFile << " ,nanoseconds,nanoseconds,nanoseconds\n";
				for (int radius = 1; radius != stats.size() + 1; radius++) {
					auto times = stats[radius - 1];
					outputStatsFile << radius << "," 
									<< times.minimumNanos << "," 
									<< times.nano << "," 
									<< times.maximumNanos << "\n";
				}
			}
			break;
			case MICROSECOND: {
				outputStatsFile << " ,microseconds,microseconds,microseconds\n";
				for (int radius = 1; radius != stats.size() + 1; radius++) {
					auto times = stats[radius - 1];
					outputStatsFile << radius << "," 
									<< times.minimumNanos / 1000.0 << "," 
									<< times.nano / 1000.0 << ","
									<< times.maximumNanos / 1000.0 << "\n";
				}
			}
			break;
			case MILLISECOND: {
				outputStatsFile << " ,milliseconds,milliseconds,milliseconds\n";
				for (int radius = 1; radius != stats.size() + 1; radius++) {
					auto times = stats[radius - 1];
					outputStatsFile << radius << "," 
									<< times.minimumMillis << "," 
									<< times.millis << "," 
									<< times.maxmimumMillis<< "\n";
				}
			} 
			break;
			case SECOND: {
				outputStatsFile << " ,seconds,seconds,seconds\n";
				for (int radius = 1; radius != stats.size() + 1; radius++) {
					auto times = stats[radius - 1];
					outputStatsFile << radius << "," 
									<< times.minimumMillis / 1000.0 << ","
									<< times.seconds << ","
									<< times.maxmimumMillis / 1000.0 << "\n";
				}
			}
			break;
		}

		outputStatsFile.close();
		return true;
	}
	else {
		cerr << endl << "Cannot open file " << statsFileName;
		cerr << endl << "Please make sure the file is not opened already in Excel.";
		exit(-1);
	}
};

// helper to parse arguments
inline string extractValue(string argument, string from) {
	auto offset = argument.size() + 1; 
	auto startIndex = from.find(argument);
	if (startIndex != string::npos) {
		from = from.substr(startIndex + offset);
		auto endIndex = from.find_first_of(' ');
		argument = from.substr(0, endIndex);
		return argument;
	}
	return "default";
}

// default standard settings
struct Settings {
	cl_device_type deviceType = CL_DEVICE_TYPE_GPU;
	string imageFileName = "lena.ppm";
	string outputFileName;
	Save what = SHARP;
	TimeUnit unit = SECOND;
	bool interactive = false;
	bool verbose = false;
	bool showHelp = false;
	bool saveToDisk = true;
	bool useAverageKernel = true;
	bool normalizedGaussianKernel = false;
	unsigned int radius = 1;
	unsigned int cycles = 1;
	unsigned int maxRadii = 0;
	float alfa = 1.5f;
	float beta = -0.5f;
	float gama = 0.0f;
	float sigma = 1.0f;
};

inline Settings parseArguments(int argc, char **argv) {	
	/* 
	Available arguments:
		<image:lena.ppm>				-- loads lena.ppm for processing
		<using:CPU> | <using:GPU>		-- sets device to GPU or CPU (parallel, not serial)
		<radius:5>						-- sets radius of gaussian grid to 5
		<cycles:5>						-- number of cycles to run the kernel when measuring
		<measure:10>					-- measure execution time for <cycles> times for each radius from 1 to 10
		<export:blur> | <export:delta> | <export:sharp>
						-- blur: exports image where each pixel is sum
						-- delta: exports image where each pixel is sharp pixel - original pixel
						-- sharp: export processed sharpened pixel (default)
		<kernel:average> | <kernel:gaussian> | <kernel:gaussianNormalized>
						-- average uses the average of neighbour pixels to calculate pixel value (default kernel)
						-- gaussian uses a grid to calculate pixel values (default in interactive mode)
						-- gaussianNormalized uses a normalized Gaussian grid
		<unit:second> | <unit:millisecond> | <unit:nanosecond> | <unit:microsecond>
						-- specifies the time unit to be displayed
						-- also shortcuts available: s = second; ms = millisecond; ns = nanosecond; us = microsecond
		<alpha:1.5>						-- runs kernel with alpha = +1.5f
		<beta:-2.0>						-- runs kernel with beta  = -2.0f
		<gamma:0.2>						-- runs kernel with gama  = +0.2f
		<sigma:1.0>						-- sets sigma of gaussian grid to 1.0
		<interactive>					-- use OpenGL editor
		<saveAs:output.ppm>				-- save to disk (if export is mentioned, automatically saving to disk)
		<verbose>						-- enable gaussian function logging to display the grid values, and other couts
	*/
	string arguments;
	Settings standard;
	if (argc >= 1) {
		for (int i = 1; i < argc; ++i) {
			arguments += argv[i];
			arguments += " ";
		}
	}
	else {
		return standard;
	}
		
	string temp = extractValue("image", arguments);
	standard.imageFileName = temp == "default" ? standard.imageFileName : temp;
	temp = extractValue("using", arguments);
	if (temp == "CPU" || temp == "cpu" || temp == "Cpu") {
		standard.deviceType = CL_DEVICE_TYPE_CPU;
	}
	else if (temp == "GPU" || temp == "gpu" || temp == "Gpu") {
		standard.deviceType = CL_DEVICE_TYPE_GPU;
	}
	temp = extractValue("radius", arguments);
	standard.radius = temp == "default" ? standard.radius : atoi(temp.c_str());
	temp = extractValue("cycles", arguments);
	standard.cycles = temp == "default" ? standard.cycles : atoi(temp.c_str());
	temp = extractValue("measure", arguments);
	standard.maxRadii = temp == "default" ? standard.maxRadii : atoi(temp.c_str());
	temp = extractValue("saveAs", arguments);
	standard.saveToDisk = temp == "default" ? standard.saveToDisk : true;
	if (standard.saveToDisk && temp != "default") {
		standard.outputFileName = temp;
	}
	temp = extractValue("export", arguments);
	if (temp == "blur") {
		standard.what = BLUR_ONLY;
		standard.saveToDisk = true;
	}
	else if (temp == "delta") {
		standard.what = SHARP_DELTA;
		standard.saveToDisk = true;
	}
	else if (temp == "sharp") {
		standard.what = SHARP;
		standard.saveToDisk = true;
	}
	else {
		standard.saveToDisk = false;
	}
	temp = extractValue("alpha", arguments);
	standard.alfa = temp == "default" ? standard.alfa : atof(temp.c_str());
	temp = extractValue("beta", arguments);
	standard.beta = temp == "default" ? standard.beta : atof(temp.c_str());
	temp = extractValue("gamma", arguments);
	standard.gama = temp == "default" ? standard.gama : atof(temp.c_str());
	temp = extractValue("sigma", arguments);
	standard.sigma = temp == "default" ? standard.sigma : atof(temp.c_str());
	temp = extractValue("interactive", arguments);
	standard.interactive = temp == "default" ? false : true;
	temp = extractValue("verbose", arguments);
	standard.verbose = temp == "default" ? false : true;
	temp = extractValue("help", arguments);
	standard.showHelp = temp == "default" ? false : true;
	temp = extractValue("kernel", arguments);
	if (temp == "default") {
		standard.useAverageKernel = true;
	}
	else {
		standard.useAverageKernel = false;
		if (temp == "gaussianNormalized" || temp == "gaussianNormalised") {
			standard.normalizedGaussianKernel = true;
		}
	}
	temp = extractValue("unit", arguments); 
	{
		if (temp == "microsecond" || temp == "us") {
			standard.unit = MICROSECOND;
		}
		else if (temp == "millisecond" || temp == "ms") {
			standard.unit = MILLISECOND;
		}
		else if (temp == "nanosecond" || temp == "ns") {
			standard.unit = NANOSECOND;
		}
		else {
			standard.unit = SECOND;
		}
	}
	return standard;
}
