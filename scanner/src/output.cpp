#include "output.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>

namespace overwatch {

CSVWriter::CSVWriter(const std::string& output_file, bool append)
    : output_file_(output_file), append_(append) {
}

bool CSVWriter::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::string CSVWriter::escapeCSV(const std::string& field) {
    // If field contains comma, quote, or newline, wrap in quotes and escape quotes
    if (field.find(',') != std::string::npos ||
        field.find('"') != std::string::npos ||
        field.find('\n') != std::string::npos) {
        std::string escaped = field;
        // Escape quotes by doubling them
        size_t pos = 0;
        while ((pos = escaped.find('"', pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\"\"");
            pos += 2;
        }
        return "\"" + escaped + "\"";
    }
    return field;
}

void CSVWriter::writeHeader(std::ofstream& file) {
    file << "timestamp,repo_owner,repo_name,repo_url,file_path,file_url,line_number,secret_type,confidence\n";
}

int CSVWriter::writeFindings(const std::vector<Finding>& findings) {
    if (findings.empty()) {
        spdlog::debug("No findings to write");
        return 0;
    }

    // Open file
    std::ofstream file;
    bool write_header = false;

    if (append_ && fileExists(output_file_)) {
        // Append to existing file
        file.open(output_file_, std::ios::app);
        spdlog::debug("Appending to existing file: {}", output_file_);
    } else {
        // Create new file
        file.open(output_file_, std::ios::out);
        write_header = true;
        spdlog::info("Creating new CSV file: {}", output_file_);
    }

    if (!file.is_open()) {
        spdlog::error("Failed to open output file: {}", output_file_);
        return 0;
    }

    // Write header if new file
    if (write_header) {
        writeHeader(file);
    }

    // Get current timestamp in ISO 8601 format
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream timestamp_ss;
    timestamp_ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    std::string timestamp = timestamp_ss.str();

    // Write each finding
    int written = 0;
    for (const auto& finding : findings) {
        file << escapeCSV(timestamp) << ","
             << escapeCSV(finding.repo_owner) << ","
             << escapeCSV(finding.repo_name) << ","
             << escapeCSV(finding.repo_url) << ","
             << escapeCSV(finding.file_path) << ","
             << escapeCSV(finding.file_url) << ","
             << finding.line_number << ","
             << escapeCSV(finding.secret_type) << ","
             << "high\n";  // All findings are high confidence for now
        written++;
    }

    file.close();
    total_written_ += written;

    spdlog::info("Wrote {} findings to {}", written, output_file_);
    return written;
}

} // namespace overwatch
