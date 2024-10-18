#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <iostream>
#include <filesystem>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

// ��������ȡ�����㲢���ͼ�񣬷��عؼ����������
void extractAndSaveKeypoints(const string& imagePath, const string& outputFolder, 
                              vector<KeyPoint>& keypoints, Mat& descriptors) {
    // ��ȡ����ͼƬ
    Mat img = imread(imagePath);
    if (img.empty()) {
        cerr << "�޷���ȡͼƬ: " << imagePath << endl;
        return;
    }

    // ��ͼ��ת��Ϊ�Ҷ�ͼ
    Mat gray;
    cvtColor(img, gray, COLOR_BGR2GRAY);

    // ��ֵ������
    Mat binary;
    double threshold_value = 128; // ���Ը�����Ҫ������ֵ
    cv::threshold(gray, binary, threshold_value, 255, THRESH_BINARY);

    // ����ORB���������
    Ptr<ORB> orb = ORB::create();
    orb->setMaxFeatures(500); // ���������������

    // ���ؼ����������
    orb->detectAndCompute(binary, noArray(), keypoints, descriptors);

    // ���ƹؼ���
    Mat outputImage;
    drawKeypoints(gray, keypoints, outputImage, Scalar(0, 255, 0), DrawMatchesFlags::DEFAULT);

    // ȷ������ļ��д���
    if (!fs::exists(outputFolder)) {
        fs::create_directory(outputFolder);
    }

    // �������ͼ��
    string outputImagePath = outputFolder + "/output_" + fs::path(imagePath).filename().string();
    imwrite(outputImagePath, outputImage);

    cout << "������ɣ����ͼ���ѱ����� " << outputImagePath << endl;
}

// ������ƥ��ؼ��㲢����ƥ����ͼ��
void matchKeypoints(const string& imagePath1, const vector<KeyPoint>& keypoints1,
                    const Mat& descriptors1, const string& imagePath2, 
                    const vector<KeyPoint>& keypoints2, const Mat& descriptors2,
                    vector<DMatch>& good_matches, const string& outputFolder,
                    double dist_threshold) {
    // ��ȡ�ڶ���ͼ��
    Mat img1 = imread(imagePath1);
    Mat img2 = imread(imagePath2);
    
    if (img1.empty() || img2.empty()) {
        cerr << "�޷���ȡƥ���ͼ��!" << endl;
        return;
    }


    vector<DMatch> matches;
    // // ʹ��BFMatcher����ƥ��
    // BFMatcher matcher(NORM_HAMMING);
    // matcher.match(descriptors1, descriptors2, matches);
    // ���ٽ���ƥ��
    Ptr<FlannBasedMatcher> flannMatcher = FlannBasedMatcher::create();
    // flann��Ҫ�ĸ�ʽΪ32F
    Mat descriptors1_32F, descriptors2_32F;
    descriptors1.convertTo(descriptors1_32F, CV_32F);
    descriptors2.convertTo(descriptors2_32F, CV_32F);
    flannMatcher->match(descriptors1_32F, descriptors2_32F, matches);

    // ����ƥ�䲢����ƥ��ͼ��
    for (const auto& match : matches) {
        const KeyPoint& kp1 = keypoints1[match.queryIdx];
        const KeyPoint& kp2 = keypoints2[match.trainIdx];

        // ���λ�ò���
        double distance = norm(kp1.pt - kp2.pt);
        if (distance < dist_threshold) {
            good_matches.push_back(match);
        }
    }

    // ��������ƥ��ͼ�񲢱���
    Mat final_output_image;
    drawMatches(img1, keypoints1, img2, keypoints2, good_matches, final_output_image);
    string output_path = outputFolder + "/output_match.jpg";
    imwrite(output_path, final_output_image);
    cout << "ƥ����ͼ���ѱ����� " << output_path << endl;
}

// �����������ֿ�������
void analyzeGridKeypoints(const vector<KeyPoint>& keypoints, const vector<DMatch>& matches,
                          const Mat& image, int numCols, int numRows) {
    int rows = image.rows;
    int cols = image.cols;
    int gridHeight = rows / numRows;
    int gridWidth = cols / numCols;

    // ��ʼ��ÿ�����ӵ��������ƥ�������
    vector<int> keypointCounts(numRows * numCols, 0);
    vector<int> matchCounts(numRows * numCols, 0);

    // ͳ������������
    for (const auto& kp : keypoints) {
        int gridX = kp.pt.x / gridWidth;
        int gridY = kp.pt.y / gridHeight;
        if (gridX < numCols && gridY < numRows) {
            keypointCounts[gridY * numCols + gridX]++;
        }
    }

    // ͳ��ƥ�������
    for (const auto& match : matches) {
        const KeyPoint& kp1 = keypoints[match.queryIdx];
        int gridX = kp1.pt.x / gridWidth;
        int gridY = kp1.pt.y / gridHeight;
        if (gridX < numCols && gridY < numRows) {
            matchCounts[gridY * numCols + gridX]++;
        }
    }

    // ������
    cout << "�������ƥ���ͳ�ƽ����ÿ�����ӣ���" << endl;
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            cout << "���� (" << x << ", " << y << "): "
                 << "����������: " << keypointCounts[y * numCols + x] << ", "
                 << "ƥ�������: " << matchCounts[y * numCols + x] << endl;
        }
    }
}

int main() {
    // ����test�ļ���
    string outputFolder = "test_ORB";
    if (!fs::exists(outputFolder)) {
        fs::create_directory(outputFolder);
    }

    // �����һ��ͼƬ·��
    string imagePath1;
    cout << "�������һ��ͼƬ·��: ";
    getline(cin, imagePath1);

    vector<KeyPoint> keypoints1;
    Mat descriptors1;
    extractAndSaveKeypoints(imagePath1, outputFolder, keypoints1, descriptors1);

    // ����ڶ���ͼƬ·��
    string imagePath2;
    cout << "������ڶ���ͼƬ·��: ";
    getline(cin, imagePath2);

    vector<KeyPoint> keypoints2;
    Mat descriptors2;
    extractAndSaveKeypoints(imagePath2, outputFolder, keypoints2, descriptors2);

    vector<DMatch> good_matches;
    double dist_threshold = 10.0; // ������Ҫ����
    matchKeypoints(imagePath1, keypoints1, descriptors1, imagePath2, keypoints2, descriptors2, good_matches, outputFolder, dist_threshold);

    
    // �����û�ָ���ĸ�������
    int numCols = 4, numRows = 4;

    // ���ӷֿ��������
    // analyzeGridKeypoints(keypoints1, good_matches, imread(imagePath1), numCols, numRows);
    analyzeGridKeypoints(keypoints2, good_matches, imread(imagePath2), numCols, numRows);

    return 0;
}
