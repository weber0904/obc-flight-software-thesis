#ifndef OBC_SIMULATORS_GPS_GPSSOURCE_HPP
#define OBC_SIMULATORS_GPS_GPSSOURCE_HPP

#include <memory>
#include <cstdint>
#include <string>
#include <vector>

namespace OBC {
namespace GPS {

class IGpsSentenceSource {
  public:
    virtual ~IGpsSentenceSource() = default;

    virtual bool nextSentence(std::string& sentence) = 0;

    virtual void reset() = 0;
};

class FakeGpsSentenceSource final : public IGpsSentenceSource {
  public:
    explicit FakeGpsSentenceSource(std::vector<std::string> sentences);

    bool nextSentence(std::string& sentence) override;

    void reset() override;

  private:
    std::vector<std::string> m_sentences;
    std::size_t m_index;
};

class ReplayFileGpsSentenceSource final : public IGpsSentenceSource {
  public:
    explicit ReplayFileGpsSentenceSource(std::vector<std::string> sentences);

    bool nextSentence(std::string& sentence) override;

    void reset() override;

    static std::unique_ptr<ReplayFileGpsSentenceSource> loadFromFile(const std::string& path);

  private:
    std::vector<std::string> m_sentences;
    std::size_t m_index;
};

class SerialGpsSentenceSource final : public IGpsSentenceSource {
  public:
    SerialGpsSentenceSource(int fd, std::uint32_t timeoutMs);

    ~SerialGpsSentenceSource() override;

    SerialGpsSentenceSource(const SerialGpsSentenceSource&) = delete;

    SerialGpsSentenceSource& operator=(const SerialGpsSentenceSource&) = delete;

    bool nextSentence(std::string& sentence) override;

    void reset() override;

    static std::unique_ptr<SerialGpsSentenceSource> openDevice(const std::string& path,
                                                               std::uint32_t baudrate,
                                                               std::uint32_t timeoutMs);

  private:
    int m_fd;
    std::uint32_t m_timeoutMs;
    std::string m_buffer;
    bool m_discardUntilNewline;
};

std::vector<std::string> makeDefaultFakeNmeaSequence();

std::unique_ptr<IGpsSentenceSource> makeDefaultFakeGpsSentenceSource();

std::unique_ptr<IGpsSentenceSource> makeReplayFileGpsSentenceSource(const std::string& path);

std::unique_ptr<IGpsSentenceSource> makeSerialGpsSentenceSource(const std::string& path,
                                                                std::uint32_t baudrate,
                                                                std::uint32_t timeoutMs);

}  // namespace GPS
}  // namespace OBC

#endif
