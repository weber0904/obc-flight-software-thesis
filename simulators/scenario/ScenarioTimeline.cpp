#include "simulators/scenario/ScenarioTimeline.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace OBC {
namespace Scenario {

namespace {

const char* const EXPECTED_HEADER =
    "time_sec,sunlight,battery_soc_pct,ground_pass_open,link_available,omega_x_rad_s,omega_y_rad_s,omega_z_rad_s";

std::string trimCopy(const std::string& value) {
    std::size_t begin = 0U;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        begin++;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1U])) != 0) {
        end--;
    }

    return value.substr(begin, end - begin);
}

std::vector<std::string> splitCommaSeparated(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ',')) {
        fields.push_back(trimCopy(field));
    }
    return fields;
}

bool parseBinaryField(const std::string& field, std::uint8_t& value) {
    if (field == "0" || field == "false" || field == "FALSE" || field == "False") {
        value = 0U;
        return true;
    }
    if (field == "1" || field == "true" || field == "TRUE" || field == "True") {
        value = 1U;
        return true;
    }
    return false;
}

bool parseSampleRow(const std::vector<std::string>& fields, ReplaySample& sample, std::string& errorMessage) {
    if (fields.size() != 8U) {
        errorMessage = "Scenario row must contain exactly 8 comma-separated fields";
        return false;
    }

    try {
        sample.time_sec = std::stod(fields[0]);
        sample.battery_soc_pct = std::stof(fields[2]);
        sample.omega_x_rad_s = std::stof(fields[5]);
        sample.omega_y_rad_s = std::stof(fields[6]);
        sample.omega_z_rad_s = std::stof(fields[7]);
    } catch (const std::exception&) {
        errorMessage = "Scenario row contains an invalid numeric value";
        return false;
    }

    if (!parseBinaryField(fields[1], sample.sunlight) || !parseBinaryField(fields[3], sample.ground_pass_open) ||
        !parseBinaryField(fields[4], sample.link_available)) {
        errorMessage = "Scenario row contains a non-binary sunlight, ground-pass, or link-availability field";
        return false;
    }

    return true;
}

}  // namespace

bool ScenarioTimeline::loadFromCsvFile(const std::string& path, std::string& errorMessage) {
    std::ifstream input(path);
    if (!input.is_open()) {
        errorMessage = "Failed to open scenario timeline file: " + path;
        return false;
    }

    return this->loadFromCsvStream(input, errorMessage);
}

bool ScenarioTimeline::loadFromCsvStream(std::istream& input, std::string& errorMessage) {
    std::vector<ReplaySample> parsedSamples;
    std::string line;
    std::size_t lineNumber = 0U;
    bool sawHeader = false;

    while (std::getline(input, line)) {
        lineNumber++;
        const std::string trimmed = trimCopy(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        if (!sawHeader) {
            if (trimmed != EXPECTED_HEADER) {
                errorMessage = "Scenario header does not match the expected replay contract";
                return false;
            }
            sawHeader = true;
            continue;
        }

        ReplaySample sample = {};
        const std::vector<std::string> fields = splitCommaSeparated(trimmed);
        if (!parseSampleRow(fields, sample, errorMessage)) {
            errorMessage += " at line " + std::to_string(lineNumber);
            return false;
        }

        if (!parsedSamples.empty() && sample.time_sec <= parsedSamples.back().time_sec) {
            errorMessage = "Scenario times must be strictly increasing";
            return false;
        }

        parsedSamples.push_back(sample);
    }

    if (!sawHeader) {
        errorMessage = "Scenario timeline file is missing the required header";
        return false;
    }

    if (parsedSamples.empty()) {
        errorMessage = "Scenario timeline file does not contain any replay samples";
        return false;
    }

    this->m_samples = parsedSamples;
    return true;
}

bool ScenarioTimeline::empty() const {
    return this->m_samples.empty();
}

const ReplaySample& ScenarioTimeline::firstSample() const {
    return this->m_samples.front();
}

ReplaySample ScenarioTimeline::sampleAt(double timeSec) const {
    if (this->m_samples.empty()) {
        return {};
    }

    const auto upper = std::upper_bound(
        this->m_samples.begin(),
        this->m_samples.end(),
        timeSec,
        [](double replayTime, const ReplaySample& sample) { return replayTime < sample.time_sec; });

    if (upper == this->m_samples.begin()) {
        return this->m_samples.front();
    }

    if (upper == this->m_samples.end()) {
        return this->m_samples.back();
    }

    return *(upper - 1);
}

const std::vector<ReplaySample>& ScenarioTimeline::samples() const {
    return this->m_samples;
}

}  // namespace Scenario
}  // namespace OBC
