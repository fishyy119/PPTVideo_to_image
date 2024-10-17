#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

// ʱ���ʽ��
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
    ProgressReporter(double total_duration, int fps, int progress_interval, int start, int end)
        : total_duration(total_duration), progress_interval(progress_interval), start(start), end(end) {
        last_report_time = start;
        start_time = chrono::high_resolution_clock::now();
    }

    void report_progress(double elapsed_time, int frame_count) {
        if (progress_interval > 0 && elapsed_time >= last_report_time + progress_interval) {
            auto now = chrono::high_resolution_clock::now();
            chrono::duration<double> processed_time = now - start_time;
            double percent = elapsed_time / (end - start) * 100;
            cout << "\r�Ѵ��� " << percent << " % ����Ƶ���ݣ��ѻ���ʱ�䣺" 
                << time_format(processed_time.count()) << "������ȡͼƬ����" << frame_count << flush;
            last_report_time = elapsed_time;
        }
    }

    void report_result(int frame_count) {
        auto now = chrono::high_resolution_clock::now();
        chrono::duration<double> total_time = now - start_time;
        cout << "\n��������ʱ��" << time_format(total_time.count()) << "�����ͼƬ����" << frame_count << endl;
    }

private:
    double total_duration;
    int progress_interval;
    int start, end;
    double last_report_time;
    chrono::time_point<chrono::high_resolution_clock> start_time;
};

// ��ȡ֡����
void extract_frames(const string& input_file, const string& output_folder, int start, int end, int frame_skip, int progress_interval, int threshold) {
    VideoCapture cap(input_file);

    if (!cap.isOpened()) {
        cerr << "�޷�����Ƶ�ļ�" << endl;
        return;
    }

    int fps = cap.get(CAP_PROP_FPS);
    int total_frames = cap.get(CAP_PROP_FRAME_COUNT);
    double total_duration = total_frames / double(fps);

    int start_frame = (start <= 0) ? 0 : start * 60 * fps;
    int end_frame = (end <= 0) ? total_frames : min(end * 60 * fps, total_frames);

    ProgressReporter progress_reporter(total_duration, fps, progress_interval, start_frame, end_frame);
    vector<size_t> hash_list; // �洢��ϣֵ
    int frame_count = 0;
    int frame_index = start_frame;

    Mat frame;
    while (cap.isOpened()) {
        cap.set(CAP_PROP_POS_FRAMES, frame_index);
        if (!cap.read(frame)) break;

        double elapsed_time = cap.get(CAP_PROP_POS_FRAMES) / double(fps);
        progress_reporter.report_progress(elapsed_time, frame_count);

        // �������ʵ������imagehash�Ĺ�ϣ�Ƚ�
        // ��������һ������pHash�ķ������õ�hashֵ����Խ��������ԱȽ�
        size_t img_hash = /* ����pHash */ 0;
        bool similar = false;

        // ���hash_list�еĹ�ϣֵ�Ƿ�����
        for (const auto& hash_value : hash_list) {
            if (abs((int)(hash_value - img_hash)) < threshold) {
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
    cout << "��������Ƶ�ļ�·��:"; 
    getline(cin, input_file);

    cout << "����������ļ���·��:"; 
    getline(cin, output_folder);

    // ����ļ��в����ڣ��򴴽�
    if (!fs::exists(output_folder)) {
        fs::create_directories(output_folder);
    }

    int start, end, frame_skip, progress_interval, threshold;
    cout << "���������(����):"; cin >> start;
    cout << "�������յ�(����):"; cin >> end;
    cout << "��������֡���ֵ:"; cin >> frame_skip;
    cout << "�����������ʾ���ʱ��(����):"; cin >> progress_interval;
    cout << "���������ƶȱȽ���ֵ:"; cin >> threshold;

    extract_frames(input_file, output_folder, start, end, frame_skip, progress_interval * 60, threshold);

    return 0;
}
