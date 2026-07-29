#include "OBC/Components/BeaconPublisher/BeaconPublisher.hpp"

namespace OBC {

BeaconPublisher::BeaconPublisher(const char* const compName)
    : BeaconPublisherComponentBase(compName),
      m_source(nullptr),
      m_sink(nullptr),
      m_enabled(false),
      m_runtimeSuppressed(false),
      m_periodTicks(DEFAULT_PERIOD_TICKS),
      m_ticksUntilPublish(0U),
      m_nextSequence(0U),
      m_emittedCount(0U),
      m_lastSizeBytes(0U) {}

BeaconPublisher::~BeaconPublisher() = default;

void BeaconPublisher::configureRuntime(OBC::StateData::IReducedStateSource* source,
                                       OBC::StateData::IBeaconSink* sink,
                                       bool enabled) {
    const std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    this->m_source = source;
    this->m_sink = sink;
    this->m_enabled = enabled;
    this->m_runtimeSuppressed = false;
    this->resetTicks_();
    this->m_nextSequence = 0U;
    this->m_emittedCount = 0U;
    this->m_lastSizeBytes = 0U;
    this->publishState_(BeaconError::OK);
}

void BeaconPublisher::configurePeriodForTest(U32 periodTicks) {
    const std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    this->m_periodTicks = periodTicks == 0U ? 1U : periodTicks;
    this->resetTicks_();
}

bool BeaconPublisher::publishNow() {
    const std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    return this->publishNowLocked_();
}

bool BeaconPublisher::publishNowLocked_() {
    if (!this->m_enabled || this->m_runtimeSuppressed) {
        return false;
    }

    if (this->m_source == nullptr || this->m_sink == nullptr) {
        this->publishState_(BeaconError::NOT_CONFIGURED);
        this->log_WARNING_HI_BEACON_NOT_CONFIGURED();
        return false;
    }

    OBC::StateData::ReducedStateV1 state = {};
    if (!this->m_source->getReducedStateForRuntime(state)) {
        this->publishState_(BeaconError::SOURCE_UNAVAILABLE);
        this->log_WARNING_HI_BEACON_SOURCE_UNAVAILABLE();
        return false;
    }

    const U32 sequence = this->m_nextSequence;
    const OBC::StateData::BeaconPacket packet = OBC::StateData::encodeBeaconV1(state, sequence);
    if (!this->m_sink->sendBeacon(packet.bytes.data(), static_cast<U32>(packet.bytes.size()))) {
        this->publishState_(BeaconError::SINK);
        this->log_WARNING_HI_BEACON_SINK_ERROR(sequence);
        return false;
    }

    this->m_lastSizeBytes = static_cast<U32>(packet.bytes.size());
    this->m_emittedCount++;
    this->m_nextSequence++;
    this->publishState_(BeaconError::OK);
    this->log_ACTIVITY_LO_BEACON_PACKET_EMITTED(sequence, this->m_lastSizeBytes);
    return true;
}

void BeaconPublisher::setUhfBeaconSuppressedForRuntime(bool suppressed) {
    const std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    this->m_runtimeSuppressed = suppressed;
    if (!suppressed) {
        this->m_ticksUntilPublish = 0U;
    }
}

void BeaconPublisher::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    const std::lock_guard<std::mutex> lock(this->m_runtimeMutex);
    if (!this->m_enabled) {
        return;
    }
    if (this->m_ticksUntilPublish > 0U) {
        this->m_ticksUntilPublish--;
        return;
    }
    static_cast<void>(this->publishNowLocked_());
    this->resetTicks_();
}

void BeaconPublisher::publishState_(BeaconError lastError) {
    this->tlmWrite_BEACON_SEQUENCE(this->m_nextSequence);
    this->tlmWrite_BEACON_EMITTED_COUNT(this->m_emittedCount);
    this->tlmWrite_BEACON_LAST_SIZE_BYTES(this->m_lastSizeBytes);
    this->tlmWrite_BEACON_LAST_ERROR(static_cast<U32>(lastError));
}

void BeaconPublisher::resetTicks_() {
    this->m_ticksUntilPublish = this->m_periodTicks > 0U ? (this->m_periodTicks - 1U) : 0U;
}

}  // namespace OBC
