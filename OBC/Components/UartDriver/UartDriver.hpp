#ifndef OBC_Components_UartDriver_HPP
#define OBC_Components_UartDriver_HPP

#include <array>
#include <memory>
#include <mutex>
#include <string>

#include "OBC/Components/UartDriver/UartDriverComponentAc.hpp"
#include "simulators/comm/ByteStreamTransport.hpp"

namespace OBC {

class UartDriver final : public UartDriverComponentBase {
  public:
    static constexpr U32 MAX_RUNTIME_SEND_SIZE = 256U;

    explicit UartDriver(const char* const compName);

    ~UartDriver() override;

    void configureTransport(std::unique_ptr<OBC::COMM::IByteStreamTransport> transport);

    void configureTransport(const std::shared_ptr<OBC::COMM::IByteStreamTransport>& transport);

    void setTransportForTest(OBC::COMM::IByteStreamTransport* transport);

    bool pollForTest();

    bool exchangeForTest(const std::string& request, std::string& response);

    bool exchangeDelimitedForTest(const std::string& request, std::string& response, char delimiter);

    bool exchangeForRuntime(const std::string& request, std::string& response);

    bool exchangeDelimitedForRuntime(const std::string& request, std::string& response, char delimiter);

    bool sendForRuntime(const U8* data, U32 size);

    bool queueSendForRuntime(const U8* data, U32 size);

    OBC::COMM::ByteStreamStats getStatsForRuntime() const;

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    void publishStats_();

    void emitError_(U32 code);

    bool sendImmediate_(const U8* data, U32 size);

    bool pollLocked_();

    bool shouldPublishStats_(const OBC::COMM::ByteStreamStats& stats);

  private:
    std::unique_ptr<OBC::COMM::IByteStreamTransport> m_ownedTransport;
    std::shared_ptr<OBC::COMM::IByteStreamTransport> m_sharedTransport;
    OBC::COMM::IByteStreamTransport* m_transport;
    bool m_openLatched;
    std::array<U8, MAX_RUNTIME_SEND_SIZE> m_pendingRuntimeSend;
    U32 m_pendingRuntimeSendSize;
    bool m_havePendingRuntimeSend;
    bool m_havePublishedStats;
    OBC::COMM::ByteStreamStats m_lastPublishedStats;
    mutable std::mutex m_runtimeMutex;
};

}  // namespace OBC

#endif
