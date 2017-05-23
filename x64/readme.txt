Arguments can be added in any order; the argument image:lena.ppm can be skipped if image is in the same location with exe
"..." means any other arguments
===========================================================================================
All variables:			
	image:			add image path after colon
	radius:			add radius after colon
	alpha:			add alpha after colon
	beta:			add beta after colon
	gama:			add gama after colon
	sigma:			add sigma after colon
	measure:		add upper limit of interval of radii to benchmark, from 1 to value added. For each radius, the program will run cycles times
	cycles:			add number of times to run a program

All options:			ACCEPTED OPTIONS
	unit:			second, s, millisecond, ms, microsecond, us, nanosecond, ns
	kernel:			average, gaussian, gaussianNormalized, gaussianNormalised
	export:			blur, delta, sharp
	using:			CPU, Cpu, cpu, GPU, Gpu, gpu

All flags:
	help			enables help
	verbose			enables logging
	interactive		enables interactive mode (GL editor)
===========================================================================================

Examples explained:

===========================================================================================
	For help:
sharpen.exe help

	To run:
sharpen.exe image:lena.ppm radius:5

	To run with GL:
sharpen.exe interactive

	To generate timings add measure argument and specify how many cycles will run. the value from measure specifies how many radii to measure (from 1 to 5)
sharpen.exe image:lena.ppm measure:5 cycles:10

	To show debug info add verbose argument:
sharpen.exe ... verbose ...
sharpen.exe image.lena verbose radius:2

	To export certain mask add argument explore:blur or explore:delta or explore:sharp (default)
sharpen.exe ... export:blur  ...		-- saves blur image
sharpen.exe ... export:delta ...		-- saves delta mask (original image - blur)
sharpen.exe ... export:sharp ...		-- default behaviour, saves sharp image
sharpen.exe image:../../../ghost.ppm export:blur verbose

	To run with specific values for alpha, beta, gamma, radius, sigma just add them as argument in format variable:value
sharpen.exe radius:3 alpha:-2.3 beta:1.5 gama:233
sharpen.exe beta:1.3

	To run on certain device type add argument using:GPU or using:CPU
sharpen.exe ... using:CPU ...
sharpen.exe image:ghost.ppm unit:second using:cpu radius:23 verbose
sharpen.exe image:lena.ppm radius:5 beta:2.1 using:CPU export:sharp

	To save output under a specific filename add saveAs:<path> and the path or file name
sharpen.exe ... saveAs:gpgpu.ppm ... 
sharpen.exe image:../../reduced.ppm saveAs:CPUcustomOutput.ppm using:CPU export:delta

	To specify time unit use unit:second or unit:s ; unit:millisecond or unit:ms ; unit:microsecond or unit:us ; unit:nanosecond or unit:ns
sharpen.exe image:../toMeasure.ppm measure:20 cycles:30 unit:microsecond

	To specify what kernel to use add argument kernel:gaussian or kernel:gaussianNormalised or kernelAverage. 
sharpen.exe radius:5 kernel:gaussian verbose
sharpen.exe image:lena.ppm kernel:gaussian verbose sigma:233 
sharpen.exe image:lena.ppm interactive kernel:gaussianNormalised 



