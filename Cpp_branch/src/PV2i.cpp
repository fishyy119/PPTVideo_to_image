#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>
#include <set>
#include <cmath>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;


/**
 * @brief ��������������ʽ��Ϊ HH:MM:SS ���ַ�����ʽ��
 * 
 * @param seconds ������
 * @return string ��ʽ�����ʱ���ַ���
 */
string time_format(double seconds) {
    int hours = int(seconds / 3600);
    int minutes = int((seconds - hours * 3600) / 60);
    int secs = int(seconds) % 60;
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, secs);
    return string(buffer);
}

// ���ȱ�����
class ProgressReporter {
public:
/**
 * @brief ���캯��
 * 
 * @param total_duration ��Ƶ��ʱ����s��
 * @param fps ֡��
 * @param progress_interval ���ȱ�������min��
 * @param start ��ȡ��ʼ֡
 * @param end ��ȡ����֡
 */
    ProgressReporter(double total_duration, int fps, int progress_interval, int start, int end)
        : total_duration(total_duration), fps(fps), progress_interval(progress_interval), start(start), end(end) {
        
        start_time = chrono::high_resolution_clock::now();
        // ����������Ҫ�����ʱ���
        for (double t = start / fps + 1; t <= total_duration; t += progress_interval * 60) {
            report_times.push_back(t);
        }
    }

/**
 * @brief �������
 * 
 * @param elapsed_time ��ǰ�Ѵ�������Ƶʱ�䣨s��
 * @param frame_count �Ѿ���ȡ������ͼ��������Ч�ģ�
 */
    void report_progress(double elapsed_time, int frame_count) {
        if (!report_times.empty() && elapsed_time >= report_times.front()) {
            auto now = chrono::high_resolution_clock::now();
            chrono::duration<double> processed_time = now - start_time;
            double percent = (elapsed_time * fps - start) / (end - start) * 100;
            cout << "\r" << std::string(80, ' '); // �����ǰ��
            cout << "\r�Ѵ��� " << percent << " % ����Ƶ���ݣ��ѻ���ʱ�䣺" 
                << time_format(processed_time.count()) << "������ȡͼƬ����" << frame_count << flush;
            report_times.erase(report_times.begin()); // �Ƴ��ѱ����ʱ���
        }
    }

    void report_result(int frame_count) {
        auto now = chrono::high_resolution_clock::now();
        chrono::duration<double> total_time = now - start_time;
        cout << "\n��������ʱ��" << time_format(total_time.count()) << "�����ͼƬ����" << frame_count << endl;
    }

private:
    const double total_duration;  // ��ʱ��
    const int fps;                // ֡��
    const int progress_interval;   // ���ȱ�����
    const int start;              // ��ȡ��ʼ֡
    const int end;                // ��ȡ����֡
    chrono::time_point<chrono::high_resolution_clock> start_time; // ��ʼʱ��
    vector<double> report_times; // �洢���б����ʱ���
};

/**
 * @brief ����ͼ��ĸ�֪��ϣֵ
 * 
 * @param img ����ͼ��
 * @return size_t ����õ��Ĺ�ϣֵ
 */
size_t calculate_pHash(const Mat& img) {
    // ����ͼ���С
    Mat resized;
    resize(img, resized, Size(32, 32)); // ����Ϊ32x32

    // ת��Ϊ�Ҷ�ͼ
    Mat gray;
    cvtColor(resized, gray, COLOR_BGR2GRAY);

    // ����DCT������Ҫ�����͵ģ������ڴ�ת��һ��
    Mat gray_float;
    gray.convertTo(gray_float, CV_64F);

    // ����DCT
    Mat dct_result;
    dct(gray_float, dct_result);
    
    // ��ȡ���Ͻ�8x8��DCTϵ��
    Mat dct_roi = dct_result(Rect(0, 0, 8, 8));

    // ����ƽ��ֵ
    double mean = cv::mean(dct_roi)[0];
    
    // ���ɹ�ϣֵ
    size_t hash = 0;
    for (int i = 0; i < dct_roi.rows; ++i) {
        for (int j = 0; j < dct_roi.cols; ++j) {
            // ���DCTϵ������ƽ��ֵ�������ö�Ӧ��λΪ1
            hash <<= 1;
            if (dct_roi.at<double>(i, j) > mean) {
                hash |= 1;
            }
        }
    }
    
    return hash;
}

// ��ȡ֡����
/**
 * @brief ����Ƶ�ļ�����ȡָ����Χ��֡�������浽ָ���ļ��С�
 * 
 * @param input_file ������Ƶ�ļ�·��
 * @param output_folder ���֡���ļ���·��
 * @param start ��ʼʱ�䣨min��
 * @param end ����ʱ�䣨min��
 * @param frame_skip ������֡��
 * @param progress_interval ������ʾ���ʱ�䣨min��
 * @param threshold ���ƶȱȽ���ֵ
 */
void extract_frames(const string& input_file, const string& output_folder, int start, int end, int frame_skip, int progress_interval, int threshold) {
    // ����Ƶ�ļ������Ҽ���֡���Ȳ���
    VideoCapture cap(input_file);

    if (!cap.isOpened()) {
        cerr << "�޷�����Ƶ�ļ�" << endl;
        return;
    }

    int fps = cap.get(CAP_PROP_FPS); // ֡��
    int total_frames = cap.get(CAP_PROP_FRAME_COUNT); // ��֡��
    double total_duration = total_frames / double(fps); // ��ʱ����s��

    int start_frame = (start <= 0) ? 0 : start * 60 * fps; // ��ʼ֡
    int end_frame = (end <= 0) ? total_frames : min(end * 60 * fps, total_frames); // ����֡

    // �����ã�����Ҫ��
    ProgressReporter progress_reporter(total_duration, fps, progress_interval, start_frame, end_frame);
    vector<size_t> hash_list; // �洢��ϣֵ
    // set<size_t> hash_set; // �洢��ϣֵ
    int frame_count = 0; // �Ѿ���ȡ������ͼ��������Ч�ģ�
    int frame_index = start_frame;

    Mat frame;
    while (cap.isOpened()) {
        cap.set(CAP_PROP_POS_FRAMES, frame_index);
        if (!cap.read(frame)) break;

        size_t img_hash = calculate_pHash(frame);
        // TODO: ���õļ���㷨
        bool similar = false;

        // // ����ϣֵ�Ƿ�����
        // auto it = hash_set.lower_bound(img_hash);
        // if (it != hash_set.end() && abs((int)(*it ^ img_hash)) < threshold) {
        //     similar = true; // �ҵ����ƹ�ϣֵ
        // } else {
        //     hash_set.insert(img_hash); // �����µĹ�ϣֵ
        // }


        // ���hash_list�еĹ�ϣֵ�Ƿ�����
        for (const auto& hash_value : hash_list) {
            int hamming_distance = __builtin_popcount(hash_value ^ img_hash);
            if (hamming_distance < threshold) {
                similar = true;
                break;
            }
        }

        if (!similar) {
            double elapsed_time = cap.get(CAP_PROP_POS_FRAMES) / double(fps); // ��ǰ�Ѵ�������Ƶʱ�䣨s��
            string frame_path = output_folder + "/frame_" + to_string(int(elapsed_time / 60)) + "min_" + to_string(frame_count) + ".jpg";
            imwrite(frame_path, frame);
            hash_list.push_back(img_hash);
            progress_reporter.report_progress(elapsed_time, frame_count);
            frame_count++;
        }

        if (cap.get(CAP_PROP_POS_FRAMES) >= end_frame) break;
        frame_index += frame_skip;
    }

    cap.release();
    progress_reporter.report_result(frame_count);
}

// ��ȡ��ǰʱ�䲢��ʽ��Ϊ "output_MMDD_HHmmss"
string get_default_output_folder_name() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_c);

    std::ostringstream oss;
    oss << "output_"
        << std::setfill('0') << std::setw(2) << now_tm.tm_mon + 1
        << std::setfill('0') << std::setw(2) << now_tm.tm_mday << "_"
        << std::setfill('0') << std::setw(2) << now_tm.tm_hour
        << std::setfill('0') << std::setw(2) << now_tm.tm_min
        << std::setfill('0') << std::setw(2) << now_tm.tm_sec;

    return oss.str();
}

/**
 * @brief ��ȡ�û����룬���Ϊ���򷵻�ָ����Ĭ��ֵ��
 * 
 * @param prompt ��ʾ��Ϣ
 * @param default_prompt Ĭ����ʾֵ
 * @param default_value ���ص�Ĭ��ֵ
 * @return string �û�������ַ����򷵻ص�Ĭ��ֵ
 */
string get_input(const string& prompt, const string& default_prompt, const string& default_value) {
    cout << prompt << " (Ĭ��: " << default_prompt << "): ";
    string input;
    getline(cin, input);
    return input.empty() ? default_value : input; // �������Ϊ�գ��򷵻�Ĭ��ֵ
}

int main() {
    string input_file = get_input("��������Ƶ�ļ�·��", "1.mp4", "1.mp4");
    string output_folder = get_input("����������ļ���·��", "output_MMDD_HHmmss", get_default_output_folder_name());
    // ����ļ��в����ڣ��򴴽�
    if (!fs::exists(output_folder)) {
        fs::create_directories(output_folder);
    }

    int start = stoi(get_input("���������(����)", "��ͷ", "0"));
    int end = stoi(get_input("�������յ�(����)", "��β", "-1"));
    int frame_skip = stoi(get_input("��������֡���ֵ", "30", "30"));
    int progress_interval = stoi(get_input("�����������ʾ���ʱ��(����)", "5", "5"));
    int threshold = stoi(get_input("���������ƶȱȽ���ֵ", "4", "4"));

    // TODO: ���Ӵ������������ʾ

    // ���������Ĵ�����
    extract_frames(input_file, output_folder, start, end, frame_skip, progress_interval, threshold);
    system("PAUSE");

    return 0;
}