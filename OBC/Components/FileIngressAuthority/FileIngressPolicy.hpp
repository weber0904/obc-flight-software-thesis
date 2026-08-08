#ifndef OBC_FILE_INGRESS_POLICY_HPP
#define OBC_FILE_INGRESS_POLICY_HPP

#include <string>

namespace OBC {

enum class FileIngressRejectReason : unsigned int {
    NONE = 0,
    NOT_CONFIGURED = 1,
    EMPTY_PATH = 2,
    ABSOLUTE_PATH = 3,
    PARENT_REFERENCE = 4,
    PREFIX_MISMATCH = 5,
    SUBDIRECTORY_UNSUPPORTED = 6,
    INVALID_FILENAME = 7,
    SYMLINK_ESCAPE = 8,
    EXISTING_PATH_INVALID = 9,
    ALIAS_CREATE_FAILED = 10,
    RUNTIME_REWRITE_FAILED = 11,
};

class FileIngressPolicy {
  public:
    static constexpr const char* LOGICAL_PREFIX = ".sequence-staging/";
    static constexpr const char* PHYSICAL_STAGING_SUFFIX = "sequences/staging";

    bool configure(const std::string& runtimeRoot, const std::string& workingRoot);

    bool validateDestinationPath(const std::string& destinationPath,
                                 FileIngressRejectReason& reason,
                                 std::string& canonicalPhysicalPath) const;

    bool isConfigured() const;

    const std::string& getAliasPath() const;
    const std::string& getLogicalPrefix() const;
    const std::string& getPhysicalStagingRoot() const;
    const std::string& getRuntimePrefix() const;

  private:
    std::string m_aliasPath;
    std::string m_logicalPrefix;
    std::string m_physicalStagingRoot;
    std::string m_runtimePrefix;
    bool m_configured = false;
};

}  // namespace OBC

#endif
