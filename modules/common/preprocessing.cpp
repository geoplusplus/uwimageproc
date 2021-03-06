/********************************************
 * FILE NAME: preprocessing.cpp             *
 * DESCRIPTION:                             *
 * VERSION:                                 *
 * AUTHORS: Victor García                   *
 * MODIFIED BY: José Cappelletto            *
 *              Fabio Morales               *
 ********************************************/

/*
	Based on original helper function from *mosaic* respository. Snapshot of

	commit 849b20407eebb5c2f5caac1f2389ab6a914259c1
	Author: victorygc <victorygarciac@gmail.com>
	Date:   Wed Jan 31 01:37:49 2018 -0400
	
	Intended to improve current histogram stretching implementation, from percentile based
	RGB channel stretch, to a more general channel-by-channel basis. This will also improve
	*uwimageproc* histretch module, as it will include the percentile based approach
	Currently being handled in a separate branch
*/

#include "preprocessing.h"

void getHistogram(cv::Mat *img, cv::Mat *dstHist){
	// We will require 256 bins
	int histSize[] = {256}; //from 0 to 255
	float range[] = { 0, 256 } ; //the upper boundary is exclusive
	const float* histRange[] = { range }; // const as we don't expect to modify the content, just in case
	bool uniform = true; bool accumulate = false; // flags args for the OpenCV calcHist function

	calcHist(img, 1, 0, cv::Mat(), *dstHist, 1, histSize, histRange, uniform, accumulate );
	//Now we have the resulting histogram stored in dstHist
}

/*
void printHistogram(int histogram[256], std::string filename, cv::Scalar color){
    // Finding the maximum value of the histogram. It will be used to scale the
    // histogram to fit the image.
    int max = 0, i;
    for(i=0; i<256; i++){
        if( histogram[i] > max ) max= histogram[i];
    }
    // Creating an image from the histogram.
    cv::Mat imgHist(1480,1580, CV_8UC3, cv::Scalar(255,255,255));
    cv::Point pt1, pt2;
    pt1.y = 1380;
    for(i=0; i<256; i++){
        pt1.x = 150 + 5*i + 1;
        pt2.x = 150 + 5*i + 3;
        pt2.y = 1380 - 1280 * histogram[i] / max;
        cv::rectangle(imgHist,pt1,pt2,color,CV_FILLED);
    }
    // y-axis labels
    cv::rectangle(imgHist,cv::Point(130,1400),cv::Point(1450,80),cvScalar(0,0,0),1);
    cv::putText(imgHist, std::to_string(max), cv::Point(10,100), cv::FONT_HERSHEY_PLAIN, 1.5, cvScalar(0,0,0), 2.0);
    cv::putText(imgHist, std::to_string(max*3/4), cv::Point(10,420), cv::FONT_HERSHEY_PLAIN, 1.5, cvScalar(0,0,0), 2.0);
    cv::putText(imgHist, std::to_string(max/2), cv::Point(10,740), cv::FONT_HERSHEY_PLAIN, 1.5, cvScalar(0,0,0), 2.0);
    cv::putText(imgHist, std::to_string(max/4), cv::Point(10,1060), cv::FONT_HERSHEY_PLAIN, 1.5, cvScalar(0,0,0), 2.0);
    cv::putText(imgHist, std::to_string(0), cv::Point(10,1380), cv::FONT_HERSHEY_PLAIN, 1.5, cvScalar(0,0,0), 2.0);
    // x-axis labels
    cv::putText(imgHist, std::to_string(0), cv::Point(152-7*1,1430), cv::FONT_HERSHEY_PLAIN, 1.5, cvScalar(0,0,0), 2.0);
    cv::putText(imgHist, std::to_string(63), cv::Point(467-7*2,1430), cv::FONT_HERSHEY_PLAIN, 1.5, cvScalar(0,0,0), 2.0);
    cv::putText(imgHist, std::to_string(127), cv::Point(787-7*3,1430), cv::FONT_HERSHEY_PLAIN, 1.5, cvScalar(0,0,0), 2.0);
    cv::putText(imgHist, std::to_string(191), cv::Point(1107-7*3,1430), cv::FONT_HERSHEY_PLAIN, 1.5, cvScalar(0,0,0), 2.0);
    cv::putText(imgHist, std::to_string(255), cv::Point(1427-7*3,1430), cv::FONT_HERSHEY_PLAIN, 1.5, cvScalar(0,0,0), 2.0);
    // Saving the image
    cv::imwrite(filename, imgHist);
}
// */


// Now it will operate in a single channel of the provided image. So, future implementations will require a function call per channel (still faster)
void imgChannelStretch(cv::Mat imgOriginal, cv::Mat imgStretched, int lowerPercentile, int higherPercentile){
    // Computing the histograms
    cv::Mat histogram;

    getHistogram(&imgOriginal, &histogram);
    // printHistogram(histogram, "inputCPU.jpg", 255);

    // Computing the percentiles. We force invalid values as initial values (just in case)
    float channelLowerPercentile = -1.0, channelHigherPercentile = -1.0;
    int height = imgOriginal.size().height;
    int width = imgOriginal.size().width;
    int i = 0;
    float sum = 0.0;
	float normImgSize = height * width / 100.0;

	while ( sum < higherPercentile * normImgSize ){
        if(sum < lowerPercentile * normImgSize) channelLowerPercentile++; 
        channelHigherPercentile++;
        sum += histogram.at<float>(i,0);
        i++;
    }

    float b = - channelLowerPercentile;
    float m = 255.0 / ( channelHigherPercentile - channelLowerPercentile );

    imgStretched += b;
    imgStretched *= m;

    // getHistogram(imgStretched, histogram);
    // printHistogram(histogram, "outputCPU.jpg", 255);

}

#if USE_GPU
// Now it will operate in a single channel of the provided image. So, future implementations will require a function call per channel (still faster)
void imgChannelStretchGPU(cv::cuda::GpuMat imgOriginalGPU, cv::cuda::GpuMat imgStretchedGPU, int lowerPercentile, int higherPercentile){
    cv::Mat Original;
    imgOriginalGPU.download(Original);

    // Computing the histograms
    cv::Mat histogram;
    getHistogram(&Original, &histogram);
    // printHistogram(histogram, "inputGPU.jpg", 255);

    // Computing the percentiles. We force invalid values as initial values (just in case)
    float channelLowerPercentile = -1.0, channelHigherPercentile = -1.0;
    float height = Original.size().height;
    float width  = Original.size().width;
    int i = 0;
    float sum  = 0.0;

	float normImgSize = height * width / 100.0;

	while ( sum < higherPercentile * normImgSize ){
        if(sum < lowerPercentile * normImgSize) channelLowerPercentile++; 
        channelHigherPercentile++;
        sum += histogram.at<float>(i,0);
        i++;
    }

    float m  = 255.0 / ( channelHigherPercentile - channelLowerPercentile );
    float b  = - channelLowerPercentile;

    cv::cuda::add(imgStretchedGPU, b, imgStretchedGPU);
    cv::cuda::multiply(imgStretchedGPU, m, imgStretchedGPU);

    imgStretchedGPU.download(Original);
    // getHistogram(Original, histogram);
    // printHistogram(histogram, "outputGPU.jpg", 255);

}
#endif

int numChannel(char c){
    if(c == 'R' || c == 'H' || c == 'h' || c == 'L' || c == 'Y' ) return 0;  
    if(c == 'G' || c == 'S' || c == 's' || c == 'a' || c == 'C' ) return 1;   
    if(c == 'B' || c == 'V' || c == 'l' || c == 'b' || c == 'X' ) return 2;
    return -1;  
}

int numSpace(char c){
    if(c == 'R' || c == 'G' || c == 'B' ) return 0;  
    if(c == 'H' || c == 'S' || c == 'V' ) return 1;    
    if(c == 'h' || c == 's' || c == 'l' ) return 2;
    if(c == 'L' || c == 'a' || c == 'b' ) return 3;  
    if(c == 'Y' || c == 'C' || c == 'X' ) return 4;
    return -1;
}
