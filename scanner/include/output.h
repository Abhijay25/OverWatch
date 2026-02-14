#pragma once

#include "secret_detector.h"
#include <string>
#include <vector>
#include <fstream>

namespace overwatch {

/**
 * CSV output writer for findings
 */
class CSVWriter {
public:
    /**
     * Construct CSV writer
     * @param output_file Path to CSV file
     * @param append If true, append to existing file; if false, overwrite
     */
    explicit CSVWriter(const std::string& output_file, bool append = true);

    /**
     * Write findings to CSV
     * @param findings Vector of findings to write
     * @return Number of findings written
     */
    int writeFindings(const std::vector<Finding>& findings);

    /**
     * Get total findings written
     */
    int getTotalWritten() const { return total_written_; }

private:
    std::string output_file_;
    bool append_;
    int total_written_ = 0;

    // Escape CSV field
    std::string escapeCSV(const std::string& field);

    // Check if file exists
    bool fileExists(const std::string& path);

    // Write CSV header
    void writeHeader(std::ofstream& file);
};

} // namespace overwatch
