#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>
#include <cmath>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;


/**
 * @brief 将给定的秒数格式化为 HH:MM:SS 的字符串格式。
 * 
 * @param seconds 总秒数
 * @return string 格式化后的时间字符串
 */
string time_format(double seconds) {
    int hours = int(seconds / 3600);
    int minutes = int((seconds - hours * 3600) / 60);
    int secs = int(seconds) % 60;
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, secs);
    return string(buffer);
}

// 进度报告类
class ProgressReporter {
public:
/**
 * @brief 构造函数
 * 
 * @param total_duration 视频总时长（s）
 * @param fps 帧率
 * @param progress_interval 进度报告间隔（min）
 * @param start 提取起始帧
 * @param end 提取结束帧
 */
    ProgressReporter(double total_duration, int fps, int progress_interval, int start, int end)
        : total_duration(total_duration), fps(fps), progress_interval(progress_interval), start(start), end(end) {
        
        start_time = chrono::high_resolution_clock::now();
        // 计算所有需要报告的时间点
        for (double t = start / fps; t <= total_duration; t += progress_interval * 60) {
            report_times.push_back(t);
        }
    }

/**
 * @brief 输出报告
 * 
 * @param elapsed_time 当前已处理到的视频时间（s）
 * @param frame_count 已经提取出来的图像数（有效的）
 */
    void report_progress(double elapsed_time, int frame_count) {
        if (!report_times.empty() && elapsed_time >= report_times.front()) {
            auto now = chrono::high_resolution_clock::now();
            chrono::duration<double> processed_time = now - start_time;
            double percent = (elapsed_time * fps - start) / (end - start) * 100;
            cout << "\r已处理 " << percent << " % 的视频内容，已花费时间：" 
                << time_format(processed_time.count()) << "，已提取图片数：" << frame_count << flush;
            report_times.erase(report_times.begin()); // 移除已报告的时间点
        }
    }

    void report_result(int frame_count) {
        auto now = chrono::high_resolution_clock::now();
        chrono::duration<double> total_time = now - start_time;
        cout << "\n处理总用时：" << time_format(total_time.count()) << "，输出图片数：" << frame_count << endl;
    }

private:
    const double total_duration;  // 总时长
    const int fps;                // 帧率
    const int progress_interval;   // 进度报告间隔
    const int start;              // 提取开始帧
    const int end;                // 提取结束帧
    chrono::time_point<chrono::high_resolution_clock> start_time; // 开始时间
    vector<double> report_times; // 存储所有报告的时间点
};

/**
 * @brief 计算图像的感知哈希值
 * 
 * @param img 输入图像
 * @return size_t 计算得到的哈希值
 */
size_t calculate_pHash(const Mat& img) {
    // 调整图像大小
    Mat resized;
    resize(img, resized, Size(32, 32)); // 调整为32x32

    // 转换为灰度图
    Mat gray;
    cvtColor(resized, gray, COLOR_BGR2GRAY);

    // 后面DCT函数需要浮点型的，所以在此转化一下
    Mat gray_float;
    gray.convertTo(gray_float, CV_64F);

    // 计算DCT
    Mat dct_result;
    dct(gray_float, dct_result);
    
    // 提取左上角8x8的DCT系数
    Mat dct_roi = dct_result(Rect(0, 0, 8, 8));

    // 计算平均值
    double mean = cv::mean(dct_roi)[0];
    
    // 生成哈希值
    size_t hash = 0;
    for (int i = 0; i < dct_roi.rows; ++i) {
        for (int j = 0; j < dct_roi.cols; ++j) {
            // 如果DCT系数大于平均值，则设置对应的位为1
            hash <<= 1;
            if (dct_roi.at<double>(i, j) > mean) {
                hash |= 1;
            }
        }
    }
    
    return hash;
}

// 提取帧函数
/**
 * @brief 从视频文件中提取指定范围的帧，并保存到指定文件夹。
 * 
 * @param input_file 输入视频文件路径
 * @param output_folder 输出帧的文件夹路径
 * @param start 开始时间（min）
 * @param end 结束时间（min）
 * @param frame_skip 跳过的帧数
 * @param progress_interval 进度提示间隔时间（min）
 * @param threshold 相似度比较阈值
 */
void extract_frames(const string& input_file, const string& output_folder, int start, int end, int frame_skip, int progress_interval, int threshold) {
    // 打开视频文件，并且计算帧数等参数
    VideoCapture cap(input_file);

    if (!cap.isOpened()) {
        cerr << "无法打开视频文件" << endl;
        return;
    }

    int fps = cap.get(CAP_PROP_FPS); // 帧率
    int total_frames = cap.get(CAP_PROP_FRAME_COUNT); // 总帧数
    double total_duration = total_frames / double(fps); // 总时长（s）

    int start_frame = (start <= 0) ? 0 : start * 60 * fps; // 起始帧
    int end_frame = (end <= 0) ? total_frames : min(end * 60 * fps, total_frames); // 结束帧

    // 报告用，可能要改
    ProgressReporter progress_reporter(total_duration, fps, progress_interval, start_frame, end_frame);
    vector<size_t> hash_list; // 存储哈希值
    int frame_count = 0; // 已经提取出来的图像数（有效的）
    int frame_index = start_frame;

    Mat frame;
    while (cap.isOpened()) {
        cap.set(CAP_PROP_POS_FRAMES, frame_index);
        if (!cap.read(frame)) break;

        double elapsed_time = cap.get(CAP_PROP_POS_FRAMES) / double(fps); // 当前已处理到的视频时间（s）
        progress_reporter.report_progress(elapsed_time, frame_count);

        size_t img_hash = calculate_pHash(frame);
        bool similar = false;

        // 检查hash_list中的哈希值是否相似
        for (const auto& hash_value : hash_list) {
            int hamming_distance = __builtin_popcount(hash_value ^ img_hash);
            if (hamming_distance < threshold) {
                similar = true;
                break;
            }
        }

        if (!similar) {
            string frame_path = output_folder + "/frame_" + to_string(int(elapsed_time / 60)) + "min_" + to_string(frame_count) + ".jpg";
            imwrite(frame_path, frame);
            hash_list.push_back(img_hash);
            frame_count++;
        }

        if (cap.get(CAP_PROP_POS_FRAMES) >= end_frame) break;
        frame_index += frame_skip;
    }

    cap.release();
    progress_reporter.report_result(frame_count);
}


int main(void) {
    string input_file, output_folder;
    cout << "请输入视频文件路径:"; 
    getline(cin, input_file);

    cout << "请输入输出文件夹路径:"; 
    getline(cin, output_folder);

    // 如果文件夹不存在，则创建
    if (!fs::exists(output_folder)) {
        fs::create_directories(output_folder);
    }

    int start, end, frame_skip, progress_interval, threshold;
    cout << "请输入起点(分钟):"; cin >> start;
    cout << "请输入终点(分钟):"; cin >> end;
    cout << "请输入跳帧检测值:"; cin >> frame_skip;
    cout << "请输入进度提示间隔时间(分钟):"; cin >> progress_interval;
    cout << "请输入相似度比较阈值:"; cin >> threshold;

    extract_frames(input_file, output_folder, start, end, frame_skip, progress_interval, threshold);

    system("PAUSE");

    return 0;
}
