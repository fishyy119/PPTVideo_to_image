#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <iostream>
#include <filesystem>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

// 函数：提取特征点并输出图像，返回关键点和描述符
void extractAndSaveKeypoints(const string& imagePath, const string& outputFolder, 
                              vector<KeyPoint>& keypoints, Mat& descriptors) {
    // 读取输入图片
    Mat img = imread(imagePath);
    if (img.empty()) {
        cerr << "无法读取图片: " << imagePath << endl;
        return;
    }

    // 将图像转换为灰度图
    Mat gray;
    cvtColor(img, gray, COLOR_BGR2GRAY);

    // 二值化处理
    Mat binary;
    double threshold_value = 128; // 可以根据需要调整阈值
    cv::threshold(gray, binary, threshold_value, 255, THRESH_BINARY);

    // 创建ORB特征检测器
    Ptr<ORB> orb = ORB::create();
    orb->setMaxFeatures(500); // 设置最大特征点数

    // 检测关键点和描述符
    orb->detectAndCompute(binary, noArray(), keypoints, descriptors);

    // 绘制关键点
    Mat outputImage;
    drawKeypoints(gray, keypoints, outputImage, Scalar(0, 255, 0), DrawMatchesFlags::DEFAULT);

    // 确保输出文件夹存在
    if (!fs::exists(outputFolder)) {
        fs::create_directory(outputFolder);
    }

    // 保存输出图像
    string outputImagePath = outputFolder + "/output_" + fs::path(imagePath).filename().string();
    imwrite(outputImagePath, outputImage);

    cout << "处理完成，输出图像已保存至 " << outputImagePath << endl;
}

// 函数：匹配关键点并保存匹配结果图像
void matchKeypoints(const string& imagePath1, const vector<KeyPoint>& keypoints1,
                    const Mat& descriptors1, const string& imagePath2, 
                    const vector<KeyPoint>& keypoints2, const Mat& descriptors2,
                    vector<DMatch>& good_matches, const string& outputFolder,
                    double dist_threshold) {
    // 读取第二幅图像
    Mat img1 = imread(imagePath1);
    Mat img2 = imread(imagePath2);
    
    if (img1.empty() || img2.empty()) {
        cerr << "无法读取匹配的图像!" << endl;
        return;
    }


    vector<DMatch> matches;
    // // 使用BFMatcher进行匹配
    // BFMatcher matcher(NORM_HAMMING);
    // matcher.match(descriptors1, descriptors2, matches);
    // 快速近似匹配
    Ptr<FlannBasedMatcher> flannMatcher = FlannBasedMatcher::create();
    // flann需要的格式为32F
    Mat descriptors1_32F, descriptors2_32F;
    descriptors1.convertTo(descriptors1_32F, CV_32F);
    descriptors2.convertTo(descriptors2_32F, CV_32F);
    flannMatcher->match(descriptors1_32F, descriptors2_32F, matches);

    // 过滤匹配并生成匹配图像
    for (const auto& match : matches) {
        const KeyPoint& kp1 = keypoints1[match.queryIdx];
        const KeyPoint& kp2 = keypoints2[match.trainIdx];

        // 检查位置差异
        double distance = norm(kp1.pt - kp2.pt);
        if (distance < dist_threshold) {
            good_matches.push_back(match);
        }
    }

    // 生成最终匹配图像并保存
    Mat final_output_image;
    drawMatches(img1, keypoints1, img2, keypoints2, good_matches, final_output_image);
    string output_path = outputFolder + "/output_match.jpg";
    imwrite(output_path, final_output_image);
    cout << "匹配结果图像已保存至 " << output_path << endl;
}

// 函数：分析分块特征点
void analyzeGridKeypoints(const vector<KeyPoint>& keypoints, const vector<DMatch>& matches,
                          const Mat& image, int numCols, int numRows) {
    int rows = image.rows;
    int cols = image.cols;
    int gridHeight = rows / numRows;
    int gridWidth = cols / numCols;

    // 初始化每个格子的特征点和匹配点数量
    vector<int> keypointCounts(numRows * numCols, 0);
    vector<int> matchCounts(numRows * numCols, 0);

    // 统计特征点数量
    for (const auto& kp : keypoints) {
        int gridX = kp.pt.x / gridWidth;
        int gridY = kp.pt.y / gridHeight;
        if (gridX < numCols && gridY < numRows) {
            keypointCounts[gridY * numCols + gridX]++;
        }
    }

    // 统计匹配点数量
    for (const auto& match : matches) {
        const KeyPoint& kp1 = keypoints[match.queryIdx];
        int gridX = kp1.pt.x / gridWidth;
        int gridY = kp1.pt.y / gridHeight;
        if (gridX < numCols && gridY < numRows) {
            matchCounts[gridY * numCols + gridX]++;
        }
    }

    // 输出结果
    cout << "特征点和匹配点统计结果（每个格子）：" << endl;
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            cout << "格子 (" << x << ", " << y << "): "
                 << "特征点数量: " << keypointCounts[y * numCols + x] << ", "
                 << "匹配点数量: " << matchCounts[y * numCols + x] << endl;
        }
    }
}

int main() {
    // 创建test文件夹
    string outputFolder = "test_ORB";
    if (!fs::exists(outputFolder)) {
        fs::create_directory(outputFolder);
    }

    // 输入第一张图片路径
    string imagePath1;
    cout << "请输入第一张图片路径: ";
    getline(cin, imagePath1);

    vector<KeyPoint> keypoints1;
    Mat descriptors1;
    extractAndSaveKeypoints(imagePath1, outputFolder, keypoints1, descriptors1);

    // 输入第二张图片路径
    string imagePath2;
    cout << "请输入第二张图片路径: ";
    getline(cin, imagePath2);

    vector<KeyPoint> keypoints2;
    Mat descriptors2;
    extractAndSaveKeypoints(imagePath2, outputFolder, keypoints2, descriptors2);

    vector<DMatch> good_matches;
    double dist_threshold = 10.0; // 根据需要调整
    matchKeypoints(imagePath1, keypoints1, descriptors1, imagePath2, keypoints2, descriptors2, good_matches, outputFolder, dist_threshold);

    
    // 输入用户指定的格子数量
    int numCols = 4, numRows = 4;

    // 增加分块分析功能
    // analyzeGridKeypoints(keypoints1, good_matches, imread(imagePath1), numCols, numRows);
    analyzeGridKeypoints(keypoints2, good_matches, imread(imagePath2), numCols, numRows);

    return 0;
}
