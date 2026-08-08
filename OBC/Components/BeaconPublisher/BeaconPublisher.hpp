#ifndef OBC_COMPONENTS_BEACONPUBLISHER_BEACONPUBLISHER_HPP
#define OBC_COMPONENTS_BEACONPUBLISHER_BEACONPUBLISHER_HPP

#include <mutex>

#include "OBC/Components/BeaconPublisher/BeaconPublisherComponentAc.hpp"
#include "OBC/Components/CommController/CommBeaconSuppressControl.hpp"
#include "OBC/Components/OnboardStateData/OnboardStateData.hpp"

namespace OBC {

class BeaconPublisher final : public BeaconPublisherComponentBase, public OBC::ICommBeaconSuppressControl {
  public:
    static constexpr U32 DEFAULT_PERIOD_TICKS = 17U;

    explicit BeaconPublisher(const char* const compName);

    ~BeaconPublisher() override;

    void configureRuntime(OBC::StateData::IReducedStateSource* source,
                          OBC::StateData::IBeaconSink* sink,
                          bool enabled = true);

    void configurePeriodForTest(U32 periodTicks);

    bool publishNow();

    void setUhfBeaconSuppressedForRuntime(bool suppressed) override;

  private:
    enum class BeaconError : U32 {
        OK = 0U,
        NOT_CONFIGURED = 1U,
        SOURCE_UNAVAILABLE = 2U,
        SINK = 3U,
    };

    void schedIn_handler(FwIndexType portNum, U32 context) override;
    bool publishNowLocked_();
    void publishState_(BeaconError lastError);
    void resetTicks_();

  private:
    std::mutex m_runtimeMutex;
    OBC::StateData::IReducedStateSource* m_source;
    OBC::StateData::IBeaconSink* m_sink;
    bool m_enabled;
    bool m_runtimeSuppressed;
    U32 m_periodTicks;
    U32 m_ticksUntilPublish;
    U32 m_nextSequence;
    U32 m_emittedCount;
    U32 m_lastSizeBytes;
};

}  // namespace OBC

#endif
