#include "Sharper.h"

void Sharper::init()
{
	program = CreateProgram(kernelSource, deviceType, useAverageKernel, whatToSave);
	context = program.getInfo<CL_PROGRAM_CONTEXT>();
	device = context.getInfo<CL_CONTEXT_DEVICES>().front();
	queue = CommandQueue(context, device);
	kernelName = getKernelNameFrom(kernelSource);
	
	if (imagePixels.size() == 0) {
		if (verbose) {
			cout << endl << "Reading image...";
		}
		imagePixels = readPPMAddAlpha(imageFileName, width, height, max, magic);
	}
	if (verbose) {
		cout << endl << "Loaded image successfully";
	}
	deviceInputImage = Image2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, IMAGE_FORMAT, width, height, 0, imagePixels.data(), &status);
	onError(status, "Could not create Image2D for device input", QUIT);
	deviceOutputImage = Image2D(context, CL_MEM_WRITE_ONLY, IMAGE_FORMAT, width, height, 0, nullptr, &status);
	onError(status, "Could not create Image2d for device output", QUIT);
	deviceSampler = Sampler(context, CL_FALSE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST);
	onError(status, "Could not create Sampler for device sampler", QUIT);

	origin[0] = 0;		origin[1] = 0;		origin[2] = 0;
	region[0] = width;  region[1] = height; region[2] = 1;

	kernel = Kernel(program, kernelName.c_str(), &status);
	
	update(radius, alfa, beta, gama, sigma);
	if (verbose) {
		cout << endl << "Created OpenCL kernel successfully";
	}
	glGenTextures(1, &texture);
}

Sharper::Sharper()
{
}

Sharper::Sharper(Settings settings)
{
	Sharper(settings, settings.deviceType);
}

Sharper::Sharper(Settings settings, cl_device_type newDeviceType, bool autoInit)
{
	imageFileName = settings.imageFileName;
	setWhatToSave(settings.what);
	deviceType = newDeviceType;
	radius = settings.radius;
	alfa = settings.alfa;
	beta = settings.beta;
	gama = settings.gama;
	sigma = settings.sigma;
	verbose = settings.verbose;
	useAverageKernel = settings.useAverageKernel;
	normalizedGaussianKernel = settings.normalizedGaussianKernel;
	if (autoInit) {
		init();
	}
	
}

Sharper::~Sharper()
{
}

void Sharper::setImage(vector<unsigned char> externalImage, unsigned int width, unsigned int height, unsigned int max, string magic)
{
	imagePixels = externalImage;
	this->width = width;
	this->height = height;
	this->max = max;
	this->magic = magic;
}

void Sharper::setWhatToSave(Save what)
{
	switch (what)
	{
	case BLUR_ONLY: whatToSave = "blur";
		break;
	case SHARP_DELTA: whatToSave = "delta";
		break;
	case SHARP: whatToSave = "sharp";
		break;
	default:
		break;
	}
}

void Sharper::updateGaussianKernel() {
	grid = getGaussianFilter(verbose, normalizedGaussianKernel, radius, sigma);
	unsigned int sizeOfGrid = (radius * 2 + 1) * (radius * 2 + 1) * sizeof(float);
	deviceGrid = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeOfGrid, grid, &status);
	onError(status, "Could not create Buffer for device grid", QUIT);

	onError(kernel.setArg(0, deviceInputImage), "Could not set argument on kernel for variable input", QUIT);
	onError(kernel.setArg(1, deviceOutputImage), "Could not set argument on kernel for variable output", QUIT);
	onError(kernel.setArg(2, deviceSampler), "Could not set argument on kernel for variable sampler", QUIT);
	onError(kernel.setArg(3, deviceGrid), "Could not set argument on kernel for variable mask ", QUIT);
	onError(kernel.setArg(4, sizeof(int), &radius), "Could not set the argument on kernel for variable radius", QUIT);
	onError(kernel.setArg(5, sizeof(float), &alfa), "Could not set argument on kernel for variable alfa", QUIT);
	onError(kernel.setArg(6, sizeof(float), &beta), "Could not set argument on kernel for variable beta", QUIT);
	onError(kernel.setArg(7, sizeof(float), &gama), "Could not set argument on kernel for variable gama", QUIT);
}

void Sharper::updateAverageKernel() {
	onError(kernel.setArg(0, deviceInputImage), "Could not set argument on kernel for variable input", QUIT);
	onError(kernel.setArg(1, deviceOutputImage), "Could not set argument on kernel for variable output", QUIT);
	onError(kernel.setArg(2, deviceSampler), "Could not set argument on kernel for variable sampler", QUIT);
	onError(kernel.setArg(3, sizeof(int), &radius), "Could not set the argument on kernel for variable radius", QUIT);
	onError(kernel.setArg(4, sizeof(float), &alfa), "Could not set argument on kernel for variable alfa", QUIT);
	onError(kernel.setArg(5, sizeof(float), &beta), "Could not set argument on kernel for variable beta", QUIT);
	onError(kernel.setArg(6, sizeof(float), &gama), "Could not set argument on kernel for variable gama", QUIT);
}

void Sharper::update(unsigned int r, float a, float b, float g, float s)
{
	radius = r;
	alfa = a;
	beta = b;
	gama = g;
	sigma = s;
	useAverageKernel ? updateAverageKernel() : updateGaussianKernel();
}

void Sharper::update(bool withRun, unsigned int radius, float alfa, float beta, float gama, float sigma)
{
	update(radius, alfa, beta, gama, sigma);
	if (withRun) {
		run();
	}
}

void Sharper::run()
{
	queue.enqueueNDRangeKernel(kernel, NullRange, NDRange(width, height));
	queue.finish();
}

void Sharper::saveToDisk()
{
	string suffix = "-";
	suffix.append(textify(deviceType));
	saveToDisk(whatToSave + suffix + ".ppm");
}

void Sharper::saveToDisk(string fileName)
{
	if (verbose) {
		cout << endl << "Saving image...";
	}
	queue.enqueueReadImage(deviceOutputImage, CL_BLOCKING, origin, region, 0, 0, imagePixels.data());
	savePPMSkippingAlpha(fileName, imagePixels, width, height, max, magic);
	cout << endl << "Saved as: " << fileName;
}

GLuint Sharper::getTexture() 
{
	queue.enqueueReadImage(deviceOutputImage, CL_BLOCKING, origin, region, 0, 0, imagePixels.data());
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imagePixels.data());

	return texture;
}

