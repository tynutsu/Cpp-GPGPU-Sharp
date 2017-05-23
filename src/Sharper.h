#ifndef SHARPER_H
#define SHARPER_H
#include "Util.h"

class Sharper
{
private:
	cl_int status = CL_SUCCESS;
	// program properties
	Program program;
	Context context;
	CommandQueue queue;

	// device properties
	Device device;
	cl_device_type deviceType = CL_DEVICE_TYPE_GPU;
		
	// kernel properties
	Kernel kernel;
	bool useAverageKernel = true;
	bool normalizedGaussianKernel = false;
	string kernelName;
	string kernelSource;
	
	// image properties
	string imageFileName = "lena.ppm";
	vector<unsigned char> imagePixels;
	unsigned int width, height, max;
	string magic;
	string whatToSave = "sharp";

	// device references
	Image2D deviceInputImage;
	Image2D deviceOutputImage;
	Sampler deviceSampler;
	Buffer deviceGrid;

	// sharp variables
	float* grid;
	unsigned int radius = 1;
	float alfa = 1.5f, beta = -0.5f, gama = 0.0f, sigma = 1.0f;

	// surface area properties
	cl::size_t<3> origin, region;

	bool verbose = false;

	GLuint texture;

	void Sharper::updateAverageKernel();
	void Sharper::updateGaussianKernel();

public:
	Sharper();
	Sharper(Settings settings);
	Sharper(Settings settings, cl_device_type newDeviceType, bool autoInit = true);
	~Sharper();
	void init();
	void setImageFile(string newImageFile)				{ imageFileName = newImageFile; }
	void setDeviceType(cl_device_type newDeviceType)	{ deviceType = newDeviceType; }
	void setAlfa(float newAlfa)							{ alfa = newAlfa; }
	void setBeta(float newBeta)							{ beta = newBeta; }
	void setGama(float newGama)							{ gama = newGama; }
	void setSigma(float newSigma)						{ sigma = newSigma; }
	void setRadius(unsigned int newRadius)				{ radius = newRadius; }
	void setImageFileName(string newImageFileName)		{ imageFileName = newImageFileName; }
	void setImage(vector <unsigned char> externalImage, unsigned int width, unsigned int height, unsigned int max, string magic);
	void setWhatToSave(Save what);
	void update(unsigned int radius, float alfa = 1.5f, float beta = -0.5f, float gama = 0.0f, float sigma = 1.0f);
	void update(bool withRun, unsigned int radius, float alfa = 1.5f, float beta = -0.5f, float gama = 0.0f, float sigma = 1.0f);
	void run();
	void saveToDisk();
	void saveToDisk(string fileName);
	auto getWidth() { return width; }
	auto getHeight() { return height; }
	GLuint getTexture();
};

#endif // !SHARPER_H