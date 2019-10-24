#ifndef WAVEFILE_WAVEFILE_H
#define WAVEFILE_WAVEFILE_H

#include <filesystem>

/**
 * @namespace mp3_converter
 *
 * @brief Namespace containing the classes used in mp3_converter.
 */
namespace mp3_converter {
/**
 * @class WavefileConversionTask
 *
 * @brief A task to convert a given wav file to an mp3 file using lame encoder.
 *
 * Output will always be in the same dir where the project is running, with the
 * extension exchanged from wav to mp3 to match the file contents.
 */
class WavefileConversionTask
{
  public:
    /**
     * @brief Create a conversion task from the given in_file.
     *
     * @param in_file The file to convert in this task.
     */
    WavefileConversionTask(const std::filesystem::path &in_file);

    /**
     * @brief Run the conversion.
     *
     * @throws wave_format_error When the wave file could not be read correctly.
     * @throws lame_encoding_error When lame encoding fails.
     *
     * @return True if successfull, else false.
     */
    bool run();

    /**
     * @struct wave_format_error
     *
     * @brief An error class indicating that reading of the wav file failed.
     */
    struct wave_format_error : std::runtime_error
    {
        // inherit c'tors
        using std::runtime_error::runtime_error;
    };

  private:
    size_t m_task_num;

    std::filesystem::path m_wav_file_in;
};

} // namespace mp3_converter
#endif /* WAVEFILE_WAVEFILE_H */
