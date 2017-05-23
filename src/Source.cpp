#include "Source.h"

Sharper sharp;
Settings setup;
float alfa, beta, gama, delta = 0.01f, sigma;
unsigned int radius, windowID;
bool valuesChanged;

int main(int argc, char **argv)
{
	setup = parseArguments(argc, argv);
	
	if (setup.showHelp) {
		printUsage();					// show help if required
	}
	
	if (setup.interactive) {
		runGL(argc, argv);				// take the interactive path
	}
	else {
		if (setup.maxRadii) {
			runBenchmark();				// measure execution times
		}
		else {
			runCL();					// run average filter on gpu
		}
	}
	if (setup.verbose) {
		cout << endl << "Finished.";
	}
	return 0;
}

void GLrender()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix(); {
		glLoadIdentity();
		glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix(); {
			glLoadIdentity();
			glDisable(GL_LIGHTING);
			glColor3f(1.0f, 1.0f, 1.0f);

			glEnable(GL_TEXTURE_2D); {
				// bind texture
				glBindTexture(GL_TEXTURE_2D, sharp.getTexture());
				// draw textured quad
				glBegin(GL_QUADS); {
					glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
					glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
					glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
					glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
				}
				glEnd();
			}
			glDisable(GL_TEXTURE_2D);

		}
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
	}
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glutSwapBuffers();
}

void runGL(int argc, char **argv) {
	setup.useAverageKernel = false;
	sharp = Sharper(setup, CL_DEVICE_TYPE_GPU);
	alfa = setup.alfa;
	beta = setup.beta;
	gama = setup.gama;
	radius = setup.radius;
	sigma = setup.sigma;
	printHelp();
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(sharp.getWidth(), sharp.getHeight());
	windowID = glutCreateWindow("Sharp");
	glutKeyboardFunc(press);
	glutKeyboardUpFunc(release);
	glutDisplayFunc(GLrender);
	glutMainLoop();
}

void runCL(void)
{
	sharp = Sharper(setup, setup.deviceType);
	averageMeasure(setup.unit, true, setup.cycles, execute, sharp);
	if (setup.outputFileName.size()) {
		sharp.saveToDisk(setup.outputFileName);
	}
	else {
		sharp.saveToDisk();
	}
}

void runBenchmark() {
	auto verbose = setup.verbose;
	unsigned int localWidth, localHeight, localMax;
	string localMagic;
	cout << endl << "Reading image...";
	auto imageToMeasure = readPPMAddAlpha(setup.imageFileName, localWidth, localHeight, localMax, localMagic);
	cout << "Done!" << endl;
	vector<Time> gpuTimes(setup.maxRadii), cpuTimes(setup.maxRadii);
	Sharper gpu = Sharper(setup, CL_DEVICE_TYPE_GPU, false);
	if (verbose) {
		cout << endl << "Setting externally loaded image for GPU";
	}
	gpu.setImage(imageToMeasure, localWidth, localHeight, localMax, localMagic);
	gpu.init();
	Sharper cpu = Sharper(setup, CL_DEVICE_TYPE_CPU, false);
	if (verbose) {
		cout << endl << "Setting externally loaded image for CPU";
	}
	cpu.setImage(imageToMeasure, localWidth, localHeight, localMax, localMagic);
	cpu.init();
	cout << endl << "Starting benchmark: " << endl;
	for (int radius = 1; radius != setup.maxRadii + 1; radius++) {
		if (verbose) {
			cout << endl << endl << "Radius: " << radius << endl;
			cout << "GPU: ";
		}
		gpu.update(radius);
		gpuTimes[radius - 1] = averageMeasure(setup.unit, setup.verbose, setup.cycles, execute, gpu);
		if (verbose) {
			cout << "CPU: ";
		}
		cpu.update(radius);
		cpuTimes[radius - 1] = averageMeasure(setup.unit, setup.verbose, setup.cycles, execute, cpu);
	}
	cout << "Done benchmarking!" << endl;
	logStats("CPU-times", cpuTimes, setup.cycles, setup.unit);
	logStats("GPU-times", gpuTimes, setup.cycles, setup.unit);

	if (setup.saveToDisk) {
		cpu.saveToDisk();
		gpu.saveToDisk();
		cout << endl << "Saved images to disk";
	}
}

void press(unsigned char key, int x, int y)
{
	valuesChanged = true;
	switch (key)
	{
	case '1':
		alfa += delta;
		break;
	case '2':
		alfa -= delta;
		break;
	case '3':
		beta += delta;
		break;
	case '4':
		beta -= delta;
		break;
	case '5':
		gama += delta;
		break;
	case '6':
		gama -= delta;
		break;
	case '7':
		sigma += delta;
		break;
	case '8':
		sigma -= delta;
		break;
	case '*':
		delta *= 2;
		break;
	case '/':
		delta /= 2;
		break;
	case '+':
		delta += 0.1f;
		break;
	case '-':
		delta -= 0.1f;
		break;
	case 27:
		glutDestroyWindow(windowID);
		system("cls");
		cout << "Bye!" << endl;
		exit(0);
	default:
		valuesChanged = false;
		break;
	}
	if (valuesChanged) {
		sharp.update(true, radius, alfa, beta, gama, sigma);
		printReset();
	}
	glutPostRedisplay();
}

void release(unsigned char key, int x, int y)
{
	valuesChanged = true;
	switch (key)
	{
	case '9':
		radius++;
		break;
	case '0':
		if (radius > 1) {
			radius--;
		}
		break;
	case 'a' : case 'A':
		alfa = setup.alfa;
		break;
	case 'b': case 'B':
		beta = setup.beta;
		break;
	case 'g': case 'G':
		gama = setup.gama;
		break;
	case 's': case 'S':
		sigma = setup.sigma;
		break;
	case 'r': case 'R':
		radius = setup.radius;
		break;
	case 'd': case 'D':
		delta = 0.01f;
		break;
	case 'p': case 'P':
		sharp.saveToDisk("InteractiveEdit.ppm");
		valuesChanged = false;
		break;
	case 'h': case 'H':
		printHelp();
		valuesChanged = false;
		break;
	default:
		valuesChanged = false;
		break;
	}
	if (valuesChanged) {
		sharp.update(true, radius, alfa, beta, gama, sigma);
		printReset();
	}
	glutPostRedisplay();
}

void printHelp() {
	system("cls");
	cout << "=======================================================================================";
	cout << endl;
	cout << endl << "============================= Welcome to interactive mode =============================";
	cout << endl << "Use keys 1,2,3,4,5,6,7,8,9,0 to modify varius values";
	cout << endl << "Looks difficult, right?";
	cout << endl << "Let's simplify: Odds increase, Evens decrease";
	cout << endl << "The amount to increase or decrease (DELTA) can be changed using +, -, / or *";
	cout << endl << "And the order of values is alfabetical (almost):";
	cout << endl << "=======================================================================================";
	cout << endl << "\t\tFor ALPHA\t use 1 (+DELTA), \t2 (-DELTA)";
	cout << endl << "\t\tFor BETA\t use 3 (+DELTA), \t4 (-DELTA)";
	cout << endl << "\t\tFor GAMA\t use 5 (+DELTA), \t6 (-DELTA)";
	cout << endl << "\t\tFor SIGMA\t use 7 (+DELTA), \t8 (-DELTA)";
	cout << endl << "\t\tFor Radius\t use 9 (+DELTA), \t0 (-DELTA)";
	cout << endl << "=======================================================================================";
	cout << endl << "Right so you know how to change the values, let's see how you revert them to defaults";
	cout << endl << "\t\tALPHA\tBETA\tGAMMA\tDELTA\tSIGMA\tRADIUS";
	cout << endl << "\t\t  A  \t B  \t  G  \t  D  \t  S  \t  R";
	printf("\n\t\t %04.2f\t%04.2f\t%04.2f\t%04.2f\t%04.2f\t  %i", alfa, beta, gama, delta, sigma, radius);
	cout << endl << "=======================================================================================";
	cout << endl << "An extra key if you want to save to file the current state of the image, PRINT it with P";
	cout << endl << "And H of course is for help, and shows this screen with the keys usage";
	cout << endl << "To exit, of course... ESC";
	cout << endl << "Enjoy!";
	cout << endl << "=======================================================================================";
	cout << endl << "Oh! Almost forgot... the screen is dark until you press a modifier key";
	cout << endl << "That's because we only update the screen when a value has changed... efficient, eh?";
	cout << endl << "=======================================================================================";
}

void printReset() {
	system("cls");
	cout << endl << "=======================================================================================";
	cout << endl << "\t\tALPHA\tBETA\tGAMMA\tDELTA\tSIGMA\tRADIUS";
	printf("\n\t\t %04.2f\t%04.2f\t%04.2f\t%04.2f\t%04.2f\t  %i", alfa, beta, gama, delta, sigma, radius);
	cout << endl << "=======================================================================================";
}