#pragma once

#include <string>
#include <vector>

/**
 * @brief Utility class for exporting video files from PNG frame sequences.
 * 
 * This class handles the video encoding process using FFmpeg libraries,
 * converting PNG images to MP4 video format.
 */
class VideoExporter
{
public:
    /**
     * @brief Exports a sequence of PNG frames to an MP4 video file.
     * 
     * @param frameDirectory Path to directory containing PNG frames
     * @param outputVideoPath Path where the output MP4 file will be saved
     * @param fps Frames per second for the output video
     * @param targetWidth Target width for video (frames will be scaled to this)
     * @param targetHeight Target height for video (frames will be scaled to this)
     * @return true if export was successful, false otherwise
     */
    static bool ExportFramesToVideo(
        const std::string& frameDirectory,
        const std::string& outputVideoPath,
        int fps,
        int targetWidth = 1920,
        int targetHeight = 1080
    );

private:
    /**
     * @brief Collects and sorts PNG files from a directory.
     * 
     * @param frameDirectory Path to search for PNG files
     * @param outFramePaths Vector to store the collected frame paths
     * @return true if frames were found, false otherwise
     */
    static bool CollectFramePaths(
        const std::string& frameDirectory,
        std::vector<std::string>& outFramePaths
    );
};
