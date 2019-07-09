#include <numeric>
#include "matching2D.hpp"

using namespace std;

// Find best matches for keypoints in two camera images based on several matching methods
void matchDescriptors(std::vector<cv::KeyPoint> &kPtsSource, std::vector<cv::KeyPoint> &kPtsRef, cv::Mat &descSource, cv::Mat &descRef,
                      std::vector<cv::DMatch> &matches, std::string descriptorType, std::string matcherType, std::string selectorType)
{
    // configure matcher
    bool crossCheck = false;
    cv::Ptr<cv::DescriptorMatcher> matcher;

    if (matcherType.compare("MAT_BF") == 0)
    {
        int normType = cv::NORM_HAMMING;
        matcher = cv::BFMatcher::create(normType, crossCheck);
    }
    else if (matcherType.compare("MAT_FLANN") == 0)
    {
        // ...
    }

    // perform matching task
    if (selectorType.compare("SEL_NN") == 0)
    { // nearest neighbor (best match)

        matcher->match(descSource, descRef, matches); // Finds the best match for each descriptor in desc1
    }
    else if (selectorType.compare("SEL_KNN") == 0)
    { // k nearest neighbors (k=2)

        // ...
    }
}

// Use one of several types of state-of-art descriptors to uniquely identify keypoints
void descKeypoints(vector<cv::KeyPoint> &keypoints, cv::Mat &img, cv::Mat &descriptors, string descriptorType)
{
    // select appropriate descriptor
    cv::Ptr<cv::DescriptorExtractor> extractor;
    if (descriptorType.compare("BRISK") == 0)
    {

        int threshold = 30;        // FAST/AGAST detection threshold score.
        int octaves = 3;           // detection octaves (use 0 to do single scale)
        float patternScale = 1.0f; // apply this scale to the pattern used for sampling the neighbourhood of a keypoint.

        extractor = cv::BRISK::create(threshold, octaves, patternScale);
    }
    else
    {

        //...
    }

    // perform feature description
    double t = (double)cv::getTickCount();
    extractor->compute(img, keypoints, descriptors);
    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    cout << descriptorType << " descriptor extraction in " << 1000 * t / 1.0 << " ms" << endl;
}

// Detect keypoints in image using the traditional Shi-Thomasi detector
void detKeypointsShiTomasi(vector<cv::KeyPoint> &keypoints, cv::Mat &img, bool bVis)
{
    // compute detector parameters based on image size
    int blockSize = 4;       //  size of an average block for computing a derivative covariation matrix over each pixel neighborhood
    double maxOverlap = 0.0; // max. permissible overlap between two features in %
    double minDistance = (1.0 - maxOverlap) * blockSize;
    int maxCorners = img.rows * img.cols / max(1.0, minDistance); // max. num. of keypoints

    double qualityLevel = 0.01; // minimal accepted quality of image corners
    double k = 0.04;

    // Apply corner detection
    double t = (double)cv::getTickCount();
    vector<cv::Point2f> corners;
    cv::goodFeaturesToTrack(img, corners, maxCorners, qualityLevel, minDistance, cv::Mat(), blockSize, false, k);

    // add corners to result vector
    for (auto it = corners.begin(); it != corners.end(); ++it)
    {

        cv::KeyPoint newKeyPoint;
        newKeyPoint.pt = cv::Point2f((*it).x, (*it).y);
        newKeyPoint.size = blockSize;
        keypoints.push_back(newKeyPoint);
    }
    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    cout << "Shi-Tomasi detection with n=" << keypoints.size() << " keypoints in " << 1000 * t / 1.0 << " ms" << endl;

    // visualize results
    if (bVis)
    {
        cv::Mat visImage = img.clone();
        cv::drawKeypoints(img, keypoints, visImage, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        string windowName = "Shi-Tomasi Corner Detector Results";
        cv::namedWindow(windowName, 6);
        imshow(windowName, visImage);
        cv::waitKey(0);
    }
}

void detKeypointsHarris(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img, bool bVis)
{
    double t = (double)cv::getTickCount();

    const int blockSize = 4;
    const int apertureSize = 3;
    const double k = 0.04;
    cv::Mat dst = cv::Mat::zeros(img.size(), CV_32FC1);
    cornerHarris(img, dst, blockSize, apertureSize, k);
    cv::Mat dst_norm, dst_norm_scaled;
    normalize(dst, dst_norm, 0, 255, cv::NORM_MINMAX, CV_32FC1, cv::Mat());
    convertScaleAbs(dst_norm, dst_norm_scaled);

    for (size_t r = 0; r < dst_norm.rows; r++)
    {
        for (size_t c = 0; c < dst_norm.cols; c++)
        {
            int response = (int)dst_norm.at<float>(r, c);
            const double minResponse = 100;
            if (response <= minResponse)
                continue;
            cv::KeyPoint kp;
            kp.pt = cv::Point2f(c, r);
            kp.size = 2 * apertureSize;
            kp.response = response;
            bool overlap = false;
            const double maxOverlap = 0;
            for (auto it = keypoints.begin(); it != keypoints.end(); ++it)
            {
                double kpOverlap = cv::KeyPoint::overlap(kp, *it);
                if (kpOverlap > maxOverlap)
                {
                    overlap = true;
                    if (kp.response > it->response)
                    {
                        *it = kp;
                        break;
                    }
                }
            }
            if (!overlap)
                keypoints.push_back(kp);
        }
    }

    cout << "Harris detection with n=" << keypoints.size() << " keypoints in " << 1000 * t / 1.0 << " ms" << endl;

    if (bVis)
    {
        string windowName = "Harris detector results";
        cv::namedWindow(windowName);
        cv::imshow(windowName, dst_norm_scaled);
        cv::waitKey(0);
    }
}

void detKeypointsModern(std::vector<cv::KeyPoint> &keypoints, cv::Mat &img, std::string detectorType, bool bVis)
{
    double t = (double)cv::getTickCount();
    
    if (detectorType.compare("FAST") == 0)
    {
        int threshold = 30; // difference between intensity of the central pixel and pixels of a circle around this pixel
        bool bNMS = true; // perform non-maxima suppression on keypoints
        cv::FastFeatureDetector::DetectorType type = cv::FastFeatureDetector::TYPE_9_16; // TYPE_9_16, TYPE_7_12, TYPE_5_8
        cv::Ptr<cv::FeatureDetector> detector = cv::FastFeatureDetector::create(threshold, bNMS, type);
        detector->detect(img, keypoints);
    }
    else if (detectorType.compare("BRISK") == 0)
    {
        int thresh = 30;
        int octaves = 3;
        float patternScale = 1.0;
        cv::Ptr<cv::BRISK> brisk = cv::BRISK::create(thresh, octaves, patternScale);
        brisk->detect(img, keypoints);
    }
    else if (detectorType.compare("ORB") == 0) 
    {
        int nfeatures = 500;
        float scaleFactor = 1.2f;
        int nlevels = 8;
        int edgeThreshold = 31;
        int firstLevel = 0;
        int WTA_K = 2;
        cv::ORB::ScoreType scoreType = cv::ORB::HARRIS_SCORE;
        int patchSize = 31;
        int fastThreshold = 20;
        cv::Ptr<cv::ORB> orb = cv::ORB::create(nfeatures, scaleFactor, nlevels, edgeThreshold, firstLevel,
            WTA_K, scoreType, patchSize, fastThreshold);
        orb->detect(img, keypoints);
    }
    else if (detectorType.compare("AKAZE") == 0) 
    {
        cv::AKAZE::DescriptorType descriptor_type = cv::AKAZE::DESCRIPTOR_MLDB;
        int descriptor_size = 0;
        int descriptor_channels = 1;
        float threshold = 0.001f;
        int nOctaves = 4;
        int nOctaveLayers = 4;
        cv::KAZE::DiffusivityType diffusivity = cv::KAZE::DIFF_PM_G2;
        cv::Ptr<cv::AKAZE> orb = cv::AKAZE::create(descriptor_type, descriptor_size, descriptor_channels, 
            threshold, nOctaves, nOctaveLayers, diffusivity);
        orb->detect(img, keypoints);
    }
    else if (detectorType.compare("SIFT") == 0)
    {
        int nfeatures = 0;
        int nOctaveLayers = 3;
        double contrastThreshold = 0.04;
        double edgeThreshold = 10; 
        double sigma = 1.6;
        cv::Ptr<cv::Feature2D> sift = cv::xfeatures2d::SIFT::create(nfeatures, nOctaveLayers, contrastThreshold,
            edgeThreshold, sigma);
        sift->detect(img, keypoints);
    }
    else
    {
        cout << "wrong detector type" << endl;
        throw std::runtime_error("wrong detector type");
    }

    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    cout << detectorType + " detector with n= " << keypoints.size() << " keypoints in " << 1000 * t / 1.0 << " ms" << endl;

    if (bVis)
    {
        cv::Mat visImage = img.clone();
        cv::drawKeypoints(img, keypoints, visImage, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        string windowName = detectorType + " Results";
        cv::namedWindow(windowName, 2);
        imshow(windowName, visImage);
        cv::waitKey(0);
    }
}