#ifndef OBC_SIMULATORS_COMM_SERIALINGRESSACQUISITIONFILTER_HPP
#define OBC_SIMULATORS_COMM_SERIALINGRESSACQUISITIONFILTER_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace OBC {
namespace COMM {

class SerialIngressAcquisitionFilter {
  public:
    SerialIngressAcquisitionFilter();

    void onLinkOpened();

    void onLinkClosed();

    std::vector<std::uint8_t> filter(const std::uint8_t* data, std::size_t size);

    bool enabled() const;

    bool active() const;

  private:
    bool m_enabled;
    bool m_active;
    std::string m_pending;
};

}  // namespace COMM
}  // namespace OBC

#endif
