# coding=utf-8
from moviepy.editor import VideoFileClip
from PIL import Image
import imagehash
import os
import time

"""
fishyy119 24/02/29
提取视频中全部不相似帧,并输出为jpg图片
支持自定义截取起止时间、跳帧幅度
使用moviepy进行视频读取
在大幅度跳帧的情况下,处理速度劣于opencv版本
在30帧一跳的情况下,处理花费时间约为opencv版本7倍以上
在跳帧幅度小的情况下,其可能表现出比opencv版本更优的时间性能
在2帧一跳的情况下,处理速度约为moviepy版本的2.5倍
"""

class ProgressReporter:
    def __init__(self, total_duration, fps, progress_interval, start, end):
        self.total_duration = total_duration  # 记录视频总时长
        self.progress_interval = progress_interval  # 记录提示间隔
        self.start = round(start / fps)  # 以秒为单位的起止时间
        self.end = round(end / fps)
        self.last_report_time = self.start  # 记录上次报告对应的视频时间
        self.start_time = time.time()

    def report_progress(self, elapsed_time, frame_count):
        if progress_interval > 0 and elapsed_time >= self.last_report_time + self.progress_interval:
            processed_time = time.time() - self.start_time
            percent = elapsed_time / (self.end - self.start) * 100
            # elapsed_time_str = self._time_format(elapsed_time)
            # total_time_str = self._time_format(self.total_duration)
            processed_time_str = self._time_format(processed_time)
            print(f"\r已处理 {percent:.2f} %的视频内容，已花费时间：{processed_time_str}，已提取图片数：{frame_count}", end = "")
            self.last_report_time = elapsed_time
    
    def report_result(self, frame_count):
        total_time = time.time() - self.start_time
        total_time_str = self._time_format(total_time)
        print(f"\n处理总用时：{total_time_str}，处理片段{self._time_format(self.start)}--{self._time_format(self.end)}，输出图片数：{frame_count}")

    def _time_format(self, seconds):
        hours = int(seconds // 3600)
        minutes = int((seconds % 3600) // 60)
        seconds = int(seconds % 60)
        return f"{hours:02d}:{minutes:02d}:{seconds:02d}"

def extract_frames(input_file, output_folder, start = 0, end = 0, frame_skip = 5, progress_interval=60, threshold=5):
    clip = VideoFileClip(input_file)
    # 将start end处理为帧序数
    if start <= 0:
        start = 0
    else:
        start = int(start * 60 * clip.fps)

    if end <= 0:
        end = int(clip.duration * clip.fps)
    else:
        end = int(end * 60 * clip.fps)
        if end > int(clip.duration * clip.fps):
            end = int(clip.duration * clip.fps)  # 防止终点超出末尾
    
    progress_reporter = ProgressReporter(clip.duration, clip.fps, progress_interval, start, end)  # 进度汇报
    hash_list = []  # 用于存储已输出图片的哈希值
    frame_count = 0

    for idx in range(start, end, frame_skip):

        progress_reporter.report_progress(idx / clip.fps, frame_count)
        frame = clip.get_frame(idx / clip.fps)  # 按索引获取视频帧
        image = Image.fromarray(frame)  # 将当前帧转换为图像对象

        # 判断当前图片是否与已输出的图片相似
        similar = False
        img_hash = imagehash.phash(image)
        for hash_value in reversed(hash_list):
            if hash_value - img_hash < threshold:
                similar = True
                break
    
        if not similar:
            frame_path = os.path.join(output_folder, f"frame_{int(idx / clip.fps // 60)}min_{frame_count:04d}.jpg")  # 使用 JPG 格式保存图片
            image.save(frame_path)  # 保存图像到输出文件夹
            hash_list.append(img_hash)  # 将当前图片的哈希值添加到列表中
            frame_count += 1  # 更新帧计数

    clip.close()  # 关闭视频文件
    progress_reporter.report_result(frame_count)

def get_input_file_path():
    while True:
        input_file = input("请输入视频文件路径(留空则默认为\"延河课堂.mp4\"):") or "延河课堂.mp4"
        if os.path.exists(input_file):
            return input_file
        else:
            print("文件路径不存在，请重新输入。")

def get_start_end():
    while True:
        start_input = input("请输入处理片段起点(分钟)(留空则默认为开头):") or '0'
        end_input = input("请输入处理片段终点(分钟)(留空则默认为结尾):") or '0'
        
        try:
            start = float(start_input)
            end = float(end_input)
            
            if start < 0 or end < 0:
                raise ValueError("时间不能小于0")
            elif start > end and abs(end - 0) > 1e-9:
                raise ValueError("起点不能大于终点")
            else:
                return start, end
        except ValueError as e:
            print(f"输入错误：{e} 。请重新输入。")

def get_non_negative_integer(prompt, default=None):
    while True:
        value_input = input(prompt) or default
        
        try:
            value = int(value_input)
            if value < 0:
                raise ValueError("请输入非负整数")
            else:
                return value
        except ValueError as e:
            print(f"输入错误：{e} 。请重新输入。")

if __name__ == "__main__":
    input_file = get_input_file_path()
    output_folder = input("请输入输出文件夹路径(留空则默认为\"output_yyMMdd_HHmmss\"):") or time.strftime('output_%y%m%d_%H%M%S', time.localtime())
    start, end = get_start_end()
    frame_skip = get_non_negative_integer("请输入跳帧检测值(留空则默认为30):", 30)
    progress_interval = get_non_negative_integer("请输入进度提示间隔时间(分钟)(留空默认为1分钟,0为不显示):", 1)
    threshold = get_non_negative_integer("请输入相似度比较阈值(留空则默认为5):", 5)

    # 检查工作目录下是否存在输出文件夹，如果不存在则创建
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
        
    extract_frames(input_file, output_folder, start, end, frame_skip, progress_interval * 60, threshold)  # 调用 extract_frames 函数处理视频
    input("按下回车键退出")