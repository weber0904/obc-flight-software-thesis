#ifndef OBC_PersistentFaultManagerTester_HPP
#define OBC_PersistentFaultManagerTester_HPP

#include <string>

#include "OBC/Components/PersistentFaultManager/PersistentFaultManager.hpp"
#include "OBC/Components/PersistentFaultManager/PersistentFaultManagerGTestBase.hpp"

namespace OBC {

class PersistentFaultManagerTester final : public PersistentFaultManagerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 32;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    PersistentFaultManagerTester();

    ~PersistentFaultManagerTester() override;

    void testHistoryCommandRequiresConfiguration();

    void testHistoryCommandReportsLatestFirst();

    void testHistoryCommandClampsLargeLimit();

    void testHistoryCommandReportsEmptyConfiguredSet();

  private:
    void connectPorts();

    void initComponents();

    std::string makeTempRoot_() const;

    void cleanupTree_() const;

    bool configure_();

    OBC::PersistentFaultRecord makeRecord_(OBC::PersistentFaultRecordKind kind, U32 bootCount, U32 detail) const;

  private:
    std::string m_tempRoot;
    OBC::PersistentFaultManager component;
};

}  // namespace OBC

#endif
