#include "WavefileConversionTask.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <mutex>
#include <string>

using namespace std::filesystem;

namespace {
void print_usage()
{
    std::cout << "Converts all .wav files in a given folder to mp3 files in "
                 "the same folder\n\n"
              << "Usage: mp3_converter [dir]\n"
              << "Start this program with a single dir as a paramter.\n"
              << "The parameter is not traversed recursively!\n"
              << std::endl;
}

std::vector<path> list_all_wav_files(const path &directory)
{
    std::vector<path> files;

    directory_iterator dir_contents(directory);

    for (const auto &file : dir_contents)
    {
        if (file.is_regular_file() && file.path().extension() == ".wav")
        {
            files.emplace_back(file.path());
        }
    }

    return files;
}

// credit for the following two classes to:
// https://stackoverflow.com/questions/17196402/is-using-stdasync-many-times-for-small-tasks-performance-friendly
class Semaphore
{
  private:
    std::mutex              m;
    std::condition_variable cv;
    unsigned int            count;

  public:
    explicit Semaphore(unsigned int n)
        : count(n)
    {
    }

    void notify()
    {
        std::unique_lock<std::mutex> l(m);
        ++count;
        cv.notify_one();
    }

    void wait()
    {
        std::unique_lock<std::mutex> l(m);
        cv.wait(l, [this] { return count != 0; });
        --count;
    }
};

class Semaphore_waiter_notifier
{
    Semaphore &s;

  public:
    Semaphore_waiter_notifier(Semaphore &s)
        : s {s}
    {
        s.wait();
    }

    ~Semaphore_waiter_notifier()
    {
        s.notify();
    }
};

// start a few more threads than hardware concurrency is good for.
Semaphore thread_limiter(
    static_cast<unsigned int>(std::thread::hardware_concurrency() * 1.2));
} // namespace

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        print_usage();
        return 0;
    }

    if (argc > 2)
    {
        std::cerr << "Too many parameters!\n\n";
        return -1;
    }
    if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")
    {
        print_usage();
        return 0;
    }

    path potential_dir(argv[1]);
    if (!is_directory(potential_dir))
    {
        std::cerr << "Given parameter: " << potential_dir
                  << " is not a directory.\n";
        print_usage();

        return -1;
    }

    auto wav_file_list = list_all_wav_files(potential_dir);
    if (wav_file_list.size() == 0)
    {
        std::cerr << "No wav files found in folder " << potential_dir
                  << ". Did no work." << std::endl;
        return -1;
    }

    // we got valid parameters.

    auto conversion_task = [](const path &wav_file) {
        Semaphore_waiter_notifier {thread_limiter};
        mp3_converter::WavefileConversionTask task(wav_file);
        return task.run();
    };

    std::vector<std::future<bool>> futures;

    std::for_each(
        wav_file_list.cbegin(), wav_file_list.cend(), [&](const auto &path) {
            std::future<bool> wav_task =
                std::async(std::launch::async, conversion_task, path);
            futures.emplace_back(std::move(wav_task));
        });

    for (size_t i = 0; i < futures.size(); ++i)
    {
        const std::string msg_prefix {"Thread " + std::to_string(i) + ": "};
        try
        {
            auto &thread_result = futures[i];

            constexpr std::chrono::minutes timeout(2);

            auto success = thread_result.get();

            if (success)
            {
                // using to_string instead of operator since this is
                // multi_threaded io and submitting one stream prevents it from
                // being cut of by asynchronous I/O.
                std::cout << msg_prefix + "converted file "
                                 + wav_file_list[i].string() + "\n";
            }
            else
            {
                std::cout << msg_prefix + "failed.\n";
            }
        }
        catch (const std::exception &e)
        {
            std::cout << msg_prefix + "converting file "
                             + wav_file_list[i].string()
                             + " failed with exception: " + e.what() + "\n";
        }
    }


    return 0;
}
