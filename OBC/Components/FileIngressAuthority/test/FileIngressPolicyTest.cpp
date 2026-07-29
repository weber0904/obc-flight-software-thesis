#include <sys/stat.h>
#include <unistd.h>

#include "gtest/gtest.h"

#include "OBC/Components/FileIngressAuthority/FileIngressPolicy.hpp"
#include "OBC/Components/test/TestSupport.hpp"

namespace OBC {
namespace {

TEST(FileIngressPolicy, ConfiguresRuntimeScopedAliasAndAcceptsStagingLeaf) {
    TestSupport::TempDirectory runtimeRoot;
    ASSERT_TRUE(runtimeRoot.valid());

    FileIngressPolicy policy;
    ASSERT_TRUE(policy.configure(runtimeRoot.path(), runtimeRoot.path()));
    ASSERT_TRUE(policy.isConfigured());

    struct stat aliasInfo = {};
    ASSERT_EQ(::lstat(policy.getAliasPath().c_str(), &aliasInfo), 0);
    ASSERT_TRUE(S_ISLNK(aliasInfo.st_mode));
    EXPECT_EQ(policy.getAliasPath().find("/.sequence-staging"), std::string::npos);
    EXPECT_FALSE(policy.getRuntimePrefix().empty());
    EXPECT_EQ(policy.getRuntimePrefix().rfind(".stg-", 0), 0U);

    FileIngressRejectReason reason = FileIngressRejectReason::NONE;
    std::string canonicalPath;
    ASSERT_TRUE(policy.validateDestinationPath(".sequence-staging/alpha.seq", reason, canonicalPath));
    EXPECT_EQ(reason, FileIngressRejectReason::NONE);
    EXPECT_EQ(canonicalPath, TestSupport::joinPath(policy.getPhysicalStagingRoot(), "alpha.seq"));
}

TEST(FileIngressPolicy, RejectsTraversalAbsoluteAndNestedPaths) {
    TestSupport::TempDirectory runtimeRoot;
    ASSERT_TRUE(runtimeRoot.valid());

    FileIngressPolicy policy;
    ASSERT_TRUE(policy.configure(runtimeRoot.path(), runtimeRoot.path()));

    FileIngressRejectReason reason = FileIngressRejectReason::NONE;
    std::string canonicalPath;

    EXPECT_FALSE(policy.validateDestinationPath("/tmp/nope.seq", reason, canonicalPath));
    EXPECT_EQ(reason, FileIngressRejectReason::ABSOLUTE_PATH);

    EXPECT_FALSE(policy.validateDestinationPath(".sequence-staging/../nope.seq", reason, canonicalPath));
    EXPECT_EQ(reason, FileIngressRejectReason::PARENT_REFERENCE);

    EXPECT_FALSE(policy.validateDestinationPath(".sequence-staging/nested/nope.seq", reason, canonicalPath));
    EXPECT_EQ(reason, FileIngressRejectReason::SUBDIRECTORY_UNSUPPORTED);
}

TEST(FileIngressPolicy, RejectsSymlinkEscapeAtExistingDestination) {
    TestSupport::TempDirectory runtimeRoot;
    ASSERT_TRUE(runtimeRoot.valid());

    FileIngressPolicy policy;
    ASSERT_TRUE(policy.configure(runtimeRoot.path(), runtimeRoot.path()));

    const std::string symlinkPath = TestSupport::joinPath(policy.getPhysicalStagingRoot(), "escape.seq");
    ASSERT_EQ(::symlink("/tmp", symlinkPath.c_str()), 0);

    FileIngressRejectReason reason = FileIngressRejectReason::NONE;
    std::string canonicalPath;
    EXPECT_FALSE(policy.validateDestinationPath(".sequence-staging/escape.seq", reason, canonicalPath));
    EXPECT_EQ(reason, FileIngressRejectReason::SYMLINK_ESCAPE);
}

TEST(FileIngressPolicy, ConfigureFailsWhenStagingPathComponentIsNotDirectory) {
    TestSupport::TempDirectory runtimeRoot;
    ASSERT_TRUE(runtimeRoot.valid());

    const std::string sequencesPath = TestSupport::joinPath(runtimeRoot.path(), "sequences");
    ASSERT_TRUE(TestSupport::ensureDirectory(sequencesPath));
    const std::string stagingPath = TestSupport::joinPath(sequencesPath, "staging");
    ASSERT_TRUE(TestSupport::writeBinaryFile(
        stagingPath, std::vector<U8>{'n', 'o', 't', '-', 'a', '-', 'd', 'i', 'r', 'e', 'c', 't', 'o', 'r', 'y', '\n'}));

    FileIngressPolicy policy;
    EXPECT_FALSE(policy.configure(runtimeRoot.path(), runtimeRoot.path()));
}

}  // namespace
}  // namespace OBC
