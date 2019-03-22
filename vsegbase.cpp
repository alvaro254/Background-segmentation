#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <ctype.h>
#include <cmath>
#include <tclap/CmdLine.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>

using namespace std;
using namespace cv;

/*
  Use CMake to compile
*/

int T;
int slider;

//Change the threshold value when the trackbar is moved
void on_trackbar(int, void*);
//Check if the input file is a string or a number
bool isNumber(const char *filein);

int main (int argc, char * const argv[])
{
	/* Default values */
	bool cameraInput=false;
	float threshold;
	int radius;
	const char * filein = 0;
	const char * fileout = 0;
	char out_image[50];

	TCLAP::CmdLine cmd("Video segmentation", ' ', "0.1");

	TCLAP::ValueArg<string> filename("v", "videoname", "Video path", true, "video.avi", "string");
	cmd.add(filename);
	TCLAP::ValueArg<string> outname("o", "outfilename", "Output video path", true, "out.avi", "string");
	cmd.add(outname);
	TCLAP::ValueArg<int> thres("t", "threshold", "Threshold value", false, 13, "float");
	cmd.add(thres);
	TCLAP::ValueArg<int> size("s", "radius", "Radiues value", false, 0, "int");
	cmd.add(size);

	// Parse input arguments
	cmd.parse(argc, argv);

	filein = filename.getValue().c_str();
	fileout = outname.getValue().c_str();
	threshold = thres.getValue();
	radius = size.getValue();

	//Set the global variables 
	T = threshold;
	slider = threshold;

	cout << "Input stream:" << filein << endl;
	cout << "Output:" << fileout << endl;

	//Check if the input stream is a video or a video capture device
	if(isNumber(filein)) cameraInput = true;	

	//Create a trackbar to set the Threshold
	namedWindow("Threshold");
	createTrackbar("Threshold","Threshold", &slider, 255, on_trackbar);

	//Open the input stream
	VideoCapture input;

	if (cameraInput)
		input.open(atoi(filein));
	else
		input.open(filein);

	if (!input.isOpened())
	{
		cerr << "Error: the input stream is not opened.\n";
		return 0;
	}

	Mat inFrame, inFrame2, Segmented_mask, Segmented_mask3C, foreground, splited[3], result, kernel, opening, closing;
	Mat outFrame = Mat::zeros(inFrame.size(), CV_8UC1);
	vector<Mat> channels;

	//Read the first frame
	bool wasOk = input.read(inFrame);
	if (!wasOk)
	{
		cerr << "Error: could not read any image from stream.\n";
		return 0;
	}

	double fps = 25.0;
	if (!cameraInput)
		fps=input.get(CV_CAP_PROP_FPS);

	//Open the output stream
	VideoWriter output;
	output.open(fileout, input.get(CV_CAP_PROP_FOURCC), fps, inFrame.size(), true);
	if (!output.isOpened())
	{
		cerr << "Error: the ouput stream is not opened.\n";
	}

	int frameNumber=0;
	int key = 0;

	namedWindow("Output");

	while(wasOk && key!=27)
	{
		frameNumber++;

		//Read the next frame
		wasOk = input.read(inFrame2);

		//Check if the frame has been read successfully
		if (!wasOk)

			break;

		//Subtract one frame to the previous one
		absdiff(inFrame2, inFrame, outFrame);

		//Generate a segmented mask
		Segmented_mask = outFrame > T;

		//Create a structuring element of (2*radius+1)X(2*radius+1) 
		kernel = getStructuringElement(MORPH_RECT, Size(2*radius + 1, 2*radius + 1), Point(radius, radius));
		//Apply the opening operation to the segmented mask
		morphologyEx(Segmented_mask, closing, MORPH_CLOSE, kernel);
		//Apply the closing operation to the segmented mask
		morphologyEx(Segmented_mask, opening, MORPH_OPEN, kernel);

		//opening + closing
		Segmented_mask = opening + closing;
		
		//Split the segmented mask in 3 channels
		split(Segmented_mask, splited);
		//Channel0 AND channel1
		bitwise_and(splited[0], splited[1], result);
		//(Channel0 AND channel1) AND channel2
		bitwise_and(result, splited[2], result);

		//Multiply by 255 to make the image brighter
		result *= 255;

		channels.push_back(result);
		channels.push_back(result);
		channels.push_back(result);

		//Adds 2 channels to the segmented mask to have a 3 channels segmented mask
		merge(channels, Segmented_mask3C);

		//Apply the segmeted mask of 3 channels to get the detected object
		foreground = inFrame2&Segmented_mask3C;

		//Write the segmented mask to the video
		output.write(Segmented_mask3C);

		//Show the detection of the foreground
		imshow ("Threshold", Segmented_mask3C);
		//Show the detected object
		imshow("Output", foreground);
		     
		//Read another frame
		wasOk=input.read(inFrame);

		if (!wasOk)

			break;
		
		key = waitKey(20);

		//If the spacebar is pressed, save the current frame
		if(key == 32){

			sprintf(out_image, "out_%d.jpg", frameNumber);
			imwrite(out_image, foreground);
		}

		//Clear the vector
		channels.clear();
	}

	//Close the streams
	input.release();
	output.release();

	return 0;
}

void on_trackbar(int, void *)
{
	//Update the value of the threshold
	T = slider;
}

bool isNumber(const char *filein)
{
	for(int i = 0; i < strlen(filein); i++){

		if(!isdigit(filein[i])){

			return false;
		}
	}

	return true;
}
